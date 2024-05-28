#include <iostream>
#include <exception>

#ifndef _EXCEPTIONS
#define _EXCEPTIONS

class DatabaseException : public std::exception {
private:
    const char *whatS;
public:
    DatabaseException(const char *what) {
        this->whatS = what;
    }
    const char* what() const noexcept override {
        return this->whatS;
    }
};

class DatabaseNotFound : public DatabaseException {
public:
    DatabaseNotFound() : DatabaseException("Database not found."){};
};

class DatabaseClosed : public DatabaseException {
public:
    DatabaseClosed() : DatabaseException("Database closed."){};
};

class SnapshotClosed : public DatabaseException {
public:
    SnapshotClosed() : DatabaseException("Snapshot closed."){};
};


class DatabaseInUse : public DatabaseException {
public:
    DatabaseInUse() : DatabaseException("Database in use."){};
};

class DatabaseOpenFailed : public DatabaseException {
public:
    DatabaseOpenFailed(const char *what) : DatabaseException(what){};
};

class IllegalState : public std::logic_error
{
public:
    IllegalState(const char *what) : std::logic_error(what) { };
};

class InvalidKeyLength : public IllegalState {
public:
    InvalidKeyLength() : IllegalState("key length must be > 0 and <= 1024"){}
};

class DatabaseCorrupted : public IllegalState {
public:
    DatabaseCorrupted() : IllegalState("database log file is corrupted"){}
};

class InvalidDatabase : public IllegalState {
public:
    InvalidDatabase() : IllegalState("path is not a valid database"){}
};


#endif
