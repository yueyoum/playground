#ifndef __CORO_H__
#define __CORO_H__

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace coro
{

typedef boost::coroutines::symmetric_coroutine<void>::call_type call_type;
typedef boost::coroutines::symmetric_coroutine<void>::yield_type yield_type;

struct Coroutine;
class Event;
class Scheduler;

namespace this_coroutine
{
    static Coroutine* current = NULL;
    static void jump(Coroutine*);

    void get();
    void yield();
    void wait();
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
        std::cout << "[co] jump from " << name << " to " << other->name << std::endl;
        other->from = this;
        (*yt)(*(other->ct));
    }

    void resume()
    {
        this_coroutine::current = this;
        (*ct)();
    }
};


Coroutine* spawn(std::function<void()> func, std::string name)
{
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
        : set_(false),
          process_set_(false)
    {
        co = coro::spawn(std::bind(&Event::loop, this), std::string("evt"));
    }

    void set()
    {
        if(process_set_)
        {
            std::cout << "Event can not set!" << std::endl;
            exit(1);
        }
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
            wait_queue_.push(current);
            current->yield();
        }
        set_ = false;
    }

private:
    void loop()
    {
        for(;;)
        {
            process_set_ = true;

            // assume there are A and B coroutines wait to be notifed.
            // the correct process is:
            // 1) event jump to A
            // 2) A call `this_coroutine::yield()` or `this_coroutine::wait()`
            //    to yield back to event
            // 3) due to the while loop, event then jump to B
            // 4) same as A, B will yield back to event.
            // 5) event yield out, waiting next set.
            //
            // But think about this situation:
            // 1) event jump to A
            // 2) A call `evt.set()`
            // 3) the event loop will forward one step.
            //    means that B will be notified
            // 4) B call yield to yield back to event,
            //    this set operation is complete.
            //    and return to step 2).
            // 5) A continue to execute, if A call yield,
            //    this will let event re-enter loop,
            //    the effect is same as the `evt.set()` call.
            //
            // This behavior is absolute wrong.
            // So the `process_set_` flag is used to prevent the above wrong operation.
            //
            //
            // Need a process_queue_, reason:
            // if no process_queue_, use the wait_queue_,
            // when jump to A coroutine, and A call `evt.wait()`
            // the event loop will got the A again, and jump to A
            // this will lead to infinite loop

            while(!wait_queue_.empty())
            {
                process_queue_.push(wait_queue_.front());
                wait_queue_.pop();
            }

            while(!process_queue_.empty())
            {
                auto w = process_queue_.front();
                process_queue_.pop();
                this_coroutine::jump(w);
            }
            process_set_ = false;
            set_ = true;

            this_coroutine::yield();
        }
    }

    bool set_;
    bool process_set_;
    std::queue<Coroutine*> wait_queue_;
    std::queue<Coroutine*> process_queue_;
    Coroutine* co;
};


class Scheduler
{
public:
    static Scheduler* get()
    {
        if(!instance_)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if(!instance_)
            {
                instance_ = new Scheduler();
            }
        }

        return instance_;
    }

    Event& event()
    {
        return evt_;
    }

    void run()
    {
        Coroutine* co = coro::spawn(std::bind(&Scheduler::loop, this), std::string("loop"));
        this_coroutine::jump(co);
    }

    void spawn(std::function<void()> func, std::string name)
    {
        auto co = coro::spawn(func, name);
        add_to_queue(co);
    }


private:
    Scheduler()
        : evt_(Event())
    {
    }

    void loop()
    {
        for(;;)
        {
            while(coroutines.empty())
            {
                std::cout << "[loop] wait start\n";
                this_coroutine::wait();
                std::cout << "[loop] wait done\n";
            }

            auto co = coroutines.front();
            coroutines.pop();

            std::cout << "[loop] start to run " << co->name << std::endl;
            this_coroutine::jump(co);
            std::cout << "[loop] back from " << co->name << std::endl;

            if(co->alive)
            {
                coroutines.push(co);
            }
            else
            {
                delete co;
            }
        }
    }

    void add_to_queue(Coroutine* co)
    {
        coroutines.push(co);
        evt_.set();
    }

    static std::mutex mutex_;
    static Scheduler* instance_;

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
        current->jump(other);
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

void this_coroutine::wait()
{
    if(!this_coroutine::current)
    {
        std::cout << "ERROR can not wait in main context" << std::endl;
        exit(1);
    }

    Scheduler::get()->event().wait();
}


std::mutex Scheduler::mutex_;
Scheduler* Scheduler::instance_ = NULL;

}

#endif
