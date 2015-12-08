#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include "coro_echo_server.h"

void connection_handler(Client client)
{
    std::cout << "new client" << std::endl;
    for(;;)
    {
        std::string data = client->recv(1024);
        std::cout << "GOT DATA: " << data << std::endl;

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
