//
// Created by enrico_scalabrino on 27/10/20.
//

#define PRIVATE_BUFFER_SIZE  8192
#include "Security.h"
#include "Message.h"
#include <sodium/crypto_secretbox.h>
#include <fstream>
#include <boost/crc.hpp>  // for boost::crc_32_type

unsigned char key[crypto_secretbox_KEYBYTES] ={"pds_project_key"};
unsigned char nonce[crypto_secretbox_NONCEBYTES]={};
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
    if(isLogged())
        return;

    std::cout <<"Inserire nome utente o exit per uscire: ";
    std::cin >> usr;
    if(usr=="exit")
    {
        if(thread)
        {
            cv.notify_all();
        }
        return;
    }
    std::cout <<"Inserire password: ";
    std::cin >> psw ;
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
    std::string path_str;
    std::cout <<"Inseire path per favore: ";
    std::cin >> path_str;
    Message::message<MsgType> get_data;
    get_data.set_id(MsgType::GETPATH);
    get_data << path_str;
    get_data.sendMessage(socket);
}

void Security::logout() const
{
    Message::message<MsgType> fine;
    fine.set_id(MsgType::LOGOUT);
    fine.sendMessage(socket);
    std::cout<<"Logout di user = "<<usr<<std::endl;
}


boost::asio::ip::tcp::socket &Security::getSocket() const {
    return socket;
}

bool Security::isLogged() const {
    return logged;
}

void Security::setLogged(bool logged_){
    Security::logged = logged_;
}

std::string &Security::getUsr() const {
    return usr;
}

std::string &Security::getPsw() const {
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
        std::cout<<"Tempo impiegato per il calcolo del CRC: "<<(double)(clock() - tStart)/CLOCKS_PER_SEC;

        std::cout << std::hex << std::uppercase << result.checksum() << std::endl;

        std::stringstream stream;
        stream << std::hex << std::uppercase << result.checksum();
        return stream.str();

    }
    catch ( std::exception &e )
    {
        std::cerr << "Found an exception with '" << e.what() << "'." << std::endl;
        return e.what();
        /* VA GESTITA LA RETURN ADATTA */
    }
    catch ( ... )
    {
        std::cerr << "Found an unknown exception." << std::endl;
        return "Errore sconosciuto sul calcolo CRC";
        /* VA GESTITA LA RETURN ADATTA */

    }

}

Security::Security(std::string &usr, std::string &psw, boost::asio::ip::tcp::socket &socket)
        : usr(usr), psw(psw), socket(socket) {}






