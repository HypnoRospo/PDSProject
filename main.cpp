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
#include <sodium.h>
#include <sodium/crypto_pwhash.h>
#include <sodium/crypto_auth.h>
#include <sodium/crypto_secretbox.h>
#include <sodium/randombytes.h>
#include "Message.h"
using namespace  std::chrono_literals;
std::vector<char> vBuffer(1*1024); //big buffer , regulate the speed and costs
unsigned char key[crypto_secretbox_KEYBYTES] ={"pds_project_key"};
unsigned char nonce[crypto_secretbox_NONCEBYTES]={};
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

            std::cout <<"Register: " << std::endl;
            boost::system::error_code ignored_error;
            std::vector<unsigned char> cipher_vect_r;
            Message::message<MsgType> mex_r;
            mex_r.set_id(MsgType::REGISTER);
            std::string usr("andrea");
            std::string psw("pdsproject");
            std::string usr_psw = usr+" "+psw;
            size_t c_len_r =crypto_secretbox_MACBYTES + usr_psw.length();
            unsigned char ciphertext_r[c_len_r];
            //crypto_secretbox_keygen(key);

            randombytes_buf(nonce, sizeof nonce);
            Message::message<MsgType> nonce_msg_r;
            nonce_msg_r.set_id(MsgType::NONCE);
            std::vector<char> nonce_container_r;
            std::copy(&nonce[0],&nonce[crypto_secretbox_NONCEBYTES],std::back_inserter(nonce_container_r));
            nonce_container_r.push_back('\n');
            nonce_msg_r << nonce_container_r;
            boost::asio::write(socket, boost::asio::buffer(&nonce_msg_r.header.id, sizeof(nonce_msg_r.header.id)), ignored_error);
            boost::asio::write(socket, boost::asio::buffer(nonce_msg_r.body.data(), nonce_msg_r.body.size()), ignored_error);


            crypto_secretbox_easy(ciphertext_r, reinterpret_cast<const unsigned char *>(usr_psw.c_str()), usr_psw.length(), nonce, key);
            cipher_vect_r.assign(ciphertext_r,ciphertext_r+c_len_r);
            cipher_vect_r.push_back('\n'); //terminatore, importante senno dobbiamo ricopiare un'altra leggi comadno
            mex_r << cipher_vect_r;
            boost::asio::write(socket, boost::asio::buffer(&mex_r.header.id, sizeof(mex_r.header.id)), ignored_error);
            boost::asio::write(socket, boost::asio::buffer(mex_r.body.data(), mex_r.body.size()), ignored_error);


            std::cout <<"Login requested: " << std::endl;
            std::vector<unsigned char> cipher_vect;
            Message::message<MsgType> mex;
            mex.set_id(MsgType::LOGIN);
            std::string user(argv[3]);
            std::string password(argv[4]);
            std::string user_password = user+" "+password;
            size_t c_len =crypto_secretbox_MACBYTES + user_password.length();
            unsigned char ciphertext[c_len];
            //crypto_secretbox_keygen(key);

            randombytes_buf(nonce, sizeof nonce);
            Message::message<MsgType> nonce_msg;
            nonce_msg.set_id(MsgType::NONCE);
            std::vector<char> nonce_container;
            std::copy(&nonce[0],&nonce[crypto_secretbox_NONCEBYTES],std::back_inserter(nonce_container));
            nonce_container.push_back('\n');
            nonce_msg << nonce_container;
            boost::asio::write(socket, boost::asio::buffer(&nonce_msg.header.id, sizeof(nonce_msg.header.id)), ignored_error);
            boost::asio::write(socket, boost::asio::buffer(nonce_msg.body.data(), nonce_msg.body.size()), ignored_error);


            crypto_secretbox_easy(ciphertext, reinterpret_cast<const unsigned char *>(user_password.c_str()), user_password.length(), nonce, key);
            cipher_vect.assign(ciphertext,ciphertext+c_len);
            cipher_vect.push_back('\n'); //terminatore, importante senno dobbiamo ricopiare un'altra leggi comadno
            mex << cipher_vect;
            boost::asio::write(socket, boost::asio::buffer(&mex.header.id, sizeof(mex.header.id)), ignored_error);
            boost::asio::write(socket, boost::asio::buffer(mex.body.data(), mex.body.size()), ignored_error);

            Message::message<MsgType> get_data;
            get_data.set_id(MsgType::GETPATH);
            std::string get_str("GET ../files/prova\r\n");
            get_data << get_str;
            boost::asio::write(socket, boost::asio::buffer(&get_data.header.id, sizeof(get_data.header.id)), ignored_error);
            boost::asio::write(socket, boost::asio::buffer(get_data.body.data(), get_data.body.size()), ignored_error);
            socket.wait(boost::asio::ip::tcp::socket::wait_read);

            Message::message<MsgType> fine;
            fine.set_id(MsgType::LOGOUT);
            //std::string fine_str("FINE\r\n");
            //fine << fine_str;
            boost::asio::write(socket, boost::asio::buffer(&fine.header.id, sizeof(fine.header.id)), ignored_error);
            //boost::asio::write(socket, boost::asio::buffer(fine.body.data(), fine.body.size()), ignored_error);
            //cose commentate superflue, basta header

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