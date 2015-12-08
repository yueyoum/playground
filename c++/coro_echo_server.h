#ifndef __CORO_ECHO_SERVER_H__
#define __CORO_ECHO_SERVER_H__

#include <iostream>
#include <string>
#include <array>
#include <boost/asio.hpp>

#include "coro.h"

using boost::asio::ip::tcp;

class Connection: public std::enable_shared_from_this<Connection>
{
public:
    Connection(tcp::socket&& socket):
        socket_(std::move(socket))
    {}

    std::string recv(const std::size_t& size)
    {
        std::array<char, 1024> buf;

        auto current = coro::this_coroutine::detail::current;

        socket_.async_read_some(
                boost::asio::buffer(buf),
                [current](const boost::system::error_code& error, std::size_t)
                {
                    if(error)
                    {
                        std::cout << "recv error: " << error << std::endl;
                    }
                    else
                    {
                        coro::this_coroutine::detail::jump(current);
                    }
                }
                );

        coro::this_coroutine::suspend();
        return std::string(buf.data());
    }

    void send(std::string data)
    {
        auto current = coro::this_coroutine::detail::current;
        boost::asio::async_write(
                socket_,
                boost::asio::buffer(data),
                [current](const boost::system::error_code& error, std::size_t)
                {
                    if(error)
                    {
                        std::cout << "send error: " << error << std::endl;
                    }
                    else
                    {
                        coro::this_coroutine::detail::jump(current);
                    }
                }
                );

        coro::this_coroutine::suspend();
    }


private:
    tcp::socket socket_;
};


typedef std::shared_ptr<Connection> Client;


class Server
{
public:
    Server(boost::asio::io_service& io, int port, std::function<void(Client)> callback)
        : io_(io),
          acceptor_(io, tcp::endpoint(tcp::v4(), port)),
          accept_callback_(callback)
    {
        sche_ = coro::Scheduler::create(io_);
    }

    void run()
    {
        sche_->spawn(std::bind(&Server::accept_loop, this), std::string("accept_loop"));

        sche_->run();
        io_.run();
    }

private:
    void accept_loop()
    {
        auto current = coro::this_coroutine::detail::current;
        for(;;)
        {
            tcp::socket socket(io_);
            acceptor_.async_accept(
                    socket,
                    [current](const boost::system::error_code& error)
                    {
                        if(error)
                        {
                            std::cout << "accept error: " << error << std::endl;
                            exit(1);
                        }
                        else
                        {
                            coro::this_coroutine::detail::jump(current);
                        }
                    }
                    );

            coro::this_coroutine::suspend();

            Client client = std::make_shared<Connection>(std::move(socket));
            std::function<void()> handler = std::bind(accept_callback_, client);
            sche_->spawn(handler, std::string("client"));
        }
    }

    boost::asio::io_service& io_;
    tcp::acceptor acceptor_;
    std::function<void(Client)> accept_callback_;
    coro::Scheduler* sche_;
};


#endif // __CORO_ECHO_SERVER_H__

