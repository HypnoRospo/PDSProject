//
// Created by enrico_scalabrino on 27/10/20.
//

#ifndef PDSPROJECT_SECURITY_H
#define PDSPROJECT_SECURITY_H
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include "Message.h"
extern std::condition_variable cv;
class Security {
public:
    Security(std::string &usr, std::string &psw, boost::asio::ip::tcp::socket &socket);

private:
    std::string& usr;
    std::string& psw;
    bool logged{false};
    boost::asio::ip::tcp::socket& socket;
    void setNonce() const;
//protected:

public:
     void login() const;
     void register_user() const;
     void getData() const;
     void logout() const;
    void same_procedure(MsgType msgType,bool repeat) const;
    virtual ~Security();
    void setLogged(bool logged);
    [[nodiscard]] bool isLogged() const;
    [[nodiscard]] boost::asio::ip::tcp::socket &getSocket() const;
    [[nodiscard]] std::string &getUsr() const;
    [[nodiscard]] std::string &getPsw() const;

};


#endif //PDSPROJECT_SECURITY_H
