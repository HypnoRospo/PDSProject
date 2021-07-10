//
// Created by enric on 04/10/2020.
//

#ifndef PROJECT_FILEWATCHER_H
#define PROJECT_FILEWATCHER_H
#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
// Define available file changes
enum class FileStatus {created, modified, erased};

class FileWatcher {
public:
    std::string path_to_watch;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(const std::string &path_to_watch, std::chrono::duration<int, std::milli> delay,const Security& security) : path_to_watch{path_to_watch}, delay{delay} {

        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            paths_[file.path().string()] = std::filesystem::last_write_time(file);
        }
    }

    void start(const std::function<void (std::string, FileStatus)> &action) {
        while(running_){
            if(!logged)
            {
                running_=false;
                break;
            }
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);

            auto it = paths_.begin();
            while (it != paths_.end()) {
                if (!std::filesystem::exists(it->first)) {
                    action(it->first, FileStatus::erased);
                    it = paths_.erase(it);
                }
                else {
                    it++;
                }
            }

            // Check if a file was created or modified
            try
            {
                for(auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
                    auto current_file_last_write_time = std::filesystem::last_write_time(file);

                    // File creation
                    if(!contains(file.path().string())) {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(), FileStatus::created);
                        // File modification
                    } else {
                        if(paths_[file.path().string()] != current_file_last_write_time) {
                            //logica qui per capire se e' in realta' rename
                            paths_[file.path().string()] = current_file_last_write_time;
                            action(file.path().string(), FileStatus::modified);
                        }
                    }
                }
            }
            catch(std::filesystem::filesystem_error &fe)
            {
                throw fe;
            }
        }
    }

    void sync(Security const & security) const
    {

        std::vector<char> body;
        Message::message<MsgType> sync_msg;
        sync_msg.header.id = MsgType::ELEMENT_CLIENT;
        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            //  if (!file.is_directory()) {
            std::string path_usr = file.path().string() + "\r\n";
            body.insert(body.end(),path_usr.begin(),path_usr.end());
            std::ifstream ifs(file.path().string(), std::ios::binary);

            sync_msg << body;

            //}
        }


        sync_msg.sendMessage(security.getSocket());

        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            if(!file.is_directory())
            {
                Message::message<MsgType> crc_msg;
                crc_msg.header.id = MsgType::CRC;
                std::string path_usr = file.path().string() + "\r\n";
                std::vector<char> body(path_usr.begin(), path_usr.end());
                std::ifstream ifs(file.path().string(), std::ios::binary);
                // Calcolo il CRC e invio il messaggio "Path file " + CRC  al server che verifica se gli serve il nuovo file o meno
                std::string checksum_string = Security::calculate_checksum(ifs);
                std::string path = checksum_string + "\r\n";
                // Ho impacchettato nel body il path + CRC
                body.insert(body.end(),path.begin(),path.end());
                crc_msg << body;
                crc_msg.sendMessage(security.getSocket());
            }
            else
            {
                Message::message<MsgType> new_file_msg;
                new_file_msg.header.id = MsgType::NEW_FILE;
                std::string path_usr = file.path().string() + "/";
                new_file_msg<<path_usr;
                new_file_msg.sendMessage(security.getSocket());
            }

        }
    }

private:
    std::unordered_map<std::string, std::filesystem::file_time_type> paths_;
    bool running_ = true;
    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key) {
        auto el = paths_.find(key);
        return el != paths_.end();
    }
};
#endif //PROJECT_FILEWATCHER_H