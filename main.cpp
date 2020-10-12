#include <iostream>
#include <chrono>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <thread>

using namespace  std::chrono_literals;
std::vector<char> vBuffer(1*1024); //big buffer , regulate the speed and costs


void getSomeData(boost::asio::ip::tcp::socket& socket);


int main() {
    std::cout << "Client Program" << std::endl;

    /*network programming trying */

    boost::system::error_code ec;
    boost::asio::io_context context;//create a context essentially the platform specific interface
    boost::asio::io_context::work idleWork(context); //fake tasks to asio so the context doesnt finish
    std::thread thrContext = std::thread([&] () {context.run();}); //start context in background

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1",ec),5001);
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

    if(socket.is_open())
    {
        getSomeData(socket);
        std::string string_Request = "GET prova\r\n";
        socket.write_some(boost::asio::buffer(string_Request.data(),string_Request.size()),ec);
        std::string string_Request_2 = "FINE\r\n";
        socket.write_some(boost::asio::buffer(string_Request_2.data(),string_Request_2.size()),ec);
        //diamo il tempo al server di rispondere

        //codice SINCRONO. A NOI INTERESSA ASINCRONO

        std::this_thread::sleep_for(2000ms);
        //size_t bytes=socket.available();
        //meglio usare wait

        //but wait function isnt perfect -> switch to async

        //std::cout<<"Bytes disponibili: " <<bytes << std::endl;

       /*
        if(bytes > 0)
        {
            std::vector<char> v_buffer(bytes);
            socket.read_some(boost::asio::buffer(v_buffer.data(),v_buffer.size()),ec);
            for (auto c: v_buffer)
            {
                std::cout<< c;
            }

        }
       // std::string string_Request = "GET prova\r\n";
        //socket.write_some(boost::asio::buffer(string_Request.data(),string_Request.size()),ec);
        */

    }
    context.stop();
   if(thrContext.joinable())
       thrContext.join();
    return 0;
}


void getSomeData(boost::asio::ip::tcp::socket& socket)
{

    socket.async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),
                           [&](std::error_code ec,std::size_t length)
                           {
                               if(!ec)
                               {
                                   std::cout <<"\n\n Read " <<length << " bytes\n\n";
                                   for( int i=0; i<length; i++)
                                   {
                                       std::cout << vBuffer[i];
                                   }
                                   getSomeData(socket); // isn't a real recursive but a system watching of network data.
                               }
                           }
    );
}