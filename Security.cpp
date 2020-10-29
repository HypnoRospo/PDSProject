//
// Created by enrico_scalabrino on 27/10/20.
//

#include "Security.h"
#include "Message.h"
#include <sodium.h>
#include <sodium/crypto_secretbox.h>

unsigned char key[crypto_secretbox_KEYBYTES] ={"pds_project_key"};
unsigned char nonce[crypto_secretbox_NONCEBYTES]={};

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

void Security::same_procedure(MsgType msgType,bool repeat) const
{
    if(repeat) form();
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

void Security::getData(std::string& path) const
{
    Message::message<MsgType> get_data;
    get_data.set_id(MsgType::GETPATH);
    get_data << path;
    get_data.sendMessage(socket);
}

void Security::logout() const
{
    Message::message<MsgType> fine;
    fine.set_id(MsgType::LOGOUT);
    fine.sendMessage(socket);
}
void Security::form() const
{
    std::cout <<"Inserire nome utente: ";
    std::cin >> usr ;

    std::cout <<"Inserire password: ";
    std::cin >> psw ;
}
Security::Security(std::string &usr, std::string &psw, boost::asio::ip::tcp::socket &socket)
        : usr(usr), psw(psw), socket(socket) {}

std::string &Security::getUsr() const {
    return usr;
}

std::string &Security::getPsw() const {
    return psw;
}

boost::asio::ip::tcp::socket &Security::getSocket() const {
    return socket;
}
