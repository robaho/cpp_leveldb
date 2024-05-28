#ifndef _WAIT_GROUP
#define _WAIT_GROUP

#include <mutex>
#include <condition_variable>

class WaitGroup {
private:
    int n = 0;
    std::mutex mtx;
    std::condition_variable cv;

public:
    void add(int count) {
        std::unique_lock<std::mutex> lock(mtx);
        n += count;
    }

    void done() {
        std::unique_lock<std::mutex> lock(mtx);
        n--;
        cv.notify_one();
    }

    void waitEmpty() {
        std::unique_lock<std::mutex> lock(mtx);
        while (n > 0) {
            cv.wait(lock);
        }
    }
    int count() {
        return n;
    }
};

struct WaitGroupDone {
private:
    WaitGroup& wg;
public:
    WaitGroupDone(WaitGroup &wg) : wg(wg) {
    }
    ~WaitGroupDone() {
        wg.done();
    }
};

struct UseWaitGroup {
private:
    WaitGroup& wg;
public:
    UseWaitGroup(WaitGroup &wg) : wg(wg) {
        wg.add(1);
    }
    ~UseWaitGroup() {
        wg.done();
    }
};

#endif


