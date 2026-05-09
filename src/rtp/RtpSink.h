#ifndef _MEDIA_SINK_H_
#define _MEDIA_SINK_H_
#include <string>
#include <stdint.h>

#include "MediaSource.h"
#include "Event.h"
#include "Rtp.h"
#include "TimerManager.h"

class RtpSink
{
public:
    typedef void (*SendPacketCallback)(RtpPacket* mediaPacket);

    RtpSink(std::shared_ptr<TimerManager> timeManager, std::shared_ptr<MediaSource> mediaSource, int payloadType);
    virtual ~RtpSink();

    void setSendFrameCallback(SendPacketCallback cb);

    void stop();

protected:
    virtual void handleFrame(Frame* frame) = 0;
    void sendRtpPacket(RtpPacket* packet);
    void start(int ms);

private:
    static void timeoutCallback(void*);

protected:
    std::shared_ptr<TimerManager> mTimerManager;
    std::shared_ptr<MediaSource> mMediaSource;
    SendPacketCallback mSendPacketCallback;

    uint8_t mCsrcLen;
    uint8_t mExtension;
    uint8_t mPadding;
    uint8_t mVersion;
    uint8_t mPayloadType;
    uint8_t mMarker;
    uint16_t mSeq;
    uint32_t mTimestamp;
    uint32_t mSSRC;

private:
    TimerEvent mTimerEvent;
    TimerId mTimerId;
};

#endif //_MEDIA_SINK_H_