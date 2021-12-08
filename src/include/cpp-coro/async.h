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

#include <cpp-coro/bits/storage.h>
#include <cpp-coro/trace.h>
#include <concepts>
#include <coroutine>
#include <thread>

namespace mp_coro {

template<std::invocable Func>
class async {
public:
  using return_type = std::invoke_result_t<Func>;
  explicit async(Func func): func_{std::move(func)} {}
  static bool await_ready() noexcept { TRACE_FUNC(); return false; }
  void await_suspend(std::coroutine_handle<> handle)
  {
    auto work = [&, handle]() {
      TRACE_FUNC();
      try {
        if constexpr(std::is_void_v<return_type>)
          func_();
        else
          result_.set_value(func_());
      }
      catch(...) {
        result_.set_exception(std::current_exception());
      }
      handle.resume();
    };

    TRACE_FUNC();
    std::jthread(work).detach();  // TODO: Fix that (replace with a thread pool)
  }
  decltype(auto) await_resume() { TRACE_FUNC(); return std::move(result_).get(); }
private:
  Func func_;
  detail::storage<return_type> result_;
};

} // namespace mp_coro
