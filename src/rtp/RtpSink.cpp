#include <arpa/inet.h>
#include "RtpSink.h"
#include "plog/Log.h"

RtpSink::RtpSink(std::shared_ptr<TimerManager> timeManager, std::shared_ptr<MediaSource> mediaSource, int payloadType) : 
    mMediaSource(mediaSource), mSendPacketCallback(NULL), mTimerManager(timeManager), mCsrcLen(0), mExtension(0), mPadding(0),
    mVersion(RTP_VESION), mPayloadType(payloadType), mMarker(0), mSeq(0), mTimestamp(0), mTimerId(0)
{
    mTimerEvent = TimerEvent(this);
    mTimerEvent.setTimeoutCallback(timeoutCallback);
    mSSRC = rand();
}

RtpSink::~RtpSink()
{
    mTimerManager->removeTimedEvent(mTimerId);
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
    if (!frame) {
        return;
    }

    rtpSink->handleFrame(frame);
    rtpSink->mMediaSource->putFrame(frame);
}

void RtpSink::start(int ms)
{
    mTimerId = mTimerManager->addTimer(mTimerEvent, Utils::getCurrentTime(), ms);
}

void RtpSink::stop()
{
    mTimerManager->removeTimedEvent(mTimerId);
}