#include "Webrtc.h"
#include "Scheduler.h"
#include "RtpSink.h"
#include "V4l2MediaSource.h"
#include "H264RtpSink.h"
#include "KlipperCommand.h"
#include "Base.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <json/json.h>

void sendPacket(RtpPacket* packet) {
    Webrtc::Instance().sendVideoPacket(packet);
}

int main() {
    rtc::InitLogger(rtc::LogLevel::Error); //webrtc 内部日志
    std::string devName = Utils::getCameraDevName(CAMERA_NAME);
    std::cout << " devName = " << devName << std::endl;

    std::shared_ptr<EventScheduler> scheduler(EventScheduler::createNew(EventScheduler::POLLER_SELECT));
    std::shared_ptr<ThreadPool> pool = std::make_shared<ThreadPool>(1);
    std::shared_ptr<MediaSource> source = std::make_shared<V4l2MediaSource>(pool, "/dev/video0");
    std::shared_ptr<RtpSink> rtpSink = std::make_shared<H264RtpSink>(scheduler, source);
    rtpSink->setSendFrameCallback(sendPacket);
    std::shared_ptr<KlipperCommand> klipper = std::make_shared<KlipperCommand>(scheduler->getPoller());
    klipper->init(KLIPPY_UNIX_SOCKET_PATH);
    klipper->onPosChange([](double x, double y, double z) {
        RTC_Pos_Param param;
        param.x = x;
        param.y = y;
        param.z = z;
        Webrtc::Instance().sendCtlPacket(RET_KLIPPY_POS, (uint8_t*)&param, sizeof(param));
    });

    Webrtc::Instance().addCmdProcessor(CMD_KLIPPY_MOVE, std::bind(&KlipperCommand::processMove, klipper, std::placeholders::_1, std::placeholders::_2));
    Webrtc::Instance().addCmdProcessor(CMD_KLIPPY_XY, std::bind(&KlipperCommand::processXyZero, klipper, std::placeholders::_1, std::placeholders::_2));
    Webrtc::Instance().addCmdProcessor(CMD_KLIPPY_Z, std::bind(&KlipperCommand::processZZero, klipper, std::placeholders::_1, std::placeholders::_2));

    Webrtc::Instance().start(48080);
    scheduler->loop();
    Webrtc::Instance().stop();
}