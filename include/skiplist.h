#ifndef _SKIPLIST
#define _SKIPLIST

#include <vector>
#include "arena.h"

template <typename K> class SkipList {
private:
    static const int kMaxHeight = 12;
    static const int kBranching = 4;

    struct Node {
        const K key;
        const int height;
        Node(const K& key,const int height) : key(key), height(height) {}
        Node* next(int n) {
            return next_[n];
        }
        void setNext(int n,Node* node) {
            next_[n] = node;
        }
    private:
        std::atomic<Node*> next_[1];
    };

    Arena arena;
    int (* const cmp)(const K&, const K&);
    Node * const head_;
    std::atomic<int> maxHeight;

    Node* findGreaterOrEqual(const K& key,std::vector<Node*> *prev) {
        Node* x = head_;
        int level = maxHeight - 1;
        while(true) {
            Node* next = x->next(level);
            if(keyIsAfterNode(key,next)) {
                x = next;
            } else {
                if(prev!=nullptr) {
                    (*prev)[level] = x;
                }
                if(level==0) {
                    return next;
                } else {
                    level--;
                }
            }
        }
    }

    bool keyIsAfterNode(const K& key,const Node *n) {
        if(n==NULL) return false;
        return (n!=nullptr) && (cmp(n->key,key) < 0);
    }
    bool equal(const K& a, const K& b) {
        return cmp(a,b)==0;
    }

    int randomHeight() {
        auto height = 1;
        for(;height < kMaxHeight && (rand()%kBranching == 0);) {
            height++;
        }
        return height;
    }

    SkipList(SkipList& s) = delete;

    Node* newNode(const K& key,int height) {
        void* const node_memory = arena.allocate(sizeof(Node) + sizeof(std::atomic<Node*>) * (height-1));
        return new (node_memory) Node(key,height);
    }

public:
    SkipList(int (*cmp)(const K&,const K&)) : cmp(cmp), head_(newNode(K(),kMaxHeight)), maxHeight(1) {
        for(int i=0;i<kMaxHeight;i++) {
            head_->setNext(i,nullptr);
        }
    }
    ~SkipList() {
    }

    K put(const K& key) {
        std::vector<Node*> prev(kMaxHeight);
        auto x = findGreaterOrEqual(key,&prev);
        if(x!=NULL && equal(x->key,key)) {
            Node *replace = newNode(key,x->height);
            for(int i=0;i<x->height;i++) {
                replace->setNext(i,x->next(i));
                prev[i]->setNext(i,replace);
            }
            return x->key;
        }
        auto height = randomHeight();
        if(height > maxHeight) {
            for(int i=maxHeight;i<height;i++) {
                prev[i] = head_;
            }
            maxHeight = height;
        }
        x = newNode(key,height);
        for(int i=0;i<height;i++) {
            x->setNext(i,prev[i]->next(i));
            prev[i]->setNext(i,x);
        }
        return K();
    }
    bool contains(const K& key) {
        Node *x = findGreaterOrEqual(key, nullptr);
        return x!=nullptr && equal(key,x->key);
    }
    K get(const K& key) {
        Node *x = findGreaterOrEqual(key, NULL);
        return (x!=nullptr && equal(key,x->key)) ? x->key : K();
    }
    K remove(const K& key) {
        std::vector<Node*> prev(kMaxHeight);
        Node* x = findGreaterOrEqual(key,&prev);
        if (x!=nullptr && equal(x->key,key)) {
            for(int i=0;i<x->height;i++) {
                prev[i]->setNext(i,x->next(i));
            }
            return x->key;
        }
        return K();
    }

    class Iterator {
    friend class SkipList;
    private:
        Node *node = nullptr;
        SkipList * const list;
        Iterator(SkipList *list) : list(list){}
    public:
        K key() {
            return node->key;
        }
        inline bool valid() {
            return node!=nullptr;
        }
        bool seekToFirst() {
            node = list->head_->next(0);
            return valid();
        }
        bool seek(K target) {
            node = list->findGreaterOrEqual(target,nullptr);
            return valid();
        }
        bool next() {
            node = node->next(0);
            return valid();
        }
    };

    Iterator iterator() {
        return Iterator(this);
    }
};
#endif
