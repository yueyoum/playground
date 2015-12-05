#include <iostream>
#include <boost/coroutine/symmetric_coroutine.hpp>

typedef boost::coroutines::symmetric_coroutine<void> coro_t;

coro_t::call_type* a, * b, *z;

void xx(coro_t::yield_type&);
void yy(coro_t::yield_type&);
void zz(coro_t::yield_type&);


void xx(coro_t::yield_type& yield)
{
    std::cout << "xx 1" << std::endl;
    b = new coro_t::call_type(yy);
    (*b)();
    std::cout << "xx 2" << std::endl;
    yield(*b);
    std::cout << "xx 3" << std::endl;
    yield();
}

void yy(coro_t::yield_type& yield)
{
    std::cout << "yy 1" << std::endl;
    yield();
    std::cout << "yy 2" << std::endl;
    z = new coro_t::call_type(zz);
    yield(*z);
    std::cout << "yy 3" << std::endl;
    yield();
}

void zz(coro_t::yield_type& yield)
{
    std::cout << "zz 1" << std::endl;
    yield(*b);
    std::cout << "zz 2" << std::endl;
    yield();
}


int main()
{
    std::cout << "main 1" << std::endl;
    a = new coro_t::call_type(xx);
    (*a)();
    std::cout << "main 2" << std::endl;
    (*a)();
    std::cout << "main 3" << std::endl;
}


// output
// main 1
// xx 1
// yy 1
// xx 2
// yy 2
// zz 1
// yy 3
// main 2
// xx3
// main 3
