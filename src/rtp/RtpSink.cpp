#include <arpa/inet.h>
#include "RtpSink.h"
#include "plog/Log.h"

RtpSink::RtpSink(std::shared_ptr<EventScheduler> scheduler, std::shared_ptr<MediaSource> mediaSource, int payloadType) : 
    mMediaSource(mediaSource), mSendPacketCallback(NULL), mScheduler(scheduler), mCsrcLen(0), mExtension(0), mPadding(0),
    mVersion(RTP_VESION), mPayloadType(payloadType), mMarker(0), mSeq(0), mTimestamp(0), mTimerId(0)
{
    mTimerEvent = new TimerEvent(this);
    mTimerEvent->setTimeoutCallback(timeoutCallback);
    mSSRC = rand();
}

RtpSink::~RtpSink()
{
    mScheduler->removeTimedEvent(mTimerId);
    delete mTimerEvent;
}

void RtpSink::setSendFrameCallback(SendPacketCallback cb)
{
    mSendPacketCallback = cb;
}

void RtpSink::sendRtpPacket(RtpPacket* packet)
{
    RtpHeader* rtpHead = packet->mRtpHeadr;
    rtpHead->csrcLen = mCsrcLen;
    rtpHead->extension = mExtension;
    rtpHead->padding = mPadding;
    rtpHead->version = mVersion;
    rtpHead->payloadType = mPayloadType;
    rtpHead->marker = mMarker;
    rtpHead->seq = htons(mSeq);
    rtpHead->timestamp = htonl(mTimestamp);
    rtpHead->ssrc = htonl(mSSRC);
    packet->mSize += RTP_HEADER_SIZE;

    if (mSendPacketCallback) {
        mSendPacketCallback(packet);
    }
}

void RtpSink::timeoutCallback(void* arg)
{
    RtpSink* rtpSink = (RtpSink*)arg;
    Frame* frame = rtpSink->mMediaSource->getFrame();
    if(!frame) {
        return;
    }

    rtpSink->handleFrame(frame);
    rtpSink->mMediaSource->putFrame(frame);
}

void RtpSink::start(int ms)
{
    mTimerId = mScheduler->addTimedEventRunEvery(mTimerEvent, ms);
}

void RtpSink::stop()
{
    mScheduler->removeTimedEvent(mTimerId);
}