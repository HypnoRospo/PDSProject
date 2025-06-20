//
// Created by enrico_scalabrino on 27/10/20.
//

#define PRIVATE_BUFFER_SIZE  8192
#include "Security.h"
#include "Message.h"
#include <sodium/crypto_secretbox.h>
#include <fstream>
#include <boost/crc.hpp>  // for boost::crc_32_type
#include <boost/algorithm/string.hpp>

unsigned char key[crypto_secretbox_KEYBYTES] ={"pds_project_key"};
unsigned char nonce[crypto_secretbox_NONCEBYTES]={};
extern std::string file_path;
// Redefine this to change to processing buffer size

Security::~Security() = default;

void Security::register_user() const
{
    std::cout <<"Register procedure..." << std::endl;
    same_procedure(MsgType::REGISTER,false);
}


void Security::login() const
{
    std::cout <<"Login procedure... " << std::endl;
    same_procedure(MsgType::LOGIN,false);
}

void Security::setNonce() const
{
    crypto_secretbox_keygen(nonce);

    Message::message<MsgType> nonce_msg_r;
    nonce_msg_r.set_id(MsgType::NONCE);
    std::vector<char> nonce_container_r;
    std::copy(&nonce[0],&nonce[crypto_secretbox_NONCEBYTES],std::back_inserter(nonce_container_r));
    nonce_msg_r << nonce_container_r;
    nonce_msg_r.sendMessage(socket);

}

void Security::same_procedure(MsgType msgType,bool thread) const
{
    //form
    if(logged)
        return;
for(;;) {
    std::cout << "Inserire nome utente o exit per uscire: ";
    std::cin >> usr;
    if (usr == "exit") {
        if (thread) {
            std::unique_lock<std::mutex> lk(mutex);
            cv.wait(lk, [] { return ready; });

            // Send data back to main()
            processed = true;
            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            lk.unlock();
            cv.notify_one();
        } else {

            std::unique_lock<std::mutex> lk(mutex);
            cv.wait(lk, [] { return !ready && !processed; });
            // Send data back to main()
            processed = true;
            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            lk.unlock();
            cv.notify_one();
        }
        return;
    }
    boost::trim(usr);
    if(usr!="admin" && usr!="root" && usr!=" " && usr!="/")
    {
        break;
    }
    else
        std::cout<<"Nome utente non disponibile, riprovare."<<std::endl;

}
    for(;;){
        std::cout <<"Inserire password: ";
        std::cin >> psw ;
        boost::trim(psw);
        if(psw.size()<8) //altre politiche disponibili
        {
            std::cout<<"Password troppo corta, inserire una nuova password"<<std::endl;

        }
        else break;
    }

    //fine form

    std::vector<unsigned char> cipher_vect;
    Message::message<MsgType> mex;
    mex.set_id(msgType);
    std::string usr_psw = usr+" "+psw;
    size_t c_len =crypto_secretbox_MACBYTES + usr_psw.length();
    unsigned char ciphertext[c_len];
    setNonce();
    crypto_secretbox_easy(ciphertext, reinterpret_cast<const unsigned char *>(usr_psw.c_str()), usr_psw.length(), nonce, key);
    cipher_vect.assign(ciphertext,ciphertext+c_len);
    mex << cipher_vect;
    mex.sendMessage(socket);
}

void Security::getData() const
{
    std::cout <<"Inseire path per favore: ";
    std::cin >> file_path;
    Message::message<MsgType> get_data;
    get_data.set_id(MsgType::GETPATH);
    get_data << file_path;
    get_data.sendMessage(socket);
}

void Security::logout() const
{
    Message::message<MsgType> fine;
    fine.set_id(MsgType::LOGOUT);
    fine.sendMessage(socket);
    std::cout<<"Logout di user = "<<usr<<std::endl;
}

void Security::end() const
{
    Message::message<MsgType> fine;
    fine.set_id(MsgType::END);
    fine.sendMessage(socket);
}



boost::asio::ip::tcp::socket &Security::getSocket() const {
    return socket;
}

std::string &Security::getUsr() const {
    return usr;
}

std::string &Security::getPsw() const {  //change password feature
    return psw;
}

std::string Security::calculate_checksum(std::ifstream &ifs) {

    std::streamsize const  buffer_size = PRIVATE_BUFFER_SIZE;

    try
    {
        boost::crc_32_type  result;
        clock_t tStart = clock();
        if ( ifs )
        {
            // get length of file:
            ifs.seekg (0, std::ifstream::end);
            int length = ifs.tellg();
            ifs.seekg (0, std::ifstream::beg);
            //

            if(length > buffer_size)
            {
                std::vector<char>   buffer(buffer_size);
                while(ifs.read(&buffer[0], buffer_size))
                    result.process_bytes(&buffer[0], ifs.gcount());
            }
            else {
                std::vector<char> buffer(length);
                ifs.read(&buffer[0],length);
                result.process_bytes(&buffer[0],ifs.gcount());
            }
        }
        else
        {
            std::cerr << "Impossibile aprire il file '"<< std::endl;
        }
        std::cout<<"Tempo impiegato per il calcolo del CRC: "<<(double)(clock() - tStart)/CLOCKS_PER_SEC<<std::endl;
        std::cout <<"Checksum: "<< std::hex << std::uppercase << result.checksum() << std::endl;

        std::stringstream stream;
        stream << std::hex << std::uppercase << result.checksum();
        return stream.str();

    }
    catch ( std::exception &e )
    {
        std::cerr << "Found an exception with '" << e.what() << "'." << std::endl;
        return e.what();
    }

}

Security::Security(std::string &usr, std::string &psw, boost::asio::ip::tcp::socket &socket)
        : usr(usr), psw(psw), socket(socket) {}






