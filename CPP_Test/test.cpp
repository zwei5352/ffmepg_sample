#include <iostream>
#include <typeinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <process.h>
#include <time.h>
#include <string>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <thread>
#include <functional>
using namespace std;
class MyTest
{

public:
    MyTest() {
        printf("%s\n", __FUNCTION__);
    }
    static void free(MyTest* ptr) {
        delete ptr;
    }
private:
    ~MyTest() {
        printf("%s\n", __FUNCTION__);
    }

};

MyTest *ttt() {
    return NULL;
}

int main()
{
    std::shared_ptr<MyTest> my(ttt(), [](MyTest *ptr) {MyTest::free(ptr); });
    my.reset(new MyTest(), [](MyTest *ptr) {MyTest::free(ptr); });
    my.reset();
    my.reset();
    

}