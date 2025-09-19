#include "Webrtc.h"
#include "Scheduler.h"
#include "RtpSink.h"
#include "V4l2MediaSource.h"
#include "H264RtpSink.h"
#include "Klipper.h"
#include "Base.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <json/json.h>


#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Logger.h"
#include "plog/Init.h"
#include "plog/Log.h"

void sendPacket(RtpPacket* packet) {
    Webrtc::Instance().sendVideoPacket(packet);
}

void addKlippyProcessor(std::shared_ptr<Klipper> &klipper)
{
    Webrtc::Instance().addCmdProcessor(CMD_KLIPPY_MOVE, [=] (void* input, void* output) {
        return klipper->processMove(input, output);
    });

    Webrtc::Instance().addCmdProcessor(CMD_KLIPPY_XY, [=] (void* input, void* output) {
        return klipper->processXyZero(input, output);
    });

    Webrtc::Instance().addCmdProcessor(CMD_KLIPPY_Z, [=] (void* input, void* output) {
        return klipper->processZZero(input, output);
    });
}

void addShowDevCmdProcess() {
    std::vector<std::string> devs;
    Utils::getVideoDevs(devs);

    Webrtc::Instance().addCmdProcessor(CMD_SHOW_DEVS, [&] (void* input, void* output) {
        RTC_Output *rtc = (RTC_Output*)output;
        rtc->ret = RET_OK;
        std::string devStr;
        for (auto dev : devs) {
            devStr += dev + "=";
        }

        rtc->len = devStr.size() + OUTPUT_HEADER_SIZE;
        memcpy(rtc->buf, devStr.c_str(), devStr.size());

        return 0;
    });
}

int main() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3); // Create the 1st appender.
    plog::init(plog::debug, &consoleAppender).addAppender(&fileAppender); 

    std::shared_ptr<EventScheduler> scheduler(EventScheduler::createNew(EventScheduler::POLLER_SELECT));
    std::shared_ptr<Klipper> klipper = std::make_shared<Klipper>();
    std::shared_ptr<ThreadPool> pool = std::make_shared<ThreadPool>(1);
    klipper->init(KLIPPY_UNIX_SOCKET_PATH, scheduler->getPoller());

    addKlippyProcessor(klipper);
    addShowDevCmdProcess();

    Webrtc::Instance().start(48080);
    scheduler->loop();
    Webrtc::Instance().stop();

    // std::shared_ptr<MediaSource> source = std::make_shared<V4l2MediaSource>(pool, "/dev/video0");
    // std::shared_ptr<RtpSink> rtpSink = std::make_shared<H264RtpSink>(scheduler, source);

    // rtpSink->setSendFrameCallback(sendPacket);

    // klipper->onPosChange([](double x, double y, double z) {
    //     RTC_Pos_Param param;
    //     param.x = x;
    //     param.y = y;
    //     param.z = z;
    //     Webrtc::Instance().sendCtlPacket(RET_KLIPPY_POS, (uint8_t*)&param, sizeof(param));
    // });
}