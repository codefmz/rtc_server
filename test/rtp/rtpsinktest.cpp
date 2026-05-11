#include "gtest/gtest.h"

#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

#include "MediaSource.h"
#include "RtpSink.h"
#include "H264RtpSink.h"
#include "WinCapMediaSource.h"
#include "rtc_utils.h"

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Logger.h"
#include "plog/Init.h"
#include "plog/Log.h"

class RtpSinkTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

};

TEST_F(RtpSinkTest, test_rtp_sink)
{
    {
        auto timerManager = std::make_shared<TimerManager>();
        auto winCapMediaSource = std::make_shared<WinCapMediaSource>(nullptr);
        auto h264RtpSink = std::make_shared<H264RtpSink>(timerManager, winCapMediaSource);
        h264RtpSink->setSendFrameCallback([&](RtpPacket* packet) {
            Utils::dumpToFile("./rtpdata/test.rtp", packet->mBuffer, packet->mSize);
        });
        winCapMediaSource->startCapture();
        h264RtpSink->start();

        std::this_thread::sleep_for(std::chrono::seconds(3));
        h264RtpSink->stop();
    }
    PLOGD << "Test finished.";
}

int main(int argc, char** argv)
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
