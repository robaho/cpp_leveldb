#include <memory>
#include <exception>

struct A {
    int val;
};

int main() {

    A *a = new A();

    std::shared_ptr<A> sp(a);

    std::weak_ptr<A> wp = sp;

    std::shared_ptr<A> sp2(wp);

    if(sp.use_count()!=2) throw std::exception();
}