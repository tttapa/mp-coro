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
#include <mp-coro/coro_ptr.h>
#include <mp-coro/trace.h>
#include <mp-coro/type_traits.h>
#include <coroutine>

namespace mp_coro::detail {

/// Lazy task that can later be started explicitly, and that notifies another
/// variable (the “sync” object) of its completion.
/// This class doesn't spawn any threads itself, it just defines a coroutine
/// with the notification of the “sync” object as its continuation.
template <sync_notification_type Sync, task_value_type T>
class [[nodiscard]] synchronized_task {
  public:
    /// The type of the value produced by this @ref synchronized_task.
    using value_type = T;

    /// Required promise type for coroutines returning a @ref synchronized_task.
    /// Stores the value produced by the @ref synchronized_task, and a pointer
    /// to the “sync” object, a variable to notify after completion of the
    /// @ref synchronized_task.
    struct promise_type : private detail::noncopyable, task_promise_storage<T> {
        /// Pointer to the “sync” object to notify of our completion.
        Sync *sync = nullptr;

        /// Returns a @ref synchronized_task that references this promise.
        synchronized_task get_return_object() noexcept {
            TRACE_FUNC();
            return this;
        }

        /// Lazy: not started until @ref start() is invoked explicitly.
        static std::suspend_always initial_suspend() noexcept {
            TRACE_FUNC();
            return {};
        }

        /// Awaiter returned by @ref final_suspend.
        struct final_awaiter : std::suspend_always {
            void await_suspend(std::coroutine_handle<promise_type> this_coro) noexcept {
                TRACE_FUNC();
                this_coro.promise().sync->notify_awaitable_completed();
            }
        };

        /// When awaited, notifies the “sync” object (@ref sync) of this
        /// @ref synchronized_task's completion.
        static awaiter_of<void> auto final_suspend() noexcept {
            TRACE_FUNC();
            return final_awaiter {};
        }
    };

    synchronized_task(synchronized_task &&) = default;           ///< Move constructor
    synchronized_task &operator=(synchronized_task &&) = delete; ///< Move assignment not allowed.

    /// Start (resume) execution of the @ref synchronized_task.
    /// @param  s
    ///         Reference to the “sync” object, i.e. the variable to notify of
    ///         the %task's completion. When the %task is suspended at its final
    ///         suspension point, `s.notify_awaitable_completed()` is called.
    /// @note   `s.notify_awaitable_completed()` is called directly in
    ///         @ref promise_type::final_awaiter::await_suspend(), it does not
    ///         (and cannot) use Symmetric Control Transfer.
    void start(Sync &s) {
        promise_->sync = &s;
        std::coroutine_handle<promise_type>::from_promise(*promise_).resume();
    }

    /// Get the value produced by this %task.
    /// Must be called after the “sync” object passed to @ref start() has been
    /// notified.
    [[nodiscard]] decltype(auto) get() const & {
        TRACE_FUNC();
        return promise_->get();
    }
    /// @copydoc get()const&
    [[nodiscard]] decltype(auto) get() const && {
        TRACE_FUNC();
        return std::move(*promise_).get();
    }
    /// @copydoc get()const&
    [[nodiscard]] decltype(auto) nonvoid_get() const & {
        TRACE_FUNC();
        return promise_->nonvoid_get();
    }
    /// @copydoc get()const&
    [[nodiscard]] decltype(auto) nonvoid_get() const && {
        TRACE_FUNC();
        return std::move(*promise_).nonvoid_get();
    }

  private:
    /// An owning pointer to the promise object in the coroutine frame.
    /// When the task is destructed, this will cause the coroutine frame to be
    /// destroyed automatically.
    promise_ptr<promise_type> promise_;

    /// Private constructor used in @ref promise_type::get_return_object.
    synchronized_task(promise_type *promise) : promise_(promise) { TRACE_FUNC(); }
};

/// Coroutine returning an @ref synchronized_task that awaits the given 
/// awaitable and returns its result.
///
/// @relates synchronized_task
template <sync_notification_type Sync, awaitable A>
synchronized_task<Sync, remove_rvalue_reference_t<await_result_t<A>>>
make_synchronized_task(A &&awaitable) {
    TRACE_FUNC();
    co_return co_await std::forward<A>(awaitable);
}

} // namespace mp_coro::detail
