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

#include "V4l2.h"

#include "nozzlecalib.h"
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

        PLOGD << "Open show video " << devName;
        int ret = source->init(devName);
        if (ret != 0) {
            PLOG_ERROR << "init dev " << devName << "failed, errno = " << errno;
            return ret;
        }

        RTC_Output *rtc = (RTC_Output*)output;
        rtc->ret = RET_OK;
        rtc->len = OUTPUT_HEADER_SIZE;

        return 0;
    });

    Webrtc::Instance().addCmdProcessor(CMD_STOP_SHOW_VIDEO, [=] (void* input, void* output) {
        RTC_Input *in = (RTC_Input*)input;
        std::string devName((char *)in->buf, in->len - INPUT_HEADER_SIZE);
        PLOGD << "Stop show video " << devName;

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

void addNozzleCalibCmdProcess(std::shared_ptr<NozzleCalib> &calib) {
    Webrtc::Instance().addCmdProcessor(CMD_CALIB_OFFSET_PRE, [=] (void* input, void* output) {
        RTC_Output *out = (RTC_Output*)output;
        out->ret = RET_OK;

        RTC_Input *in = (RTC_Input*)input;
        RTC_CALIB_OFFSET_PARAM *param = (RTC_CALIB_OFFSET_PARAM *)in->buf;
        int ret = calib->nozzleOffsetPre(param->nozzleT, param->posX, param->posY, param->posZ);
        if (ret != 0) {
            PLOG_ERROR << "nozzleOffsetPre failed.";
            out->ret = RET_ERR;
            return -1;
        }

        out->len = OUTPUT_HEADER_SIZE;
        return 0;
    });

    Webrtc::Instance().addCmdProcessor(CMD_CALIB_OFFSET_POST, [=] (void* input, void* output) {
        RTC_Output *out = (RTC_Output*)output;
        out->ret = RET_OK;

        RTC_Input *in = (RTC_Input*)input;
        RTC_CALIB_OFFSET_PARAM *param = (RTC_CALIB_OFFSET_PARAM *)in->buf;
        int ret = calib->nozzleOffsetPost(param->nozzleT);
        if (ret != 0) {
            PLOG_ERROR << "nozzleOffsetPost failed";
            out->ret = RET_ERR;
            return -1;
        }

        out->len = OUTPUT_HEADER_SIZE;
        return 0;
    });

    Webrtc::Instance().addCmdProcessor(CMD_CALIB_RT, [=](void *input, void *output) {
        RTC_Output *out = (RTC_Output*)output;
        out->ret = RET_OK;

        RTC_Input *in = (RTC_Input*)input;
        RTC_CALIB_RT_PARAM *param = (RTC_CALIB_RT_PARAM *)in->buf;
        int ret = calib->nozzleRt(param->cnt, param->posX, param->posY, param->posZ);
        if (ret != 0) {
            PLOG_ERROR << "nozzleRt failed";
            out->ret = RET_ERR;
            return -1;
        }

        out->len = OUTPUT_HEADER_SIZE;
        return 0;
    });
}

int main() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("./server.log", 80000000, 100);
    plog::init(plog::debug, &fileAppender).addAppender(&consoleAppender); 

    // V4l2MediaSourceParam param;
    // param.width = 1920;
    // param.height = 1080;
    // param.fmt = V4L2_PIX_FMT_H264;
    // param.fps = 30;
    // param.driverType = V4L2_CAP_VIDEO_CAPTURE;

    std::shared_ptr<ThreadPool> pool = std::make_shared<ThreadPool>(1);
    std::shared_ptr<EventScheduler> scheduler(EventScheduler::createNew(EventScheduler::POLLER_SELECT));

    V4l2MediaSourceParam param;
    param.width = 1920;
    param.height = 1088;
    param.fmt = V4L2_PIX_FMT_NV12;
    param.fps = 30;
    param.driverType = V4L2_CAP_VIDEO_CAPTURE_MPLANE;

    std::shared_ptr<V4l2MediaSource> source = std::make_shared<V4l2MediaSource>(pool, param);
    std::shared_ptr<H264RtpSink> rtpSink = std::make_shared<H264RtpSink>(scheduler, source);
    std::shared_ptr<Klipper> klipper = std::make_shared<Klipper>();
    klipper->init(KLIPPY_UNIX_SOCKET_PATH, scheduler->getPoller());
    rtpSink->setSendFrameCallback(sendPacket);
    klipper->onPosChange([](double x, double y, double z) {
        RTC_Pos_Param param;
        param.x = x;
        param.y = y;
        param.z = z;
        Webrtc::Instance().sendCtlPacket(RET_KLIPPY_POS, (uint8_t*)&param, sizeof(param));
    });

    std::shared_ptr<NozzleCalib> nozzleCalib = std::make_shared<NozzleCalib>(klipper);
    addKlippyProcessor(klipper);
    addShowDevCmdProcess();
    addShowVideoCmdProcess(source, rtpSink);
    addNozzleCalibCmdProcess(nozzleCalib);


    Webrtc::Instance().start(48080);
    scheduler->loop();
    Webrtc::Instance().stop();
}