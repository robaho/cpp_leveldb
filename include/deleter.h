#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

class Deleter {
private:    
    const std::string dbpath;
    std::ofstream file;
public:
    Deleter(const std::string& dbpath) : dbpath(dbpath){};
    Deleter() : dbpath(""){};
    void scheduleDeletion(std::vector<std::string>& files) {
        if(dbpath=="") return;
        if(!file.is_open()) {
            file.open(dbpath+"/deleted",std::ios::out | std::ios::app);
        }
        for(auto f : files) {
            file << f << "\n";
        }
        file.flush();
    }
    void deleteScheduled() {
        if(file.is_open()) file.close();
        std::fstream in;
        in.open(dbpath+"/deleted",std::ios::in);
        std::string entry;
        while(std::getline(in,entry)) {
            auto filepath = dbpath+"/"+entry;
            if(fs::exists(filepath)) {
                fs::remove(filepath);
            }
        }
        in.close();
        fs::remove(dbpath+"/deleted");
    }
};
