// The MIT License (MIT)
//
// Copyright (c) 2021 Mateusz Pusz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <mp-coro/bits/noncopyable.h>
#include <mp-coro/bits/task_promise_storage.h>
#include <mp-coro/concepts.h>
#include <mp-coro/coro_ptr.h>
#include <mp-coro/trace.h>
#include <mp-coro/type_traits.h>
#include <concepts>
#include <coroutine>

namespace mp_coro {

/// Task that produces a value of type @p T. To get that value, simply await the
/// @ref task.
///
/// Tasks are lazy: they always suspend initially. Each task has an optional
/// continuation (i.e. the coroutine that is waiting for the result of this 
/// task) that is resumed at the final suspend point.
///
/// @tparam T           Type of the value returned.
/// @tparam Allocator   Has no effect.
///
/// @see https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer
///
/// @par Example
///
/// ```cpp
/// task<> foo() {
///     co_return;
/// }
/// task<> bar() {
///     task<> f = foo();
///     co_await f;
/// }
/// ```
/// @mermaid
/// %%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#f4f4ff', 'secondaryColor': 'rgba(244,244,255,0.9)', 'tertiaryColor': '#F9FAFC'}}}%%
/// flowchart
///     start(( ))
///     subgraph bar
///     bar_coro_alloc["coroutine frame allocation\n<code>promise_type promise {};</code>\n<code>promise.get_return_object()</code>"]
///     bar_coro_alloc --> call_foo["<code>task&lt;&gt; f = foo();</code>"]
///     await_f["<code>co_await f;</code>"]
///     await_f --> op_co_await["<code>f.operator co_await()</code>"]
///     awaiter_promise["<code>task::awaiter a {f.promise}</code>"]
///     op_co_await --> awaiter_promise
///     await_rdy["<code>a.await_ready()</code>"]
///     awaiter_promise --> await_rdy
///     await_rdy --> if_await_rdy{{"<code>f.done()</code>"}}
///     if_await_rdy -->|false| await_sus["<code>a.await_suspend(bar)</code>"]
///     if_await_rdy -->|true| await_rsm["<code>a.await_resume()</code>"]
///     await_sus --> await_sus_body["<code>f.promise.continuation = bar\nreturn foo</code>"]
///     await_rsm --> bar_ret_void["<code>promise.return_void()</code>"]
///     bar_ret_void --> destroy_f["<code>f.~task()</code>"]
///     destroy_f --> destroy_foo["<code>foo.destroy()</code>"]
///     destroy_foo --> bar_final_sus["<code>promise.final_suspend()</code>"]
///     end
///     subgraph foo
///     coro_alloc["coroutine frame allocation\n<code>promise_type promise {};</code>\n<code>promise.get_return_object()</code>"]
///     coro_alloc --> init_sus["<code>co_await promise.initial_suspend()</code>"]
///     co_ret["<code>co_return;</code>"]
///     co_ret --> ret_void["<code>promise.return_void()</code>"]
///     ret_void --> final_sus["<code>promise.final_suspend()</code>"]
///     final_sus --> final_sus_a["<code>task::promise_type::final_awaiter a {}</code>"]
///     final_sus_a --> final_await_sus["<code>a.await_suspend(foo)</code>"]
///     final_await_sus --> final_await_sus_body["<code>return promise.continuation</code>"]
///     end
///     end_(( ))
///     start --> bar_coro_alloc
///     call_foo -->|Function call| coro_alloc
///     init_sus -->|Suspend| await_f
///     await_sus_body -->|Symmetric Control Transfer| co_ret
///     final_await_sus_body -->|Symmetric Control Transfer| await_rsm
///     bar_final_sus --> end_
/// @end_mermaid
///
/// @ingroup coro_ret_types
template <task_value_type T = void, typename Allocator = void>
class [[nodiscard]] task {
  public:
    /// The type of the value produced by this @ref task.
    using value_type = T;

    /// Required promise type for coroutines returning a @ref task. Stores the
    /// value produced by the @ref task, and a handle to an optional
    /// continuation to execute
    /// @todo when exactly?
    struct promise_type : private detail::noncopyable, detail::task_promise_storage<T> {
        std::coroutine_handle<> continuation = std::noop_coroutine();

        /// Returns a @ref task that references this promise.
        task get_return_object() noexcept {
            TRACE_FUNC();
            return this;
        }

        /// Lazy: not started until awaited.
        static std::suspend_always initial_suspend() noexcept {
            TRACE_FUNC();
            return {};
        }

        /// Awaiter returned by @ref final_suspend.
        struct final_awaiter : std::suspend_always {
            std::coroutine_handle<>
            await_suspend(std::coroutine_handle<promise_type> this_coro) noexcept {
                TRACE_FUNC();
                return this_coro.promise().continuation;
            }
        };

        /// Resumes the continuation of this @ref task when awaited. Beyond this
        /// point, the task's coroutine will never be resumed.
        static awaiter_of<void> auto final_suspend() noexcept {
            TRACE_FUNC();
            return final_awaiter {};
        }
    };

    task(task &&) = default;           ///< Move constructor.
    task &operator=(task &&) = delete; ///< Move assignment not allowed.

  private:
    /// Awaiter type for awaiting the result of a @ref task.
    struct awaiter {
        /// Reference to the promise object of the @ref task in question.
        promise_type &promise;

        /// Returns true if the @ref task's coroutine is already done
        /// (suspended at its final suspension point).
        bool await_ready() const noexcept {
            TRACE_FUNC();
            return std::coroutine_handle<promise_type>::from_promise(promise).done();
        }

        /// Set the current coroutine as this @ref task's continuation, and then
        /// resume this @ref task's coroutine.
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) const noexcept {
            TRACE_FUNC();
            promise.continuation = h;
            return std::coroutine_handle<promise_type>::from_promise(promise);
        }

        /// Return the value of the @ref task's promise.
        decltype(auto) await_resume() const {
            TRACE_FUNC();
            return promise.get();
        }
    };

  public:
    /// When awaited, sets the current coroutine as this @ref task's
    /// continuation, and then resumes this @ref task's coroutine.
    awaiter_of<T> auto operator co_await() const &noexcept {
        TRACE_FUNC();
        return awaiter(*promise_);
    }

    awaiter_of<const T &> auto
    operator co_await() const &noexcept requires std::move_constructible<T> {
        TRACE_FUNC();
        return awaiter(*promise_);
    }

    awaiter_of<T &&> auto operator co_await() const &&noexcept requires std::move_constructible<T> {
        TRACE_FUNC();

        struct rvalue_awaiter : awaiter {
            T &&await_resume() {
                TRACE_FUNC();
                return std::move(this->promise).get();
            }
        };
        return rvalue_awaiter({*promise_});
    }

  private:
    /// An owning pointer to the promise object in the coroutine frame. 
    /// When the task is destructed, this will cause the coroutine frame to be
    /// destroyed automatically.
    promise_ptr<promise_type> promise_;

    /// Private constructor used in @ref promise_type::get_return_object.
    task(promise_type *promise) : promise_(promise) { TRACE_FUNC(); }
};

template <awaitable A>
task<remove_rvalue_reference_t<await_result_t<A>>> make_task(A &&awaitable) {
    TRACE_FUNC();
    co_return co_await std::forward<A>(awaitable);
}

} // namespace mp_coro
