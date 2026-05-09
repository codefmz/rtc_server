#include "gtest/gtest.h"
#include "plog/Log.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Logger.h"
#include "plog/Init.h"
#include "plog/Log.h"

#include "TimerManager.h"

class wintimer : public ::testing::Test {
public:
    wintimer() {
    }
    ~wintimer() override {
    }
    void SetUp() override {
    }
    void TearDown() override {
    }
};

Timestamp getNowMs()
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}

void testTask(void* arg)
{
    auto* i = static_cast<std::atomic<int>*>(arg);
    ++(*i);
    auto tid = std::this_thread::get_id();
    PLOGD << "this_thread id: " << tid << " i: " << i->load();
}

TEST_F(wintimer, testTimer)
{
    std::atomic<int> i(100);
    auto timerManager = std::make_shared<TimerManager>();

    TimerEvent event;
    event.setTimeoutCallback(testTask);
    event.setArg(&i);
    auto timerId1 = timerManager->addTimer(event, getNowMs(), 100);
    auto timerId2 = timerManager->addTimer(event, getNowMs() + 50, 100);

    std::this_thread::sleep_for(std::chrono::milliseconds(360));
    timerManager->removeTimer(timerId1);
    timerManager->removeTimer(timerId2);

    EXPECT_GE(i.load(), 105);
}

TEST_F(wintimer, testRemoveTimer)
{
    std::atomic<int> i(100);
    auto timerManager = std::make_shared<TimerManager>();

    TimerEvent event;
    event.setTimeoutCallback(testTask);
    event.setArg(&i);
    auto timerId = timerManager->addTimer(event, getNowMs() + 100, 0);
    timerManager->removeTimer(timerId);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(i.load(), 100);
}

int main(int argc, char** argv) {

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender);


    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
