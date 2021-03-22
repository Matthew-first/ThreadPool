#include <iostream>
#include "ThreadPool.hpp"
using namespace std;
int main() {
    ThreadPool::ThreadPool pool(1);
    int i = 0;
    auto s=[]() {
        for (int i = 0; i < 10000000; i++)
        {

        }

        return "finish";
    }
    ;
    auto ss=pool.add(s);
    pool.close();
    for (int i = 0; i < 10000000; i++)
        ;
    cout << "fin" << endl;
    pool.open();
    cout << "fin1" << endl;
    auto sss=pool.add(s);
    cout << ss.get()<<' '<<sss.get() << endl;
    return 0;
}
