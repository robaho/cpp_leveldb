#pragma once

#include <iostream>
#include <exception>

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
    DatabaseNotFound() : DatabaseException("no database found"){};
};

class DatabaseClosed : public DatabaseException {
public:
    DatabaseClosed() : DatabaseException("database closed"){};
};

class SnapshotClosed : public DatabaseException {
public:
    SnapshotClosed() : DatabaseException("snapshot closed"){};
};


class DatabaseInUse : public DatabaseException {
public:
    DatabaseInUse() : DatabaseException("database in use"){};
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

class DatabaseCorrupted : public IllegalState {
public:
    DatabaseCorrupted() : IllegalState("database corrupted, run repair"){}
};

class InvalidDatabase : public IllegalState {
public:
    InvalidDatabase() : IllegalState("path is not a valid database"){}
};

class EmptyKey: public IllegalState {
public:
    EmptyKey() : IllegalState("key is empty"){}
};

class KeyTooLong: public IllegalState {
public:
    KeyTooLong() : IllegalState("key too long, max 1024"){}
};


class EndOfIterator : public DatabaseException {
public:
    EndOfIterator() : DatabaseException("end of iterator"){};
};
