//
// Created by enrico_scalabrino on 27/10/20.
//

#ifndef PDSPROJECT_SECURITY_H
#define PDSPROJECT_SECURITY_H

#include <string>
#include <boost/asio/ip/tcp.hpp>

class Security {

private:

    std::string& usr;


protected:



public:
     void login(std::string& user,std::string& password,boost::asio::ip::tcp::socket& socket);
     void register_user(std::string& user,std::string& password,boost::asio::ip::tcp::socket& socket);
};


#endif //PDSPROJECT_SECURITY_H
