#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include "coro_echo_server.h"


void connection_handler(Client client)
{
    std::cout << "new client" << std::endl;

//    Client remote = Endpoint::connect(client->io_service(), std::string("127.0.0.1"), 8008);
    for(;;)
    {
        std::string data = client->recv(1024);
        if(data.empty())
        {
            std::cout << "client connection lost" << std::endl;
            break;
        }

        std::cout << "client got: " << data << std::endl;

//        remote->send(data);
//
//        data = remote->recv(1024);
//        if(data.empty())
//        {
//            std::cout << "remote connection lost" << std::endl;
//            break;
//        }
//
//        std::cout << "remote got: " << data << std::endl;

        client->send(data);
    }
}


int main()
{
    boost::asio::io_service io;
    boost::asio::io_service::work w(io);

    Server s(io, 9090, connection_handler);
    s.run();

    return 0;
}
