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
    Webrtc::Instance().addCmdProcessor(CMD_SHOW_DEVS, [] (void* input, void* output) {
        RTC_Output *rtc = (RTC_Output*)output;
        rtc->ret = RET_OK;

        std::vector<std::string> devs;
        Utils::getVideoDevs(devs);
        std::string devStr;
        for (int i = 0; i < devs.size(); i++) {
            if (i != devs.size() - 1) {
                devStr += devs[i] + "=";
            } else {
                devStr += devs[i];
            }
        }

        rtc->len = devStr.size() + OUTPUT_HEADER_SIZE;
        memcpy(rtc->buf, devStr.c_str(), devStr.size());

        PLOG_DEBUG << "send devs: " << devStr << "  devStr.size() = " << devStr.size();
        return 0;
    });
}

void addShowVideoCmdProcess(std::shared_ptr<V4l2MediaSource> source, std::shared_ptr<H264RtpSink> rtpSink) {
    Webrtc::Instance().addCmdProcessor(CMD_SHOW_VIDEO, [=] (void* input, void* output) {
        RTC_Input *in = (RTC_Input*)input;
        std::string devName((char *)in->buf, in->len - INPUT_HEADER_SIZE);

        PLOGE << "Open show video " << devName;
        int ret = source->init(devName);
        if (ret != 0) {
            PLOG_ERROR << "init dev " << devName << "failed, errno = " << errno;
            return ret;
        }

        rtpSink->start();
        RTC_Output *rtc = (RTC_Output*)output;
        rtc->ret = RET_OK;
        rtc->len = OUTPUT_HEADER_SIZE;

        return 0;
    });

    Webrtc::Instance().addCmdProcessor(CMD_STOP_SHOW_VIDEO, [=] (void* input, void* output) {
        RTC_Input *in = (RTC_Input*)input;
        std::string devName((char *)in->buf, in->len - INPUT_HEADER_SIZE);
        PLOGE << "Stop show video " << devName;

        rtpSink->stop();
        int ret = source->deInit();
        if (ret != 0) {
            PLOG_ERROR << "Dinit dev " << devName << "failed, errno = " << errno;
            return ret;
        }


        RTC_Output *rtc = (RTC_Output*)output;
        rtc->ret = RET_OK;
        rtc->len = OUTPUT_HEADER_SIZE;

        return 0;
    });
}

int main() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./test.txt", 8000, 3);
    plog::init(plog::error, &consoleAppender).addAppender(&fileAppender); 

    std::shared_ptr<EventScheduler> scheduler(EventScheduler::createNew(EventScheduler::POLLER_SELECT));
    std::shared_ptr<Klipper> klipper = std::make_shared<Klipper>();
    std::shared_ptr<ThreadPool> pool = std::make_shared<ThreadPool>(1);
    klipper->init(KLIPPY_UNIX_SOCKET_PATH, scheduler->getPoller());

    std::shared_ptr<V4l2MediaSource> source = std::make_shared<V4l2MediaSource>(pool);
    std::shared_ptr<H264RtpSink> rtpSink = std::make_shared<H264RtpSink>(scheduler, source);

    addKlippyProcessor(klipper);
    addShowDevCmdProcess();
    addShowVideoCmdProcess(source, rtpSink);

    rtpSink->setSendFrameCallback(sendPacket);

    Webrtc::Instance().start(48080);
    scheduler->loop();
    Webrtc::Instance().stop();


    // klipper->onPosChange([](double x, double y, double z) {
    //     RTC_Pos_Param param;
    //     param.x = x;
    //     param.y = y;
    //     param.z = z;
    //     Webrtc::Instance().sendCtlPacket(RET_KLIPPY_POS, (uint8_t*)&param, sizeof(param));
    // });
}