#include "gtest/gtest.h"

#include <atomic>
#include <cstring>
#include <memory>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

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

int initSocket(SOCKET &sock)
{
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }

    return 0;
}

void exitSocket(SOCKET sock)
{
    closesocket(sock);
    WSACleanup();
}

SOCKET g_socket = 0;
SOCKADDR_IN g_dst{};

void sendFrameCallback(RtpPacket* packet) {
    Utils::dumpToFile("./rtpdata/test.rtp", packet->mBuffer, packet->mSize);
    int ret = sendto(g_socket, (const char*)packet->mBuffer, packet->mSize, 0, (sockaddr*)&g_dst, sizeof(g_dst));
    if (ret == SOCKET_ERROR) {
        std::cerr << "sendto failed: " << WSAGetLastError() << "\n";
    } else {
        PLOGD << "sendto " << ret << " bytes";
    }
}

TEST_F(RtpSinkTest, test_rtp_sink)
{
    if (initSocket(g_socket) < 0) {
        return;
    }

    g_dst.sin_family = AF_INET;
    g_dst.sin_port = htons(5004);
    inet_pton(AF_INET, "127.0.0.1", &g_dst.sin_addr);

    auto timerManager = std::make_shared<TimerManager>();
    auto winCapMediaSource = std::make_shared<WinCapMediaSource>(nullptr);
    auto h264RtpSink = std::make_shared<H264RtpSink>(timerManager, winCapMediaSource);
    h264RtpSink->setSendFrameCallback(sendFrameCallback);
    winCapMediaSource->startCapture();
    h264RtpSink->start();

    std::this_thread::sleep_for(std::chrono::seconds(20));
    winCapMediaSource->stopCapture();
    h264RtpSink->stop();
    PLOGD << "Test finished.";

    exitSocket(g_socket);
}

int main(int argc, char** argv)
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
