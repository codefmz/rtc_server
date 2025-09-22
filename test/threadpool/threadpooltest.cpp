#include "gtest/gtest.h"
#include "ThreadPool.h"
#include "plog/Log.h"
#include <memory>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Logger.h"
#include "plog/Init.h"
#include "plog/Log.h"


class threadpool : public ::testing::Test {
public:
    threadpool() {
    }
    ~threadpool() override {
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
    pthread_t tid = pthread_self();
    PLOGD << "pthread_self id: " << tid;
    PLOGD << " gettid = " << gettid();
}

TEST_F(threadpool, testThreadPoolStartEnd)
{
    std::shared_ptr<ThreadPool> pool = std::make_shared<ThreadPool>(1);

    int num = 0;
    Task task;
    task.setTaskCallback(testTask, &num);

    for (int i = 0; i < 100; i++) {
        pool->createThreads();
        pool->addTask(task);
        pool->cancelThreads();
    }

    PLOGD << "num: " << num;
}

int main(int argc, char** argv) {

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender);


    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}