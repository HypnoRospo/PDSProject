#include <iostream>
#include <thread>
#include <algorithm>
#include <condition_variable>
#include <boost/algorithm/string/find.hpp>
#include <boost/filesystem/operations.hpp>
#include <set>
#include <fstream>
#include "Security.h"
#include "FileWatcher.h"
void getSomeData_asyn(Security& security,std::vector<char>& vBuffer);
void start_new_connection(boost::asio::ip::tcp::socket& socket, boost::asio::ip::tcp::endpoint& endpoint);
void file_watcher(std::string const & user_path);
std::mutex mutex;
std::condition_variable cv;
std::set<std::string> set_errors;
boost::system::error_code ec;
void menu();
void fill_set_errors();
int main(int argc, char** argv) {

    std::cout << "Client Program" << std::endl;
    /*network programming trying */

    if( argc!=3 )
    {
        std::cout<<"Errore numero parametri linea di comando"<<std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        //boost::asio::ip::tcp::resolver resolver(io_context); ci servira' probabilmente

        boost::asio::io_context io_context;//create a context essentially the platform specific interface
        boost::asio::io_context::work idleWork(io_context); //fake tasks to asio
        std::thread thrContext = std::thread([&] () {io_context.run();}); //start context in background
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(argv[1],ec),std::stoi(argv[2]));
        boost::asio::ip::tcp::socket  socket(io_context); //the context will deliver the implementation
        start_new_connection(socket,endpoint);
        fill_set_errors();

        if(socket.is_open())
        {
            unsigned int scelta;
            std::unique_lock<std::mutex> ul(mutex);
            std::vector<char> vBuffer(1024); //big buffer , regulate the speed and costs
            std::string usr;
            std::string psw;
            Security security(usr, psw, socket);
            getSomeData_asyn(security,vBuffer);
            while(true)
            {
                if(security.isLogged())
                {
                    boost::filesystem::create_directories("../"+security.getUsr());
                    //fase di restore
                    std::thread fw(file_watcher,security.getUsr());
                }
                  menu();
                    std::cout <<"Inserire scelta: ";
                    std::cin >> scelta;
                switch(scelta)
                {
                    case 1:
                    {
                        //try_lock()
                        if(!security.isLogged())
                            security.register_user();
                        else{
                            std::cout<<"Utente "<<usr<<" loggato, eseguire prima un logout."<<std::endl;
                            continue;
                        }
                         break;
                    }
                    case 2:
                    {
                        //try_lock()
                        if(!security.isLogged())
                            security.login();
                        else{
                            std::cout<<"Utente "<<security.getUsr()<<" loggato, eseguire prima un logout."<<std::endl;
                            continue;
                        }
                        break;
                            break;
                    }
                    case 3:
                    {
                        //try_lock()
                        security.getData();
                        break;
                    }
                    case 4:
                    {
                        security.logout();
                        break;
                    }

                    case 5:
                    {
                        exit(EXIT_SUCCESS);
                    }


                    default:
                    {
                        std::cout<<"Errore scelta, si prega di riprovare"<<std::endl;
                        cv.notify_all();
                        break;
                    }

                }
                //lock()
                cv.wait(ul);
            }

        }
        io_context.stop();
        if(thrContext.joinable())
            thrContext.join();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

void getSomeData_asyn(Security& security,std::vector<char>& vBuffer)
{
        security.getSocket().async_read_some(boost::asio::buffer(vBuffer.data(),vBuffer.size()),
                               [&](std::error_code ec,std::size_t length)
                               {
                                   if(!ec)
                                   {
                                       std::cout <<"\n\n Read " <<length << " bytes\n\n";
                                       for( int i=0; i<length; i++)
                                       {
                                           if(vBuffer[i]!='\a' && vBuffer[i]!='\b' && vBuffer[i]!=EOF)
                                           std::cout << vBuffer[i];
                                       }
                                       std::cout<<std::endl;

                                       //logica applicativa


                                       std::string search(vBuffer.begin(),vBuffer.begin()+length);

                                       /*
                                       std::cout<<"STAMPA DI SEARCH-> "<<std::endl;
                                       std::cout<<":::"<<search<<std::endl;
                                       std::cout<<"STAMPA DI BUFFER-> "<<std::endl;
                                       std::cout<<":::"<<vBuffer.data()<<std::endl;

                                        */
                                       //std::cout.write(search.c_str(), search.length());

                                       if ( search.find('\a')!=std::string::npos)
                                       {
                                           std::cout << "Utente gia' presente nel sistema, inserire un diverso username" <<std::endl;
                                           security.same_procedure(MsgType::REGISTER,true);
                                       }
                                       if ( search.find('\b')!=std::string::npos)
                                       {
                                           std::cout << "Login fallito, username o password sbagliate, riprovare" <<std::endl;
                                           security.same_procedure(MsgType::LOGIN,true);
                                       }

                                       std::string registrazione("REGISTRAZIONE AVVENUTA\r\n");
                                       if (search.find(registrazione)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           security.setLogged(true);
                                           cv.notify_all();
                                       }

                                       std::string login("CLIENT LOGGED\r\n");
                                       if (search.find(login)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           std::cout<<"Login di user = "<<security.getUsr()<<std::endl;
                                           security.setLogged(true);
                                           cv.notify_all();
                                       }


                                       std::string logout("CLIENT LOGOUT\r\n");
                                       if (search.find(logout)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           security.setLogged(false);
                                           cv.notify_all();
                                       }

                                       std::string logout_err("LOGOUT FALLITO\r\n");
                                       if (search.find(logout_err)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           cv.notify_all();
                                       }


                                       std::string file_str("FILE MANDATO CON SUCCESSO\r\n");
                                       if (search.find(file_str)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           cv.notify_all();
                                       }

                                       std::string file_str_err("FILE CERCATO NON TROVATO\r\n");
                                       if (search.find(file_str_err)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           cv.notify_all();
                                       }


                                       std::string timeout("TIMEOUT SESSION EXPIRED\r\n");
                                       if (search.find(timeout)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           cv.notify_all();
                                           menu();
                                           std::cout<<"Timeout scaduto, inserire scelta se necessario: "<<std::endl;
                                       }

                                       std::string err_gnrc("ERRORE, file non trovato o errore generico\r\n");
                                       if (search.find(err_gnrc)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           cv.notify_all();
                                       }
                                           getSomeData_asyn(security,vBuffer); // isn't a real recursive but a system watching of network data.
                                   }
                                   else std::cout<<ec.message()<<std::endl;


                               }
        );
}


void file_watcher(std::string const & user_path)
{
    FileWatcher fw{"../"+user_path,std::chrono::milliseconds(3000)};
    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([](const std::string &path_to_watch,FileStatus status)-> void {
        // Process only regular files, all other file types are ignored
        if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch))) //&& status != FileStatus::erased)
            return;
        switch(status) {
            case FileStatus::created:
            {
                std::cout << "File created: " << path_to_watch << '\n';
                Message::message<MsgType> new_file_msg;
                new_file_msg.header.id=MsgType::NEW_FILE;
                new_file_msg << path_to_watch+"/r/n";
                uint32_t size = boost::filesystem::file_size(path_to_watch);
                new_file_msg.body.push_back(size);
                std::ifstream  ifs( path_to_watch, std::ios_base::binary );
                ifs.open (path_to_watch, std::ifstream::in);
                char c = ifs.get();
                while (ifs.good()) {
                    c = ifs.get();
                    new_file_msg.body.push_back(c);
                }
                ifs.close();

                break;
            }
            case FileStatus::modified:
                std::cout << "File modified: " << path_to_watch << '\n';
                break;
            case FileStatus::erased:
                std::cout << "File erased: " << path_to_watch << '\n';
                break;
            default:
                std::cout << "Error! Unknown file status.\n";
        }
    });
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
    std::cout<<"Per uscire dal programma premere [5]"<<std::endl;

    std::cout<<"###########################################"<<std::endl;
    std::cout<<"#################MENU######################"<<std::endl;
    std::cout<<"###########################################"<<std::endl;
}

//todo
void fill_set_errors()
{
    set_errors.insert("\a");
    set_errors.insert("\b");
    set_errors.insert("REGISTRAZIONE AVVENUTA\r\n");
    set_errors.insert("CLIENT LOGGED\r\n");
    set_errors.insert("CLIENT LOGOUT\r\n");
    set_errors.insert("LOGOUT FALLITO\r\n");
    set_errors.insert("FILE MANDATO CON SUCCESSO\r\n");
    set_errors.insert("ERRORE, file non trovato o errore generico\r\n");
}


void start_new_connection(boost::asio::ip::tcp::socket& socket, boost::asio::ip::tcp::endpoint& endpoint)
{
        socket.connect(endpoint,ec);
        if(!ec)
        {
            std::cout <<"Connected to the Server" << std::endl;
        }
        else
        {
            std::cout << "Failed to connect to address:\n" << ec.message() <<std::endl;
            exit(EXIT_FAILURE);
        }
}