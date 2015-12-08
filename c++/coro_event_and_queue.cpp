#include <iostream>
#include <functional>
#include <boost/asio.hpp>

#include "coro.h"

void one(coro::Event& evt)
{
    std::cout << "one start" << std::endl;
    evt.wait();
    std::cout << "one end" << std::endl;
}

void two(coro::Event& evt)
{
    std::cout << "two start" << std::endl;
    evt.wait();
    std::cout << "two end" << std::endl;
}

void three(coro::Event& evt)
{
    std::cout << "three start" << std::endl;
    for(int i=0; i<5; i++)
    {
        coro::this_coroutine::sleep_for(1);
        std::cout << "three: " << i << std::endl;
    }

    evt.set();

    std::cout << "three end" << std::endl;
}


int main()
{
    boost::asio::io_service io;
    boost::asio::io_service::work w(io);

    coro::Event evt;
    auto sche = coro::Scheduler::create(io);

    sche->spawn(std::bind(one, std::ref(evt)), std::string("one"));
    sche->spawn(std::bind(two, std::ref(evt)), std::string("two"));
    sche->spawn(std::bind(three, std::ref(evt)), std::string("three"));


    coro::Queue<int> queue;
    auto consumer = [&queue]()
        {
            for(;;)
            {
                auto value = queue.get();
                std::cout << "get value: " << value << std::endl;

                if (value == 5)
                {
                    break;
                }
            }
        };

    auto producer = [&queue]()
        {
            int data[] = {1, 2, 3, 4, 5};
            for(auto i: data)
            {
                queue.put(i);
                coro::this_coroutine::sleep_for(1);
            }
        };

    boost::asio::deadline_timer timer(io, boost::posix_time::seconds(6));
    timer.async_wait(
            [sche, consumer, producer](const boost::system::error_code&)
            {
                sche->spawn(consumer, std::string("consumer"));
                sche->spawn(producer, std::string("producer"));
            }
    );


    sche->run();
    io.run();

    delete sche;
    return 0;
}


