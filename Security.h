//
// Created by enrico_scalabrino on 27/10/20.
//

#ifndef PDSPROJECT_SECURITY_H
#define PDSPROJECT_SECURITY_H

#include <string>
#include <boost/asio/ip/tcp.hpp>
#include "Message.h"
class Security {
public:
    Security(std::string &usr, std::string &psw, boost::asio::ip::tcp::socket &socket);

private:
    std::string& usr;
    std::string& psw;
    boost::asio::ip::tcp::socket& socket;
    void setNonce() const;
    void same_procedure(MsgType msgType) const;

//protected:

public:
     void login() const;
     void register_user() const;
     void getData(std::string& path) const;
     void logout() const;

    virtual ~Security();

    [[nodiscard]] std::string &getUsr() const;
    [[nodiscard]] std::string &getPsw() const;
    [[nodiscard]] boost::asio::ip::tcp::socket &getSocket() const;
};


#endif //PDSPROJECT_SECURITY_H
