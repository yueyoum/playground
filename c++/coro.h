#ifndef __CORO_H__
#define __CORO_H__

#include <iostream>
#include <string>
#include <set>
#include <queue>
#include <mutex>
#include <boost/asio.hpp>
#include <boost/coroutine/symmetric_coroutine.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace coro
{

typedef boost::coroutines::symmetric_coroutine<void>::call_type call_type;
typedef boost::coroutines::symmetric_coroutine<void>::yield_type yield_type;

class Coroutine;
class Event;
class Scheduler;
class Timer;
static Coroutine* spawn(std::function<void()>, std::string);

static boost::asio::io_service* io;
void io_service(boost::asio::io_service& ios)
{
    io = &ios;
}

boost::asio::io_service& io_service()
{
    return *io;
}

namespace this_coroutine
{
    static Coroutine* current = NULL;

    static void jump(Coroutine*);

    Coroutine* get();
    // yield: return to scheduler
    //        and will re-enter by scheduler
    void yield();
    // suspend: give up the execution passive,
    //       due to some block call (e.g, IO, Event, Queue...)
    //       or switch to other coroutines and wait back
    //       this coroutine will be resume when that block call returns
    void suspend();
    std::shared_ptr<Timer> sleep_for(int);
}

class Coroutine
{
public:
    call_type* ct;
    yield_type* yt;
    Coroutine* from;
    Coroutine* to;
    std::string name;
    bool active;
    bool yield_back;

    Coroutine(std::string n)
        : ct(NULL),
          yt(NULL),
          from(NULL),
          to(NULL),
          name(n),
          active(true),
          yield_back(false)
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

    void add_link(Coroutine*)
    {
    }

    void remove_link(Coroutine*)
    {
    }

    void yield()
    {
        yield_back = true;
        context_switch(from);
    }

    void suspend()
    {
        context_switch(from);
    }

    void jump(Coroutine* target)
    {
        target->from = this;
        to = target;

        context_switch(target);
    }

    // void resume()
    // {
    //     this_coroutine::current = this;
    //     (*ct)();
    // }

private:
    void context_switch(Coroutine* target)
    {
        this_coroutine::current = target;
        if(target)
        {
            target->yield_back = false;
            std::cout << "[co] " << name << " switch to " << target->name << std::endl;
            (*yt)(*(target->ct));
        }
        else
        {
            std::cout << "[co] " << name << " return to main context" << std::endl;
            (*yt)();
        }
    }

    std::vector<Coroutine*> links;
};



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
            current->suspend();
        }
        set_ = false;
    }

    std::size_t size() const
    {
        return wait_queue_.size();
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

            this_coroutine::suspend();
        }
    }

    bool set_;
    bool process_set_;
    std::queue<Coroutine*> wait_queue_;
    std::queue<Coroutine*> process_queue_;
    Coroutine* co;
};


template<class T>
class Queue
{
public:
    Queue() {}

    std::size_t size() const
    {
        return q_.size();
    }

    void put(T& value)
    {
        std::cout << "[queue] put" << std::endl;
        q_.push(value);
        if(co_get_)
        {
            this_coroutine::jump(co_get_);
        }
    }

    T& get()
    {
        auto current = this_coroutine::current;
        if(!current)
        {
            std::cout << "ERROR, can not get queue in main context" << std::endl;
            exit(1);
        }

        if(co_get_)
        {
            std::cout << "ERROR, another coroutine is get the queue" << std::endl;
            exit(1);
        }

        std::cout << "[queue] co " << current->name << " start get" << std::endl;
        if(q_.empty())
        {
            co_get_ = current;
            current->suspend();
        }

        std::cout << "[queue] co " << current->name << " end get" << std::endl;

        auto value = q_.front();
        q_.pop();
        co_get_ = NULL;
        return std::ref(value);
    }


private:
    std::queue<T> q_;
    Coroutine* co_get_;
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

    void run()
    {
        co_loop_ = coro::spawn(std::bind(&Scheduler::loop, this), std::string("loop"));
        this_coroutine::jump(co_loop_);
        coroutines.insert(co_loop_);
    }

    void spawn(std::function<void()> func, std::string name)
    {
        auto co = coro::spawn(func, name);
        coroutines.insert(co);
        queue_.put(co);
    }

    void kill(Coroutine* co)
    {
        coroutines.erase(co);
        // delete co;
    }

    std::size_t size() const
    {
        return coroutines.size();
    }

private:
    Scheduler()
        : queue_(Queue<Coroutine*>())
    {
    }

    void loop()
    {
        for(;;)
        {
            std::cout << "[loop] start get" << std::endl;
            auto co = queue_.get();
            std::cout << "[loop] got: " << co->name << std::endl;

            std::cout << "[loop] start to run " << co->name << std::endl;
            this_coroutine::jump(co);
            std::cout << "[loop] back from " << co->name << std::endl;

            if(co->active)
            {
                if(co->yield_back)
                {
                    queue_.put(co);
                }
            }
            else
            {
                kill(co);
            }
        }
    }

    static std::mutex mutex_;
    static Scheduler* instance_;

    std::set<Coroutine*> coroutines;
    Queue<Coroutine*> queue_;
    Coroutine* co_loop_;
};

class Timer
{
public:
    Timer(int seconds)
        : t_(io_service())
    {
        t_.expires_from_now(boost::posix_time::seconds(seconds));
        co = this_coroutine::current;

        t_.async_wait(std::bind(&Timer::handler, this, std::placeholders::_1));
        co->suspend();
    }

    void cancel()
    {
    }

private:
    void handler(const boost::system::error_code& error)
    {
        if(error)
        {
            std::cout << "Timer error: " << error << std::endl;
        }
        else
        {
            this_coroutine::jump(co);
        }
    }

    boost::asio::deadline_timer t_;
    Coroutine* co;
};


static Coroutine* spawn(std::function<void()> func, std::string name)
{
    Coroutine* co = new Coroutine(name);

    auto func_wrapper = [co](yield_type& yield, std::function<void()> co_func)
    {
        co->yt = &yield;
        co->suspend();

        // enter func
        co_func();

        co->active = false;
        auto from = co->from;

        Scheduler::get()->kill(co);
        this_coroutine::current = NULL;
        if(from)
        {
            this_coroutine::jump(from);
        }
    };

    call_type* ct = new call_type(std::bind(func_wrapper, std::placeholders::_1, func));
    co->ct = ct;

    this_coroutine::jump(co);
    return co;
}


Coroutine* this_coroutine::get()
{
    return this_coroutine::current;
}

void this_coroutine::jump(Coroutine* other)
{
    if(this_coroutine::current)
    {
        // call in a coroutine
        current->jump(other);
    }
    else
    {
        // call in main context
        std::cout << "[main] jump from main context to " << other->name << std::endl;
        other->from = NULL;
        this_coroutine::current = other;
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

void this_coroutine::suspend()
{
    if(!this_coroutine::current)
    {
        std::cout << "ERROR can not wait in main context" << std::endl;
        exit(1);
    }

    this_coroutine::current->suspend();
}

std::shared_ptr<Timer> this_coroutine::sleep_for(int seconds)
{
    auto current = this_coroutine::current;
    if(!current)
    {
        std::cout << "ERROR can not sleep_for in main context" << std::endl;
        exit(1);
    }

    return std::make_shared<Timer>(seconds);
}


std::mutex Scheduler::mutex_;
Scheduler* Scheduler::instance_ = NULL;

}

#endif
