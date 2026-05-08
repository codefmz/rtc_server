#include "gtest/gtest.h"
#include "plog/Log.h"
#include <memory>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Logger.h"
#include "plog/Init.h"
#include "plog/Log.h"

#include "TimerManager.h"
#include "SelectPoller.h"

#include "rtc_utils.h"
#include <thread>

class linuxtimer : public ::testing::Test {
public:
    linuxtimer() {
    }
    ~linuxtimer() override {
    }
    void SetUp() override {
    }
    void TearDown() override {
    }
};

void testTask(void* arg)
{
    int *i = (int*)arg;
    ++(*i);
    auto tid = std::this_thread::get_id();
    PLOGD << "this_thread id: " << tid << " i: " << *i;
}

TEST_F(linuxtimer, testTimer)
{
    int i = 100;
    auto poller = std::make_shared<SelectPoller>();
    auto timerManager = std::make_shared<TimerManager>(poller);
    TimerEvent* event = new TimerEvent;
    event->setTimeoutCallback(testTask);
    event->setArg(&i);
    auto timerId1 = timerManager->addTimer(event, Utils::getNowMs(), 1000);

    TimerEvent* event2 = new TimerEvent;
    event2->setTimeoutCallback(testTask);
    event2->setArg(&i);
    auto timerId2 = timerManager->addTimer(event2, Utils::getNowMs() + 500, 1000);
    for (int j = 0; j < 20; ++j) {
        poller->handleEvent();
    }

    // timerManager->removeTimer(timerId1);
    // timerManager->removeTimer(timerId2);

}

int main(int argc, char** argv) {

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender);


    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}