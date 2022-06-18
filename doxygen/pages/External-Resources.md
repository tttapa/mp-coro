# External resources {#ext-resources}

## Main documentation

- cppreference.com: [Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)

## C++ Standard

- p.128: [Await [expr.await]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/n4868.pdf#page.128)
- p.145: [Yielding a value [expr.yield]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/n4868.pdf#page.145)
- p.161: [The `co_return` statement [stmt.return.coroutine]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/n4868.pdf#page.161)
- p.217: [Coroutine definitions [dcl.fct.def.coroutine]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/n4868.pdf#page.217)
- p.546: [Coroutines (standard library) [support.coroutine]](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/n4868.pdf#page.546)

## Symmetric transfer of control

- Blog post: [Lewis Baker ─ C++ Coroutines: Understanding Symmetric Transfer](https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer)
- Proposal: [P0913R0: Add symmetric coroutine control transfer](https://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p0913r0.html)

## Halo: coroutine Heap Allocation eLision Optimization: the joint response

- Proposal: [P0981R0: Halo: coroutine Heap Allocation eLision Optimization: the joint response](https://www.open-std.org/JTC1/SC22/WG21/docs/papers/2018/p0981r0.html)

> We DO NOT need to inline the body of the coroutine or the algorithm 
> `accumulate`, unless the iterator type retains a pointer or reference to the
> `generator<int>` object.
>
> Note that authors of coroutine types should be aware of this point: at least
> one blogger has described an implementation of `generator<T>::iterator` that
> holds a `generator<T>*`. However, every implementation of `generator<T>` we
> can find in the wild holds a `coroutine_handle<Promise>` instead, which avoids
> the problem, and allows heap allocation elision without inlining accumulate.