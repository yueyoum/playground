#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include "coro.h"


void one()
{
    std::cout << "[one] 1" << std::endl;
    coro::this_coroutine::yield();
    std::cout << "[one] 2" << std::endl;
}

void two()
{
    std::cout << "[two] 1" << std::endl;
    coro::this_coroutine::yield();
    std::cout << "[two] 2" << std::endl;
}

void three()
{
    std::cout << "[three] 1" << std::endl;
    return;
}

int main()
{
    std::cout << "start" << std::endl;
    boost::asio::io_service io;
    boost::asio::io_service::work w(io);

    coro::Scheduler sche(io);

    sche.spawn(one, std::string("one"));


    boost::asio::deadline_timer timter(io, boost::posix_time::seconds(3));
    timter.async_wait(
            [&sche](const boost::system::error_code&)
            {
                sche.spawn(two, std::string("two"));
            }
            );

    boost::asio::deadline_timer timter1(io, boost::posix_time::seconds(6));
    timter1.async_wait(
            [&sche](const boost::system::error_code&)
            {
                sche.spawn(three, std::string("three"));
            }
            );

    sche.run();
    std::cout << "end" << std::endl;
}

