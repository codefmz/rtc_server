#include "gtest/gtest.h"
#include "WinCapMediaSource.h"
#include "plog/Log.h"
#include <memory>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Logger.h"
#include "plog/Init.h"
#include "plog/Log.h"


class winMedia : public ::testing::Test {
public:
    winMedia() {
    }
    ~winMedia() override {
    }
    void SetUp() override {
    }
    void TearDown() override {
    }
};

TEST_F(winMedia, testThreadPoolStartEnd)
{
    auto mediaSource = std::make_shared<WinCapMediaSource>(nullptr, 30);
    mediaSource->startCapture();
}

int main(int argc, char** argv) {

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender);


    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}