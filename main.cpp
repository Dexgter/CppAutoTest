// auto cpp test, driven by tick

// TODO:
// exception handle
// errorMessages: test name, extra message

#include <iostream>
#include <chrono>
#include <thread>
using namespace std;


/* Tick Variables */

static const int FPS = 30;
static const chrono::microseconds frameTime(1000 / FPS);
// vector is not a good choice, but let's use if just for simplification
static vector<function<void()>> tickFunctions;


/* Auto Test */

class auto_test {
private:
    int successCount;
    int totalCount;
    vector<char*> errorMessages;

    enum test_state {
        off, single, multiple, over
    } state = test_state::off;

    int singleFrameTestFlag;  // record the index traversed in the single-frame tests vector
    vector<bool> multipleFrameTestFlag;  // mark finished multiple-frame tests

    void reset() {
        successCount = 0;
        totalCount = 0;
        errorMessages.clear();
        state = test_state::off;
        singleFrameTestFlag = -1;
        multipleFrameTestFlag.resize(multipleFrameTests.size(), false);
    }

private:
    static auto_test instance ;
    // finish in one frame
    static vector<function<void()>> singleFrameTests;
    // last multiple frames, ends after returning true
    static vector<function<bool()>> multipleFrameTests;

    friend class single_frame_test;
    friend class multiple_frame_test;

public:
    static void start() {
        if(instance.state != test_state::off) {
            cout << "Auto test start failed, the test is already running." << endl;
            return;
        }

        instance.reset();
        instance.state = test_state::single;  // start from single-frame tests
        tickFunctions.push_back(auto_test::tick);
        cout << "Auto test started." << endl;
    }

    static void tick() {
        if(instance.state == test_state::single) {
            // single-frame tests
            auto startTime = chrono::steady_clock::now();
            int& index = instance.singleFrameTestFlag;
            while(chrono::steady_clock::now() - startTime < frameTime) {
                index++;
                if(index >= singleFrameTests.size()) {
                    instance.state = test_state::multiple;
                    return;
                }
                singleFrameTests[index]();
            }
        }
        else if(instance.state == test_state::multiple) {
            // multiple-frame tests
            bool testFlag = false;
            for(int i = 0; i < multipleFrameTests.size(); i++) {
                if(instance.multipleFrameTestFlag[i])
                    continue;
                testFlag = true;
                bool result = multipleFrameTests[i]();
                if(result)
                    instance.multipleFrameTestFlag[i] = true;
            }
            if(!testFlag) {
                instance.state = test_state::over;
                return;
            }
        }
        else if(instance.state == test_state::over) {
            // end
            cout << "Test completed. Result: " 
                << instance.successCount << " / " << instance.totalCount
                << endl;
            tickFunctions.pop_back();
        }
    }

    static void check(bool condition) {
        instance.totalCount++;
        if(condition)
            instance.successCount++;
    }
};
// initialization
auto auto_test::instance = auto_test();
auto auto_test::singleFrameTests = vector<function<void()>>();
auto auto_test::multipleFrameTests = vector<function<bool()>>();

// using this class to declara a single-frame test function
class single_frame_test {
public:
    single_frame_test(const char* name, function<void()> test) {
        auto_test::singleFrameTests.push_back(test);
        cout << "Single-frame test registered: " << name << endl;
    }
};
// using this class to declara a multiple-frame test function
class multiple_frame_test {
public:
    multiple_frame_test(const char* name, function<bool()> test) {
        auto_test::multipleFrameTests.push_back(test);
        cout << "Multiple-frame test registered: " << name << endl;
    }
};


/* Main */

int main() {
    // start auto test
    auto_test::start();

    // imitate game loop
    while(1) {
        auto startTime = chrono::steady_clock::now();
        for(auto tickFunction : tickFunctions) {
            tickFunction();
        }
        auto spentTime = chrono::steady_clock::now() - startTime;
        auto waitTime = chrono::duration_cast<chrono::microseconds>(frameTime - spentTime);
        if (waitTime > chrono::microseconds::zero()) {
            std::this_thread::sleep_for(waitTime);
        }
    }

    return 0;
}


/* Test Cases */

// single-frame test function
static single_frame_test test1("test1", [](){
    auto_test::check(1 == 1);
    auto_test::check(1 == 0);
    cout << "Finished test1." << endl;
});

// multiple-frame test function
static multiple_frame_test test2("test2", []() -> bool {
    static int count = 0;
    count++;
    auto_test::check(true);
    if(count >= 3) {
        cout << "Finished test2, count" << count << endl;
        return true;
    }
    else {
        cout << "Running test2, count: " << count << endl;
        return false;
    }
});