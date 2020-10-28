#include <iostream>
#include <thread>
#include "Security.h"
#include "Message.h"

void getSomeData(boost::asio::ip::tcp::socket& socket);

boost::system::error_code ec;

int main(int argc, char** argv) {

    std::cout << "Client Program" << std::endl;
    /*network programming trying */

    if( argc!=5 )
    {
        std::cout<<"Errore numero parametri linea di comando"<<std::endl;
        exit(EXIT_FAILURE);
    }

    try {

        boost::asio::io_context io_context;
        //boost::asio::ip::tcp::resolver resolver(io_context); ci servira' probabilmente

        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(argv[1],ec),atoi(argv[2]));
        boost::asio::ip::tcp::socket  socket(io_context); //the context will deliver the implementation
        socket.connect(endpoint,ec);

        if(!ec)
        {
            std::cout <<"Connected to the Server" << std::endl;
        }
        else
        {
            std::cout << "Failed to connect to address:\n" << ec.message() <<std::endl;
        }

        if(socket.is_open())
        {

            std::string usr("andrea");
            std::string psw("pdsproject");
            //possiamo chiedere di passarli tramite linea di comando..cin

            Security security(usr,psw,socket);
            security.register_user();
            security.login();
            std::string path = "../files/prova";
            security.getData(path);

            socket.wait(boost::asio::ip::tcp::socket::wait_read);

            security.logout();

            getSomeData(socket);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}

/*
void Send(const Message::message<MsgType>& msg)
{
    boost::asio::post(context,
               [msg]()
               {

               });
}
 */

void getSomeData(boost::asio::ip::tcp::socket& socket)
{

    for (;;)
    {
        boost::array<char, 128> buffer;
        boost::system::error_code error;

        size_t len = socket.read_some(boost::asio::buffer(buffer), error);
        if (error == boost::asio::error::eof)
            break; // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.

        std::cout.write(buffer.data(), len);
    }

    //ho ricevuto tutto -> spacchetto logicamente

    //todo
}