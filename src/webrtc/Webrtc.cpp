#include "Webrtc.h"
#include "rtc/rtc.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <nlohmann/json.hpp>
#include <unistd.h>

using namespace rtc;
using namespace std;
using nlohmann::json;

const int BUFFER_SIZE = 2048;

template <class T>
weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { 
    return ptr; 
}

void Webrtc::start(int port)
{
    WebSocketServer::Configuration serverConfig;
    serverConfig.port = port;
    serverConfig.enableTls = false;
    serverConfig.bindAddress = "0.0.0.0";
    serverConfig.maxMessageSize = 4096;
    server = make_shared<WebSocketServer>(std::move(serverConfig));

    server->onClient([](shared_ptr<WebSocket> client) {
        Webrtc::Instance().clientConnected(client);
    });
}

void Webrtc::stop()
{
    server->stop();
}

Webrtc::Webrtc()
{
    createRecvSocket();
    outputBuf = new RTC_Output;
}

Webrtc::~Webrtc()
{
    if (sock != -1) {
        close(sock);
    }
    delete outputBuf;
}

void Webrtc::clientConnected(std::shared_ptr<WebSocket> client)
{
    if (auto addr = client->remoteAddress()) {
        cout << "WebSocketServer: Client remote address is " << *addr << endl;
    }

    client->onOpen([wclient = make_weak_ptr(client)]() {
        cout << "WebSocketServer: Client connection open" << endl;
        if (auto client = wclient.lock()) {
            if (auto path = client->path()) {
                cout << "WebSocketServer: Requested path is " << *path << endl;
            }
        }
    });

    client->onClosed([this]() {
        cout << "WebSocketServer: Client connection closed" << endl;
        this->client = nullptr;
    });

    //这里使用weak_ptr 的原因是因为 client 持有改lambda 回调对象， 回调对象内部又用 client 作为成员，会导致循环引用从而无法释放
    client->onMessage([wclient = make_weak_ptr(client)](rtc::message_variant message) { 
        Webrtc::Instance().createPeerConnection(wclient, message);
    });

    this->client = client;
}

void Webrtc::createPeerConnection(std::weak_ptr<rtc::WebSocket> wclient, rtc::message_variant message)
{
    auto client = wclient.lock();
    if (!client) {
        cout << "WebSocketServer: Client is gone" << endl;
        return;
    }

    if (std::holds_alternative<std::string>(message)) {
        cout << "WebSocketServer: Received string message: " << std::get<std::string>(message) << endl;
        json data = json::parse(std::get<std::string>(message));
        auto it = data.find("id");
        if (it == data.end()) {
            return;
        }

        auto id = it->get<std::string>();
        it = data.find("type");
        if (it == data.end()) {
            return;
        }

        auto type = it->get<std::string>();
        if (type == "offer") {
            cout << "Answering to " + id << endl;
            rtc::Configuration config;
            pc = std::make_shared<rtc::PeerConnection>(config);

            rtc::Description::Video media("video", rtc::Description::Direction::SendOnly);
            media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
            media.addSSRC(ssrc, "video-send");
            auto trackTmp = pc->addTrack(media);
            std::atomic_store(&track, trackTmp);

            peerConnectionSetup(pc, wclient, id);
        }

        if (type == "offer" || type == "answer") {
            auto sdp = data["description"].get<std::string>();
            pc->setRemoteDescription(rtc::Description(sdp, type));
        } else if (type == "candidate") {
            auto sdp = data["candidate"].get<std::string>();
            auto mid = data["mid"].get<std::string>();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
        }
    } else {
        cout << "WebSocketServer: Received bytes message" << endl;
    }
}

void Webrtc::createRecvSocket()
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6000);

    if (bind(sock, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind UDP socket on 127.0.0.1:6000");
    }

    int rcvBufSize = 212992;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char *>(&rcvBufSize), sizeof(rcvBufSize));
}

void Webrtc::processPacket(rtc::message_variant &data, std::weak_ptr<rtc::DataChannel> wdc)
{
    if (std::holds_alternative<std::string>(data)) {
        std::cout << " Webrtc error, recved std::string " << std::endl;
        return;
    }

    std::shared_ptr<rtc::DataChannel> dc = wdc.lock();
    if (!dc) {
        return;
    }

    outputBuf->ret = RET_ERR;
    outputBuf->len = OUTPUT_HEADER_SIZE;
    RTC_Input *input = reinterpret_cast<RTC_Input *>(std::get<binary>(data).data());

    auto iter = cmdProcessor.find(input->cmd);
    if (iter == cmdProcessor.end()) {
        dc->send((std::byte*)outputBuf, outputBuf->len);
        std::cout << " cmd not found cmd: " << input->cmd << std::endl;
        return;
    }

    int ret = iter->second(input, outputBuf);
    if (ret < 0) {
        dc->send((std::byte*)outputBuf, outputBuf->len);
        std::cout << " cmd process failed cmd: " << input->cmd << std::endl;
        return;
    }

    std::cout << " outputbuf->ret = " << outputBuf->ret << std::endl;
    dc->send((std::byte*)outputBuf, outputBuf->len);
}

void Webrtc::recvMedia()
{
    //Receive from UDP
    char buffer[BUFFER_SIZE];
    int len;
    while ((len = recv(sock, buffer, BUFFER_SIZE, 0)) >= 0) {
        if (len < sizeof(rtc::RtpHeader) || !track|| !track->isOpen()) {
            continue;
        }
        auto rtp = reinterpret_cast<rtc::RtpHeader *>(buffer);
        rtp->setSsrc(ssrc);
        track->send(reinterpret_cast<const std::byte *>(buffer), len);
    }
}

void Webrtc::peerConnectionSetup(std::shared_ptr<rtc::PeerConnection> pc, std::weak_ptr<rtc::WebSocket> wws, const std::string & id)
{
    pc->onStateChange([](rtc::PeerConnection::State state) {
        std::cout << "PeerConnection State: " << state << std::endl;
    });

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering State: " << state << std::endl;
    });

    pc->onLocalDescription([this, wws](rtc::Description description) {
        json message = {{"id", LOCAL_ID},
                        {"type", description.typeString()},
                        {"description", std::string(description)}};

        if (auto ws = wws.lock()) {
            ws->send(message.dump());
        }
    });

    pc->onLocalCandidate([this, wws](rtc::Candidate candidate) {
        json message = {{"id", LOCAL_ID},
                        {"type", "candidate"},
                        {"candidate", std::string(candidate)},
                        {"mid", candidate.mid()}};

        if (auto ws = wws.lock()) {
            ws->send(message.dump());
        }
    });

    pc->onDataChannel([id, this](shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel from " << id << " received with label \"" << dc->label() << "\"" << std::endl;
        dc->onOpen([id, wdc = make_weak_ptr(dc)]() {
            std::cout << "DataChannel from " << id << " opened" << std::endl;
        });

        dc->onClosed([id, this]() { 
            std::cout << "DataChannel from " << id << " closed" << std::endl;
            this->mDc = nullptr;
        });

        dc->onMessage([id, wdc = make_weak_ptr(dc)](auto data) {
            Webrtc::Instance().processPacket(data, wdc);
        });

        std::atomic_store(&mDc, dc);
    });
}
