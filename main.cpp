#include <iostream>
#include <thread>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <condition_variable>
#include <boost/algorithm/string/find.hpp>
#include <boost/filesystem.hpp>
#include <set>
#include <fstream>
#include "Security.h"
#include "FileWatcher.h"

void getSomeData_asyn(Security& security,std::vector<char>& vBuffer);
void start_new_connection(boost::asio::ip::tcp::socket& socket, boost::asio::ip::tcp::endpoint& endpoint);
void file_watcher(Security const & security);
std::mutex mutex;
std::condition_variable cv;
bool ready = false;
bool processed = false;
bool logged =false;
std::thread fw_thread;
boost::system::error_code ec;
// Redefine this to change to processing buffer size

void menu();
void prepare_file(const std::string& path_to_watch ,std::vector<char>& body);
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

        std::string root_path("../client_users/");
        if(!boost::filesystem::exists(root_path)) {
            boost::filesystem::create_directory(root_path);
        }
        boost::filesystem::current_path(root_path);
        start_new_connection(socket,endpoint);

        if(socket.is_open())
        {
            bool on=true;
            unsigned int scelta;
            std::thread fw_thread;
            std::vector<char> vBuffer(1024); //big buffer , regulate the speed and costs
            std::string usr;
            std::string psw;
            Security security(usr, psw, socket);
            getSomeData_asyn(security,vBuffer);
            while(on)
            {
                menu();
                    std::cout <<"Inserire scelta: ";
                    std::cin >> scelta;
                switch(scelta)
                {
                    case 1:
                    {
                        //try_lock()
                        if(!logged)
                        {
                            security.register_user();
                            std::lock_guard<std::mutex> lk(mutex);
                            ready = true;
                            cv.notify_one();
                            break;
                        }
                        else{
                            std::cout<<"Utente "<<usr<<" loggato, eseguire prima un logout."<<std::endl;
                            continue;
                        }
                    }
                    case 2:
                    {
                        //try_lock()
                        if(!logged)
                        {
                            security.login();
                            std::lock_guard<std::mutex> lk(mutex);
                            ready = true;
                            cv.notify_one();
                            break;
                        }
                        else{
                            std::cout<<"Utente "<<security.getUsr()<<" loggato, eseguire prima un logout."<<std::endl;
                            continue;
                        }
                    }
                    case 3:
                    {
                        //try_lock()
                        security.getData();
                        std::lock_guard<std::mutex> lk(mutex);
                        ready = true;
                        cv.notify_one();
                        break;
                    }
                    case 4:
                    {
                        //try lock
                        security.logout();
                        std::lock_guard<std::mutex> lk(mutex);
                        ready = true;
                        logged=false;
                        cv.notify_one();
                       break;
                    }

                    case 5:
                    {
                        on=false;
                        logged=false;
                        break;
                    }

                    default:
                    {
                        std::cout<<"Errore scelta, si prega di riprovare"<<std::endl;
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        continue;
                    }

                }
                //lock()
                if(on)
                {
                    std::unique_lock<std::mutex> lk(mutex);
                    cv.wait(lk, []{return processed;});
                    /*
                    if(!logged)
                    {
                        fw_thread.join();
                    }
                     */
                    ready=false;
                    processed=false;
                }
            }
        }
        //pulisco risorse
        io_context.stop();
            thrContext.join();
            sleep(1); // behind reason
        if(fw_thread.joinable()) // must wait if filewatcher has delay too long, in this case not
            fw_thread.join();
        socket.close();
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

                                           std::unique_lock<std::mutex> lk(mutex);
                                           cv.wait(lk, []{return ready;});

                                           std::cout<<"Login di user = "<<security.getUsr()<<std::endl;

                                           logged=true;

                                           // Send data back to main()
                                           processed = true;
                                           // Manual unlocking is done before notifying, to avoid waking up
                                           // the waiting thread only to block again (see notify_one for details)
                                           lk.unlock();
                                           cv.notify_one();

                                           if(!boost::filesystem::exists(security.getUsr())) {
                                               boost::filesystem::create_directory(security.getUsr());
                                           }
                                           fw_thread = std::thread(file_watcher,security);
                                       }

                                       std::string login("CLIENT LOGGED\r\n");
                                       if (search.find(login)!=std::string::npos)
                                       {
                                           //mutex.unlock()

                                           std::unique_lock<std::mutex> lk(mutex);
                                           cv.wait(lk, []{return ready;});

                                           std::cout<<"Login di user = "<<security.getUsr()<<std::endl;

                                           logged=true;

                                           // Send data back to main()
                                           processed = true;
                                           // Manual unlocking is done before notifying, to avoid waking up
                                           // the waiting thread only to block again (see notify_one for details)
                                           lk.unlock();
                                           cv.notify_one();

                                           if(!boost::filesystem::exists(security.getUsr())) {
                                               boost::filesystem::create_directory(security.getUsr());
                                           }
                                           fw_thread = std::thread(file_watcher,security);
                                       }


                                       std::string logout("CLIENT LOGOUT\r\n");
                                       if (search.find(logout)!=std::string::npos)
                                       {
                                           //mutex.unlock()

                                           std::unique_lock<std::mutex> lk(mutex);
                                           cv.wait(lk, []{return ready;});

                                           logged=false;
                                           std::cout<<logout<<std::endl;

                                           fw_thread.join();
                                           // Send data back to main()
                                           processed = true;
                                           // Manual unlocking is done before notifying, to avoid waking up
                                           // the waiting thread only to block again (see notify_one for details)
                                           lk.unlock();
                                           cv.notify_one();
                                       }

                                       std::string logout_err("LOGOUT FALLITO\r\n");
                                       if (search.find(logout_err)!=std::string::npos)
                                       {
                                           std::unique_lock<std::mutex> lk(mutex);
                                           cv.wait(lk, []{return ready;});
                                           // Send data back to main()
                                           processed = true;
                                           // Manual unlocking is done before notifying, to avoid waking up
                                           // the waiting thread only to block again (see notify_one for details)
                                           lk.unlock();
                                           cv.notify_one();
                                       }


                                       std::string file_str("FILE MANDATO CON SUCCESSO\r\n");
                                       if (search.find(file_str)!=std::string::npos)
                                       {
                                           std::unique_lock<std::mutex> lk(mutex);
                                           cv.wait(lk, []{return ready;});
                                           // Send data back to main()
                                           processed = true;
                                           // Manual unlocking is done before notifying, to avoid waking up
                                           // the waiting thread only to block again (see notify_one for details)
                                           lk.unlock();
                                           cv.notify_one();
                                       }

                                       std::string file_str_R("FILE RICEVUTO CON SUCCESSO\r\n");
                                       if (search.find(file_str_R)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           menu();
                                           std::cout<<"Operazione terminata, inserire scelta: "<<std::endl;
                                           //cv.notify_all();
                                       }

                                       std::string checksum_result("CHECKSUM CORRETTO\r\n");
                                       if(search.find(checksum_result)!=std::string::npos)
                                       {
                                           std::cout<<"File nel server gia' aggiornato"<<std::endl;
                                           //cv.notify_all();
                                       }

                                       std::string checksum_result_not("CHECKSUM NON CORRETTO\r\n");
                                       if(search.find(checksum_result_not)!=std::string::npos)
                                       {

                                           //cv.notify_all(); //sveglio gia il client, non aspetto
                                           //logica di spacchettamento
                                           std::string::size_type pos = 0;
                                           std::string delimiter = "\r\n";
                                           while ((pos = search.find(delimiter, pos )) != std::string::npos) {
                                               if(pos==length-delimiter.length()) //siamo arrivati alla fine, esco
                                                   break;
                                               std::vector<char> fill;
                                               std::string path_user;
                                               //std::string path_user(boost::find_nth(search,delimiter,i+1).begin(),boost::find_nth(search,delimiter,i+2).begin());
                                               path_user = search.substr(pos+delimiter.length(),search.find(delimiter,pos+delimiter.length())-(pos+delimiter.length()));
                                               if(boost::filesystem::is_regular_file(path_user))
                                               {
                                                   prepare_file(path_user,fill);
                                                   //send message to server
                                                   Message::message<MsgType> new_file_msg;
                                                   new_file_msg.header.id = MsgType::NEW_FILE;
                                                   //path_user+=delimiter;
                                                   std::vector<char> path_user_vector(path_user.begin(),path_user.end());
                                                   std::vector<char> total;
                                                   total.reserve( path_user.size() + fill.size()+delimiter.length() ); // preallocate memory
                                                   total.insert( total.end(), path_user.begin(), path_user.end() );
                                                   total.insert(total.end(),delimiter.begin(),delimiter.end());
                                                   total.insert( total.end(), fill.begin(), fill.end() );
                                                   new_file_msg << total;
                                                   new_file_msg.sendMessage(security.getSocket());
                                               }
                                               pos += delimiter.length();
                                           }
                                       }

                                       std::string file_str_err("FILE CERCATO NON TROVATO\r\n");
                                       if (search.find(file_str_err)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           //cv.notify_all();
                                           std::unique_lock<std::mutex> lk(mutex);
                                           cv.wait(lk, []{return ready;});
                                           // Send data back to main()
                                           processed = true;
                                           // Manual unlocking is done before notifying, to avoid waking up
                                           // the waiting thread only to block again (see notify_one for details)
                                           lk.unlock();
                                           cv.notify_one();
                                       }

                                       std::string timeout("TIMEOUT SESSION EXPIRED\r\n");
                                       if (search.find(timeout)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           //cv.notify_all();
                                           menu();
                                           std::cout<<"Timeout scaduto, inserire scelta se necessario: "<<std::endl;
                                       }

                                       std::string err_gnrc("ERRORE, file non trovato o errore generico\r\n");
                                       if (search.find(err_gnrc)!=std::string::npos)
                                       {
                                           //mutex.unlock()
                                           //cv.notify_all();
                                       }
                                           getSomeData_asyn(security,vBuffer); // isn't a real recursive but a system watching of network data.
                                   }
                                   else std::cout<<ec.message()<<std::endl;


                               }
        );
}


void file_watcher(Security const & security)
{
    FileWatcher fw{security.getUsr(),std::chrono::milliseconds(500),security}; //passo tutto security per comodita'
    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function

    fw.sync(security);

    fw.start([&](const std::string &path_to_watch,FileStatus status)-> void {
        bool file=std::filesystem::is_regular_file(std::filesystem::path(path_to_watch));
        bool folder=std::filesystem::is_directory(std::filesystem::path(path_to_watch));
        // Process only regular files, all other file types are ignored
        if(!file && !folder && status != FileStatus::erased)
        {
                return;
        }
        switch(status) {
            case FileStatus::created:
            case FileStatus::modified:{
                if(file)
                {
                    if(status==FileStatus::created)
                    std::cout << "File created: " << path_to_watch <<std::endl;
                    else std::cout <<"File modified: "<< path_to_watch<<std::endl;
                    Message::message<MsgType> new_file_msg;
                    new_file_msg.header.id = MsgType::NEW_FILE;
                    std::string path_usr = path_to_watch + "\r\n";
                    std::vector<char> body(path_usr.begin(), path_usr.end());
                    prepare_file(path_to_watch,body);
                    new_file_msg << body;
                    new_file_msg.sendMessage(security.getSocket());
                    break;
                }
                else
                {
                    if(status==FileStatus::created)
                        std::cout << "Folder created: " << path_to_watch <<std::endl;
                    else std::cout <<"Folder modified: "<< path_to_watch<<std::endl;
                    Message::message<MsgType> new_file_msg;
                    new_file_msg.header.id = MsgType::NEW_FILE;
                    std::string path_usr = path_to_watch + "/";
                    new_file_msg<<path_usr;
                    new_file_msg.sendMessage(security.getSocket());
                    break;
                }
            }
            case FileStatus::erased:
            {
                if(file) std::cout << "File erased: " << path_to_watch <<std::endl;
                else std::cout <<"Folder erased: "<< path_to_watch<<std::endl;
                Message::message<MsgType> new_file_msg;
                new_file_msg.header.id = MsgType::DELETE;
                std::string path_usr = path_to_watch + "\r\n";
                new_file_msg<<path_usr;
                new_file_msg.sendMessage(security.getSocket());
                break;
            }



            default:
                std::cout << "Error! Unknown file status.\n";
        }
    });
}

void prepare_file(const std::string& path_to_watch ,std::vector<char>& body)
{
    std::ifstream ifs(path_to_watch, std::ios::binary);
    if (ifs) {
        ifs.seekg(0, std::ifstream::end);
        int length = ifs.tellg();
        ifs.seekg(0, std::ifstream::beg);

        std::vector<char> vector_buffer(length);
        ifs.read(vector_buffer.data(), length);

        if (ifs)
            body.insert(body.end(), vector_buffer.begin(), vector_buffer.end());
        ifs.close();
    }
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

