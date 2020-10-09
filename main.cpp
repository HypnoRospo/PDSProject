#include <iostream>
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include "FileWatcher.h"
using namespace sql;

int main() {
    std::cout << "Client Program" << std::endl;

    try {
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        /* Create a connection */
        driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "root", "");
        /* Connect to the MySQL test database */
        con->setSchema("test");

        stmt = con->createStatement();
        res = stmt->executeQuery("SELECT 'Hello World!' AS _message");
        while (res->next()) {
            std::cout << "\t... MySQL replies: ";
            /* Access column data by alias or column name */
            std::cout << res->getString("_message") << std::endl;
            std::cout << "\t... MySQL says it again: ";
            /* Access column data by numeric offset, 1 is the first column */
            std::cout << res->getString(1) << std::endl;
        }
        delete res;
        delete stmt;
        delete con;

    } catch (sql::SQLException &e) {
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }



    /*network programming trying */


    boost::system::error_code ec;
    boost::asio::io_context context;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1",ec),80);
    boost::asio::ip::tcp::socket  socket(context);
    socket.connect(endpoint,ec);
    if(!ec)
    {
        std::cout <<"Connected " << std::endl;
    }
    else
    {
        std::cout << "Failed to connect to address:\n" << ec.message() <<std::endl;
    }

    if(socket.is_open())
    {
        //std::string sRequest =

        //socket.write_some(asio::buffer(data,size,size... to do

    }


    FileWatcher fw{"./",std::chrono::milliseconds(500)};
    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([](const std::string &path_to_watch,FileStatus status)-> void {
        // Process only regular files, all other file types are ignored
        if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch))) //&& status != FileStatus::erased)
            return;
        switch(status) {
            case FileStatus::created:
                std::cout << "File created: " << path_to_watch << '\n';
                break;
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

    return 0;
}
