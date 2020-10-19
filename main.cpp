#include <iostream>
#include <chrono>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/array.hpp>
#include <thread>
#include "Message.h"
using namespace  std::chrono_literals;
std::vector<char> vBuffer(1*1024); //big buffer , regulate the speed and costs


void getSomeData(boost::asio::ip::tcp::socket& socket);
boost::system::error_code ec;
boost::asio::io_context context;//create a context essentially the platform specific interface

int main() {
    std::cout << "Client Program" << std::endl;

    /*network programming trying */

    try {
        boost::asio::io_context::work idleWork(context); //fake tasks to asio so the context doesnt finish
        std::thread thrContext = std::thread([&] () {context.run();}); //start context in background

        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1",ec),5000);
        boost::asio::ip::tcp::socket  socket(context); //the context will deliver the implementation
        socket.connect(endpoint,ec);
        if(!ec)
        {
            std::cout <<"Connected " << std::endl;
        }
        else
        {
            std::cout << "Failed to connect to address:\n" << ec.message() <<std::endl;
        }

        /*
        if(socket.is_open())
        {


            Message::message<MsgType> mex;
            mex.header.id=MsgType::GET;
            mex << "GET prova\r\n";

            boost::system::error_code ignored_error;
            boost::asio::write(socket, boost::asio::buffer(&mex.header, sizeof(mex.header)), ignored_error);
            socket.wait(boost::asio::ip::tcp::socket::wait_read);

            boost::asio::write(socket, boost::asio::buffer(mex.body.data(), sizeof(mex.size())), ignored_error);
            socket.wait(boost::asio::ip::tcp::socket::wait_read);
            std::string message_2 = "FINE\r\n";

            boost::asio::write(socket, boost::asio::buffer(message_2), ignored_error);
            socket.wait(boost::asio::ip::tcp::socket::wait_read);
            getSomeData(socket);

        }
         */

        context.stop();
        if(thrContext.joinable())
            thrContext.join();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

void Send(const Message::message<MsgType>& msg)
{
    boost::asio::post(context,
               [msg]()
               {

               });
}

void getSomeData(boost::asio::ip::tcp::socket& socket)
{

    for (;;)
    {
        boost::array<char, 128> buf;
        boost::system::error_code error;

        size_t len = socket.read_some(boost::asio::buffer(buf), error);
        if (error == boost::asio::error::eof)
            break; // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.

        //std::cout.write(buf.data(), len);
    }

    //ho ricevuto tutto -> spacchetto logicamente

    //todo
}