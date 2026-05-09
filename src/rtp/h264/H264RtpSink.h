#ifndef _H264_MEDIA_SINK_H_
#define _H264_MEDIA_SINK_H_

#include <stdint.h>
#include "RtpSink.h"

class H264RtpSink : public RtpSink
{
public:
    H264RtpSink(std::shared_ptr<TimerManager> timerManager, std::shared_ptr<MediaSource> mediaSource);
    virtual ~H264RtpSink();

    virtual void handleFrame(Frame* frame);

    void start();

private:
    RtpPacket mRtpPacket;
    int mClockRate;
    int mFps;
};

#endif //_H264_MEDIA_SINK_H_