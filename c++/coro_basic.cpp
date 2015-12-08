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
    coro::this_coroutine::yield();
    std::cout << "[three] 2" << std::endl;
}

void four()
{
    std::cout << "[four] 1" << std::endl;
    coro::this_coroutine::yield();
    std::cout << "[four] 2" << std::endl;
    coro::this_coroutine::yield();
    std::cout << "[four] 3" << std::endl;
}

int main()
{
    std::cout << "start" << std::endl;
    boost::asio::io_service io;
    boost::asio::io_service::work w(io);

    auto sche = coro::Scheduler::create(io);

    sche->spawn(one, std::string("one"));

    boost::asio::deadline_timer timter(io, boost::posix_time::seconds(3));
    timter.async_wait(
            [&sche](const boost::system::error_code&)
            {
                sche->spawn(two, std::string("two"));
                sche->spawn(three, std::string("three"));
            }
            );

    boost::asio::deadline_timer timter1(io, boost::posix_time::seconds(6));
    timter1.async_wait(
            [&sche](const boost::system::error_code&)
            {
                sche->spawn(four, std::string("four"));
            }
            );

    sche->run();

    io.run();

    delete sche;
    std::cout << "end" << std::endl;
}

