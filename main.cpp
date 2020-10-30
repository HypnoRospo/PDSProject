#include <iostream>
#include <thread>
#include <algorithm>
#include <condition_variable>
#include <boost/algorithm/string/find.hpp>
#include "Security.h"
#define OK_REGISTER "-SERVER: REGISTRAZIONE AVVENUTA CON SUCCESSO\r\n"
void getSomeData(Security& security);
void getSomeData_asyn(Security& security);
using namespace  std::chrono_literals;
std::mutex mutex;
boost::system::error_code ec;
void menu();
std::vector<char> vBuffer(1024); //big buffer , regulate the speed and costs
int main(int argc, char** argv) {

    std::cout << "Client Program" << std::endl;
    /*network programming trying */

    if( argc!=5 )
    {
        std::cout<<"Errore numero parametri linea di comando"<<std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        //boost::asio::ip::tcp::resolver resolver(io_context); ci servira' probabilmente

        boost::asio::io_context io_context;//create a context essentially the platform specific interface

        boost::asio::io_context::work idleWork(io_context); //fake tasks to asio
        std::thread thrContext = std::thread([&] () {io_context.run();}); //start context in background

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
        {   unsigned int scelta;
            std::string usr(argv[3]);
            std::string psw(argv[4]);
            Security security(usr,psw,socket);
            getSomeData_asyn(security);
            menu();
            while(true)
            {
                    std::cout <<"Inserire scelta: ";
                    std::cin >> scelta;
                switch(scelta)
                {
                    case 1:
                    {
                        mutex.try_lock();
                        security.register_user();
                        break;
                    }
                    case 2:
                    {
                        mutex.try_lock();
                        security.login();
                        break;
                    }
                    case 3:
                    {
                        std::string path_str;
                        std::cout<<"Inseire path per favore: ";
                        std::cin>> path_str;
                        security.getData(path_str);
                        break;
                    }
                    case 4:
                    {
                        security.logout();
                        menu();
                        break;
                    }

                    case 5:
                        exit(EXIT_SUCCESS);

                    default:
                        std::cout<<"Errore scelta, si prega di riprovare"<<std::endl;
                        break;
                }
                mutex.lock();
                sleep(1);
            }

        }
        io_context.stop();
        if(thrContext.joinable())
            thrContext.join();
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

/*
void getSomeData(Security& security)
{

    for (;;)
    {
        std::vector<char> buffer(128);
        boost::system::error_code error;

        size_t len = security.getSocket().read_some(boost::asio::buffer(buffer), error);
        if (error == boost::asio::error::eof)
            break; // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.

        if ( std::find(buffer.begin(), buffer.begin()+len, '\a') != buffer.begin()+len )
        {
            std::cout << "Utente gia' presente nel sistema, inserire un diverso username" <<std::endl;
            security.same_procedure(MsgType::REGISTER,true);
            break;
        }
        else if( std::find(buffer.begin(), buffer.begin()+len, '\b') != buffer.begin()+len)
        {
                std::cout << "Login fallito, username o password sbagliate, riprovare" <<std::endl;
                security.same_procedure(MsgType::LOGIN,true);
                break;
        }
      else
        std::cout.write(buffer.data(), len);

    }

    //ho ricevuto tutto -> spacchetto logicamente

    //todo
}

 */

void getSomeData_asyn(Security& security)
{


        security.getSocket().async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),
                               [&](std::error_code ec,std::size_t length)
                               {
                                   if(!ec)
                                   {
                                       /*
                                       std::cout <<"\n\n Read " <<length << " bytes\n\n";
                                       for( int i=0; i<length; i++)
                                       {
                                           std::cout << vBuffer[i];
                                       }
                                        */
                                       //logica applicativa

                                       std::string search(vBuffer.data(),vBuffer.size());

                                       std::string registrazione("REGISTRAZIONE AVVENUTA\r\n");
                                       if (search.find(registrazione)!=std::string::npos)
                                       {
                                           mutex.unlock();
                                       }

                                       std::string login("CLIENT LOGGEDr\n");
                                       if (search.find(login)!=std::string::npos)
                                       {
                                           mutex.unlock();
                                       }

                                       if ( search.find('\a')!=std::string::npos)
                                       {
                                           std::cout.write(vBuffer.data(), length);
                                           std::cout << "Utente gia' presente nel sistema, inserire un diverso username" <<std::endl;
                                           security.same_procedure(MsgType::REGISTER,true);
                                           mutex.unlock();
                                       }
                                       else if ( search.find('\b')!=std::string::npos)
                                       {
                                           std::cout << "Login fallito, username o password sbagliate, riprovare" <<std::endl;
                                           security.same_procedure(MsgType::LOGIN,true);
                                           mutex.unlock();
                                       }

                                        //stampa dei messaggi che arrivano
                                           std::cout.write(vBuffer.data(), length);
                                           vBuffer.clear();

                                           getSomeData_asyn(security); // isn't a real recursive but a system watching of network data.
                                   }
                                   else std::cout<<ec.message()<<std::endl;


                               }
        );
}


void menu()
{
    std::cout<<"###########################################"<<std::endl;
    std::cout<<"############CLIENT PROGRAM#################"<<std::endl;
    std::cout<<"###########################################"<<std::endl;

    std::cout<<"Per registrarsi premere [1]"<<std::endl;
    std::cout<<"Per loggarsi premere [2]"<<std::endl;
    std::cout<<"Per richiedere un file premere [3]"<<std::endl;
    std::cout<<"Per fare il logout [4]"<<std::endl;
    std::cout<<"Per registrarsi premere [5]"<<std::endl;
    std::cout<<"Per registrarsi premere [6]"<<std::endl;

    std::cout<<"###########################################"<<std::endl;
    std::cout<<"#################MENU######################"<<std::endl;
    std::cout<<"###########################################"<<std::endl;

}