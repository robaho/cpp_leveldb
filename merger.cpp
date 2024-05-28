#include <thread>
#include <iostream>

#include "database.h"
#include "waitgroup.h"
#include "merger.h"
#include "multisegment.h"
#include "diskio.h"

/**
 * @brief starts the auto segment merger for the database.
 * @param db it's ok to pass a pointer here, since the Database instance uses the
 * WaitGroup to guard against usage after close
 */
void Merger::autoMerger(Database* db) {
    auto runnable = [=]() {
        WaitGroupDone done(db->wg);
        while(true) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait_for(lock,chrono::seconds(1));
            }
            if(db->closing || db->err) {
                break;
            }
            UseWaitGroup use(db->wg);
            try {
                mergeSegments0(db,db->options.maxSegments);
            } catch(std::exception& ex) {
                db->err = &ex;
            }
        }
    };

    std::thread runner(runnable);
    runner.detach();
}

void Merger::wakeup() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.notify_one();
}

void Merger::mergeSegments0(Database* db,int maxSegments) {
    std::unique_lock<std::mutex> lock(merger, std::try_to_lock);
    if(!lock.owns_lock()) return;

    while(true) {
        std::vector<SegmentRef> segments(db->getState()->segments);
        if(segments.size() <= maxSegments) return;
        int maxMergeSize = std::max(segments.size() / 2, (unsigned long)4);

        auto itr = segments.begin();
        auto smallest = itr;
        while(itr < segments.end()-1) {
            if((*itr)->size() < (*smallest)->size()) smallest = itr;
            itr++;
        }

        auto last = std::min(smallest+maxMergeSize,segments.end());
        std::vector<SegmentRef> mergable(smallest,last);

        auto newseg = mergeSegments1(db->deleter,db->path,mergable,smallest==segments.begin());

        std::unique_lock lock(db->db_lock);

        auto state = db->getState();
        int startAt = smallest - segments.begin();
        int index = startAt;
        for(auto s : mergable) {
            if(s!=state->segments[index++]) throw IllegalState("unexpected segment change");
        }
        for(auto s : mergable) {
            s->removeOnFinalize();
        }

        std::vector<SegmentRef> newsegments;
        for(int i=0;i<startAt;i++) {
            newsegments.push_back(state->segments[i]);
        }
        newsegments.push_back(newseg);
        for(auto itr = state->segments.begin()+(startAt+mergable.size());itr<state->segments.end();itr++) {
            newsegments.push_back(*itr);
        }

        DatabaseState newstate(newsegments,state->memory,MultiSegment::newMultiSegment(copyAndAppend(newsegments,state->memory)));
        db->setState(newstate);

        lock.unlock();
        usleep(std::chrono::microseconds(100ms).count());
    }
}

SegmentRef Merger::mergeSegments1(Deleter &deleter, const std::string& dbpath, std::vector<SegmentRef>& segments,bool purgeDeleted){
    auto lowerId = segments[0]->lowerID();
    auto upperId = (*(segments.end()-1))->upperID();

    auto keyFilename = dbpath+"/keys."+std::to_string(lowerId)+"."+std::to_string(upperId);
    auto dataFilename = dbpath+"/data."+std::to_string(lowerId)+"."+std::to_string(upperId);

    std::vector<std::string> files;

    for(auto s : segments) {
        for(auto file : s->files()) {
            files.push_back(file);
        }
    }

    auto ms = MultiSegment::newMultiSegment(segments);
    auto itr = ms->lookup("","");
    auto seg = writeAndLoadSegment(keyFilename,dataFilename,itr,purgeDeleted);
    deleter.scheduleDeletion(files);
    return seg;

}
