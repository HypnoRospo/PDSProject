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
extern std::mutex mutex;
extern bool ready;
extern bool processed;
extern bool logged;

class Security {
public:
    Security(std::string &usr, std::string &psw, boost::asio::ip::tcp::socket &socket);

private:
    std::string& usr;
    std::string& psw;
    boost::asio::ip::tcp::socket& socket;
    void setNonce() const;
//protected:

public:
     void login() const;
     void  register_user() const;
     void getData() const;
     void logout() const;
     void end() const;
    void same_procedure(MsgType msgType,bool repeat) const;
    virtual ~Security();
    static std::string calculate_checksum(std::ifstream& ifs);

    [[nodiscard]] boost::asio::ip::tcp::socket &getSocket() const;
    [[nodiscard]] std::string &getUsr() const;
    [[nodiscard]] std::string &getPsw() const;

};


#endif //PDSPROJECT_SECURITY_H
