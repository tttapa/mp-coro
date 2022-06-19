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

#include <mp-coro/bits/synchronized_task.h>
#include <mp-coro/concepts.h>
#include <mp-coro/trace.h>
#include <semaphore>

namespace mp_coro {

/// Creates a [synchronized task](@ref detail::make_synchronized_task) from the
/// awaitable, starts it, and waits for it to complete, returning the result.
/// Uses a `std::binary_semaphore` for waiting and synchronization.
template <awaitable A>
[[nodiscard]] decltype(auto) sync_await(A &&awaitable) {
    struct sync {
        std::binary_semaphore sem {0};
        void notify_awaitable_completed() { sem.release(); }
    };

    TRACE_FUNC();
    auto sync_task = detail::make_synchronized_task<sync>(std::forward<A>(awaitable));
    sync work_done;
    sync_task.start(work_done);
    work_done.sem.acquire();
    return sync_task.get();
}

} // namespace mp_coro
