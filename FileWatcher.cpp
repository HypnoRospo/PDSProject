//
// Created by enrico_scalabrino on 12/10/20.
//

#include "FileWatcher.h"
#include <iostream>

 void file_watcher()
 {
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
 }