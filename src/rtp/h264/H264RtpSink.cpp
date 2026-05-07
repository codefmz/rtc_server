#include <stdio.h>
#include <string.h>
#include <iostream>
#include "H264RtpSink.h"
#include "plog/Log.h"

H264RtpSink::H264RtpSink(std::shared_ptr<EventScheduler> scheduler, std::shared_ptr<MediaSource> mediaSource) :
    RtpSink(scheduler, mediaSource, RTP_PAYLOAD_TYPE_H264), mClockRate(90000), mFps(mediaSource->getFps())
{
    start();
}

H264RtpSink::~H264RtpSink()
{
}

void H264RtpSink::handleFrame(Frame* frame)
{
    RtpHeader* rtpHeader = mRtpPacket.mRtpHeadr;
    uint32_t frameSize = 0;
    uint8_t *data = nullptr;
    for (int i = 0; i < frame->mSizeArr.size(); i++) {
        data = frame->mBufferArr[i].data();
        frameSize = frame->mSizeArr[i];

        uint8_t naluType = data[0];
        if (frameSize <= RTP_MAX_PKT_SIZE) {
            memcpy(rtpHeader->payload, data, frameSize);
            mRtpPacket.mSize = frameSize;
            sendRtpPacket(&mRtpPacket);
            mSeq++;

            if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) {// 如果是SPS、PPS就不需要加时间戳
                continue;
            }
        } else {
            int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
            int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
            int i, pos = 1;

            /* 发送完整的包 */
            for (i = 0; i < pktNum; i++) {
                /*
                *     FU Indicator
                *    0 1 2 3 4 5 6 7
                *   +-+-+-+-+-+-+-+-+
                *   |F|NRI|  Type   |
                *   +---------------+
                * */
                rtpHeader->payload[0] = (naluType & 0x60) | 28; //(naluType & 0x60)表示nalu的重要性，28表示为分片

                /*
                *      FU Header
                *    0 1 2 3 4 5 6 7
                *   +-+-+-+-+-+-+-+-+
                *   |S|E|R|  Type   |
                *   +---------------+
                * */
                rtpHeader->payload[1] = naluType & 0x1F; //naluType & 0x1F表示nalu的类型

                if (i == 0) { //第一包数据
                    rtpHeader->payload[1] |= 0x80; // start
                } else if (remainPktSize == 0 && i == pktNum - 1) {//最后一包数据
                    rtpHeader->payload[1] |= 0x40; // end
                }
                memcpy(rtpHeader->payload + 2, data + pos, RTP_MAX_PKT_SIZE);
                mRtpPacket.mSize = RTP_MAX_PKT_SIZE + 2;
                sendRtpPacket(&mRtpPacket);

                mSeq++;
                pos += RTP_MAX_PKT_SIZE;
            }

            /* 发送剩余的数据 */
            if (remainPktSize > 0) {
                rtpHeader->payload[0] = (naluType & 0x60) | 28;
                rtpHeader->payload[1] = naluType & 0x1F;
                rtpHeader->payload[1] |= 0x40; //end

                memcpy(rtpHeader->payload + 2, data + pos, remainPktSize);
                mRtpPacket.mSize = remainPktSize + 2;
                sendRtpPacket(&mRtpPacket);

                mSeq++;
            }
        }
    }
    mTimestamp += mClockRate/mFps;
}

void H264RtpSink::start()
{
    RtpSink::start(1000 / mFps);
}
