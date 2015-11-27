#include <iostream>
#include <deque>
#include <string>
#include <array>
#include <unordered_map>
#include <boost/context/all.hpp>

boost::context::fcontext_t hub;
class Greenlet;

std::unordered_map<int, Greenlet*> greenlets_map;

class Greenlet
{
public:
    Greenlet(void (*fn)(intptr_t), std::string arg)
        : fn_(fn),
          ctx_(boost::context::make_fcontext(stack_.data()+stack_.size(), stack_.size(), fn_)),
          arg_(std::move(arg)),
          finish_(false)
    {
        id_ = reinterpret_cast<int>(this);
        greenlets_map[id_] = this;
        printf("[CORO %d]<%s> spawned\n", id_, arg_.data());
    }

    ~Greenlet()
    {
        printf("[CORO %d]<%s> finish\n", id_, arg_.data());
        auto iter = greenlets_map.find(id_);
        greenlets_map.erase(iter);
    }

    void operator()()
    {
        boost::context::jump_fcontext(&hub, ctx_, static_cast<intptr_t>(id_));
    }

    void yield(bool isfinish = false)
    {
        finish_ = isfinish;
        boost::context::jump_fcontext(&ctx_, hub, 0);
    }

    std::string& arg()
    {
        return arg_;
    }

    int& id()
    {
        return id_;
    }

    bool& finish()
    {
        return finish_;
    }


private:
    void (*fn_)(intptr_t);
    std::array<intptr_t, 4096> stack_;
    boost::context::fcontext_t ctx_;
    std::string arg_;
    int id_;
    bool finish_;
};



class Scheduler
{
public:
    Scheduler()
    {
    }

    void spawn(void (*fn)(intptr_t), std::string arg)
    {
        Greenlet* gl = new Greenlet(fn, std::move(arg));
        greenlets.push_back(gl);
    }

    void operator()()
    {
        while(!greenlets.empty())
        {
            Greenlet* gl = greenlets.front();
            greenlets.pop_front();

            printf("[MAIN] run coroutine<%s>\n", gl->arg().data());
            (*gl)();

            if(gl->finish())
            {
                delete gl;
            }
            else
            {
                greenlets.push_back(gl);
            }
        }

        printf("[MAIN] no more coroutines, exit\n");
    }

private:
    std::deque<Greenlet*> greenlets;
};

Scheduler sche;

void fxx(intptr_t);

void ff(intptr_t arg)
{
    int id = static_cast<int>(arg);
    auto gl = greenlets_map[id];

    printf("[CORO %d] <%s> enter ff\n", id, gl->arg().data());
    gl->yield();
    printf("[CORO %d] <%s> re-enter ff\n", id, gl->arg().data());
    sche.spawn(fxx, std::string("three"));
    gl->yield(true);
};


void fxx(intptr_t arg)
{
    int id = static_cast<int>(arg);
    auto gl = greenlets_map[id];
    printf("[CORO %d] <%s> enter fxx\n", id, gl->arg().data());
    gl->yield();
    printf("[CORO %d] <%s> re-enter fxx\n", id, gl->arg().data());
    sche.spawn(ff, std::string("four"));
    gl->yield(true);
};



int main()
{
    sche.spawn(ff, std::string("one"));
    sche.spawn(fxx, std::string("two"));
    sche();
    printf("[MAIN] done\n");

    return 0;
}
