#pragma once

#include <iostream>
#include <streambuf>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include "exceptions.h"

/**
 * @brief stream buffer that persists to a file descriptor, so that the 'O_SYNC' option may be applied
 * 
 */
class FdFileBuf final : public std::stringbuf {
private:
    int fd = -1;
    bool syncAfterFlush = false;
public:
    FdFileBuf(){}
    ~FdFileBuf() { close(); }
    int sync() override {
        if(!isOpen()) return 0;
        auto s = str();
        if(write(fd,s.c_str(),s.length())!=s.length()) {
            throw IllegalState("unable to write file");
        }
        if(syncAfterFlush) {
            if(fcntl(fd,F_FULLFSYNC,1)!=0) {
                fsync(fd);
            }
        }
        str("");
        return 0;
    }
    void open(const std::string& path,int mode) {
        fd = ::open(path.c_str(),mode,0660);
        if(mode & O_SYNC) {
            syncAfterFlush=true;
        }
    }
    bool isOpen() { return fd!=-1; }
    void close() {
        if(isOpen()) {
            sync();
            ::close(fd);
            fd=-1;
        }
    }
};
