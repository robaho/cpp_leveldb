#pragma once

#include <filesystem>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

namespace fs = std::filesystem;

/** cross process lock using file locks */
class LockFile {
private:
    std::filesystem::path path;
    int fd = -1;
    bool locked = false;

public:
    LockFile(const std::string& path) : path(path) {
    }
    ~LockFile() {
        if(locked) {
            unlock();
        }
    }

    bool tryLock() {
        if(locked) return true;

        try {
            if(!path.parent_path().empty()) {
                fs::create_directories(path.parent_path());
            }
            fd = open(path.c_str(), O_CREAT | O_WRONLY,0666);
            if(fd<=0) return false;
            locked = flock(fd, LOCK_EX | LOCK_NB) == 0;
            if(!locked) close(fd);
            return locked;
        } catch (const std::system_error& e) {
            return false;
        }
    }
    void unlock() {
        if(locked) {
            locked=false;
            close(fd);
        }
    }
};
