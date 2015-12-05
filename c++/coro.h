#ifndef __CORO_H__
#define __CORO_H__

#include <iostream>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace coro
{

typedef boost::coroutines::symmetric_coroutine<void>::call_type call_type;
typedef boost::coroutines::symmetric_coroutine<void>::yield_type yield_type;

struct Coroutine;

namespace this_coroutine
{
    static Coroutine* current;
    static void jump(Coroutine*);
    void yield();
}


struct Coroutine
{
    call_type* ct;
    yield_type* yt;
    Coroutine* from;
    Coroutine* to;
    bool alive;
    std::string name;

    Coroutine(std::string n)
        : ct(NULL),
          yt(NULL),
          from(NULL),
          to(NULL),
          alive(true),
          name(n)
    {}

    ~Coroutine()
    {
        delete ct;

        if(to)
        {
            to->from = NULL;
        }
        std::cout << "[co] die " << name << std::endl;
    }

    void yield()
    {
        this_coroutine::current = from;
        if(!from)
        {
            std::cout << "[co] " << name << " yield to main" << std::endl;
            (*yt)();
        }
        else
        {
            std::cout << "[co] " << name << " yield back to " << from->name << std::endl;
            (*yt)(*(from->ct));
        }
    }

    void jump(Coroutine* other)
    {
        to = other;
        this_coroutine::current = other;
        if(this_coroutine::current)
        {
            // call in a coroutine
            std::cout << "[co] jump from " << this_coroutine::current->name << " to " << other->name << "std::endl";
            other->from = this;
            (*yt)(*(other->ct));
        }
        else
        {
            // call in main context
            std::cout << "[co] jump from main context to " << other->name << "std::endl";
            other->from = NULL;
            (*(other->ct))();
        }
    }
};


static Coroutine* spawn(std::function<void()> func, std::string name)
{
    std::cout << "spawn for " << name << std::endl;
    Coroutine* co = new Coroutine(name);

    auto func_wrapper = [co](yield_type& yield, std::function<void()> co_func)
    {
        co->yt = &yield;
        co->yield();

        // enter func
        co_func();

        co->alive = false;
        co->yield();
    };

    call_type* ct = new call_type(std::bind(func_wrapper, std::placeholders::_1, func));
    co->ct = ct;

    this_coroutine::jump(co);
    return co;
}

class Event
{
public:
    Event()
        : set_(false)
    {
        co = coro::spawn(std::bind(&Event::loop, this), std::string("evt"));
    }

    void set()
    {
        this_coroutine::jump(co);
    }

    void wait()
    {
        auto current = this_coroutine::current;
        if(!current)
        {
            std::cout << "ERROR, can not wait main context" << std::endl;
            exit(1);
        }

        if(!set_)
        {
            if(current->from == co)
            {
                // event jump to current, if call current->yield()
                // it will jump back to event
                // this is wrong.

                current->from = NULL;
            }

            wait_queue.push(current);
            std::cout << "[evt] wait yield. out\n";
            current->yield();
            std::cout << "[evt] wait yield. back\n";
        }
        set_ = false;
    }

private:
    void loop()
    {
        std::cout << "in Event loop\n";
        for(;;)
        {
            while(!wait_queue.empty())
            {
                auto w = wait_queue.front();
                wait_queue.pop();
                std::cout << "[evt] after set, jump to " << w->name << std::endl;
                this_coroutine::jump(w);
            }
            set_ = true;

            std::cout << "[evt] set all done" << std::endl;
            this_coroutine::yield();
        }
    }

    bool set_;
    std::queue<Coroutine*> wait_queue;
    Coroutine* co;
};


class Scheduler
{
public:
    Scheduler(boost::asio::io_service& ios)
        : io_(ios),
          evt_(Event())
    {
    }

    void run()
    {
        Coroutine* co = coro::spawn(std::bind(&Scheduler::loop, this), std::string("loop"));
        std::cout << "start loop\n";
        this_coroutine::jump(co);

        std::cout << "start io_service\n";
        io_.run();
    }

    void spawn(std::function<void()> func, std::string name)
    {
        auto co = coro::spawn(func, name);
        add_to_queue(co);
    }

    void add_to_queue(Coroutine* co)
    {
        coroutines.push(co);
        evt_.set();
        std::cout << "spawn & set done\n";
    }

private:
    void loop()
    {
        std::cout << "in hub loop" << std::endl;
        for(;;)
        {
            std::cout << "[loop] wait start\n";
            // co->call_from = NULL;
            evt_.wait();
            std::cout << "[loop] wait done\n";

            auto co = coroutines.front();
            std::cout << "co is NULL: " << (co == NULL) << std::endl;
            coroutines.pop();

            std::cout << "[loop] start to run " << co->name << std::endl;

            this_coroutine::jump(co);

            std::cout << "[loop] back from " << co->name << std::endl;

            if(co->alive)
            {
                std::cout << co->name << " still alive" << std::endl;
                add_to_queue(co);
            }
            else
            {
                delete co;
            }
        }
    }

    boost::asio::io_service& io_;
    std::queue<Coroutine*> coroutines;
    Event evt_;
};



void this_coroutine::jump(Coroutine* other)
{
    auto current = this_coroutine::current;
    this_coroutine::current = other;

    if(current)
    {
        // call in a coroutine
        std::cout << "[this_coroutine] jump from " << current->name << " to " << other->name << std::endl;
        other->from = current;
        (*(current->yt))(*(other->ct));
    }
    else
    {
        // call in main context
        std::cout << "[this_coroutine] jump from main context to " << other->name << std::endl;
        other->from = NULL;
        (*(other->ct))();
    }
}

void this_coroutine::yield()
{
    if(!this_coroutine::current)
    {
        std::cout << "ERROR can not yield from main context" << std::endl;
        exit(1);
    }

    this_coroutine::current->yield();
}

}

#endif
