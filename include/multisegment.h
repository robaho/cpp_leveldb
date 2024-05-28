#ifndef _MULTI_SEGMENT
#define _MULTI_SEGMENT
#include <vector>
#include <boost/range/adaptor/reversed.hpp>

#include "segment.h"
#include "exceptions.h"
#include "bytebuffer.h"

class MultiSegment;

class MultiSegmentRef : public SegmentRef {
public:
    MultiSegmentRef(MultiSegment* ms) : SegmentRef((Segment*)ms){};
    MultiSegment* multi() const { return (MultiSegment*)SegmentRef::get();}
};


class MultiSegment final : public Segment
{
private:
    const std::vector<SegmentRef> segments;

    MultiSegment(std::vector<SegmentRef> segments) : segments(segments) {}
public:
    static MultiSegmentRef newMultiSegment(std::vector<SegmentRef> segments) {
        auto ref = MultiSegmentRef(new MultiSegment(segments));
        ref->weakRef = ref;
        return ref;
    }
    ByteBuffer put(const Slice& key,const Slice& value) override
    {
        throw IllegalState("put() called on MultiSegment");
    }
    ByteBuffer& get(const Slice& key,ByteBuffer &val) override
    {
        for (auto s : boost::adaptors::reverse(segments))
        {
            s->get(key,val);
            if (!val.empty()) return val;
        }
        return val;
    }
    ByteBuffer remove(const Slice& key) override
    {
        throw IllegalState("remove() called on MultiSegment");
    }
    uint64_t size() override {
        uint64_t size=0;
        for(auto s : segments) {
            size += s->size();
        }
        return size;
    }
    ID lowerID() override {
        throw IllegalState("MultiSegment does not have a lower ID");
    }
    ID upperID() override {
        throw IllegalState("MultiSegment does not have an upper ID");
    }
    void close() override
    {
        throw IllegalState("close() called on MultiSegment");
    }
    void removeSegment() override
    {
        throw IllegalState("removeSegment() called on MultiSegment");
    }
    void removeOnFinalize() override
    {
        throw IllegalState("removeOnFinalize() called on MultiSegment");
    }
    std::vector<std::string> files() override
    {
        return {};
    }
    LookupRef lookup(const Slice& lower, const Slice& upper) override
    {
        std::vector<LookupRef> iterators;
        iterators.reserve(segments.size());
        
        for (auto s : segments)
        {
            iterators.push_back(s->lookup(lower, upper));
        }
        return LookupRef(new Iterator(SegmentRef(weakRef),iterators));
    }

    class Iterator : public LookupIterator {
    private:
        const SegmentRef ms;
        std::vector<LookupRef> iterators;
    public:
        Iterator(SegmentRef ms, std::vector<LookupRef> iterators) :  ms(ms), iterators(iterators) {}        
        Slice peekKey() override {
            throw IllegalState("peekKey called on MultiSegment::Iterator");
        }
        KeyValue& next(KeyValue &kv) override {
            int currentIndex = -1;

            Slice lowest;
            Slice key;

            for (int i = iterators.size()-1;i>=0;i--) {
                auto itr = iterators[i];

                key = itr->peekKey();
                if(key.empty()) continue;

                if(lowest.empty() || key.compareTo(lowest) < 0) {
                    lowest = key;
        			currentIndex = i;
        		}
            }

            if (currentIndex == -1) {
                kv = KeyValue::EMPTY;
                return kv;
	        }

            iterators[currentIndex]->next(kv);

            // ensure that lowest remains stable during next phase
            lowest = kv.key;

            // advance all of the iterators past the current
            for( int i = iterators.size()-1; i >= 0; i--) {
                if (i == currentIndex) {
                    continue;
                }
                auto iterator = iterators[i];
                while(true) {
                    key = iterator->peekKey();
                    if(!key.empty() && lowest.compareTo(key) >= 0) {
                        iterator->next();
                    } else {
                        break;
                    }
                }
            }

            return kv;
        }
    };
};
#endif