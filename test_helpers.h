#include <chrono>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

[[maybe_unused]] static long millis(const std::chrono::system_clock::time_point& end,const std::chrono::system_clock::time_point& start) {
    return ((end-start).count())/1000;
}

[[maybe_unused]] static std::string dbsize(const std::string& dbpath) {
    long size = 0;
    for(auto f : fs::directory_iterator(dbpath)) {
        size += fs::file_size(f);
    }
    char tmp[128];
    snprintf(tmp,128,"%0.1fM", size/(1024.0*1024.0));
    return tmp;
}