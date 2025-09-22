#include "rtc/rtc.hpp"
#include "RtpSink.h"
#include "RtcPacket.h"
#include <memory>
#include <unordered_map>
#include <list>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>

#include "plog/Log.h"

typedef int SOCKET;

class Webrtc {
public:
    static Webrtc &Instance() {
        static Webrtc instance;
        return instance;
    }
    Webrtc(const Webrtc &) = delete;
    Webrtc &operator=(const Webrtc &) = delete;
    Webrtc(Webrtc &&) = delete;
    Webrtc &operator=(Webrtc &&) = delete;

    void start(int port);

    void recvMedia();

    void stop();

    void addCmdProcessor(RTC_CMD cmd, std::function<int(void *, void *)> processor) {
        cmdProcessor.insert({cmd, processor});
    }

    void sendVideoPacket(RtpPacket *packet) {
        packet->mRtpHeadr->ssrc = ssrc;
        if (track && track->isOpen()) {
            PLOGE << " send video packet size = " << packet->mSize;
            track->send(reinterpret_cast<const std::byte *>(packet->mBuffer), packet->mSize);
        }
    }

    void sendCtlPacket(RTC_Output *output) {
        auto dc = std::atomic_load(&mDc);
        if (dc) {
            dc->send(reinterpret_cast<const std::byte *>(output), output->len);
        }
    }

    void sendCtlPacket(RTC_RET retType, uint8_t *buf, uint32_t len) {
        auto dc = std::atomic_load(&mDc);
        if (dc) {
            outputBuf->ret = retType;
            memcpy(outputBuf->buf, buf, len);
            outputBuf->len = len + OUTPUT_HEADER_SIZE;
            dc->send(reinterpret_cast<const std::byte *>(outputBuf), outputBuf->len);
        }
    }

private:
    Webrtc();
    ~Webrtc();

    void clientConnected(std::shared_ptr<rtc::WebSocket> client);

    void createPeerConnection(std::weak_ptr<rtc::WebSocket> wclient, rtc::message_variant data);

    void peerConnectionSetup(std::shared_ptr<rtc::PeerConnection> pc, std::weak_ptr<rtc::WebSocket> wws, const std::string & id);

    void createRecvSocket();

    void processPacket(rtc::message_variant &data, std::weak_ptr<rtc::DataChannel> wdc);

private:
    std::shared_ptr<rtc::WebSocketServer> server;
    std::shared_ptr<rtc::WebSocket> client;
    std::shared_ptr<rtc::DataChannel> mDc;
    std::shared_ptr<rtc::PeerConnection> pc;
    SOCKET sock = -1;
    std::shared_ptr<rtc::Track> track;
    const std::string LOCAL_ID = "server";
    const rtc::SSRC ssrc = 42;
    std::unordered_map<RTC_CMD, std::function<int(void *, void *)>> cmdProcessor;
    RTC_Output *outputBuf;
};