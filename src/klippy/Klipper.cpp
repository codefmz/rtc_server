#include "Klipper.h"
#include "RtcPacket.h"
#include "plog/Log.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/time.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_SESSION_ID       1021 //订阅消息id
#define SEND_SOCKET_ID          1023 //发送消息id
#define SOCKET_END_CHAR         0x3
#define WAIT_TIME_PER           1 /* 每次等待时长，秒*/

int Klipper::init(const char *socketPath, Poller *mPoller) {
    if (sock > 0) {
        return 0;
    }
 
    sock = socket(AF_UNIX, SOCK_STREAM, 0); 
    if (sock < 0) {
        PLOGE << "Failed to create socket, errno = " << errno;
        return -1;
    }

    struct sockaddr_un addr = { 0 };
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        PLOGE << "Failed to connect to socket, errno = " << errno;
        goto err;
    }

    mIOEvent = IOEvent::createNew(sock, this);
    mIOEvent->enableReadHandling();
    mIOEvent->setReadCallback(readCallBack);
    mPoller->addIOEvent(mIOEvent);

    if (queryStatus() < 0){ //订阅状态
        PLOGE << "Failed to query status, errno = " << errno;
        goto err;
    }

    return 0;
err:
    close(sock);
    sock = -1;
    return -1;
}

int Klipper::processMove(void *input, void *output)
{
    RTC_Pos_Param *pos = (RTC_Pos_Param*)((uintptr_t)input + INPUT_HEADER_SIZE);
    double x = pos->x;
    double y = pos->y;
    double z = pos->z;

    std::string cmd = "G1 X" + std::to_string(x) + " Y" + std::to_string(y) + " Z" + std::to_string(z) + " F8000";

    if (sendCmd(cmd, true) != 0) {
        PLOGE << "Failed to send cmd, errno = " << errno << " cmd = " << cmd;
        return -1;
    }

    RTC_Output* out = (RTC_Output*)output;
    out->ret = RET_OK;
    out->len = OUTPUT_HEADER_SIZE;

    return 0;
}

int Klipper::processXyZero(void *input, void *output)
{
    std::string cmd = "G28 x y";

    if (sendCmd(cmd, true) != 0) {
        PLOGE << "send cmd error, cmd = " << cmd;
        return -1;
    }

    RTC_Output* out = (RTC_Output*)output;
    out->ret = RET_OK;
    out->len = OUTPUT_HEADER_SIZE;

    return 0;
}

int Klipper::processZZero(void *input, void *output)
{
    std::string cmd = "G28 z";

    if (sendCmd(cmd, true) != 0) {
        PLOGE << "send cmd error, cmd = " << cmd;
        return -1;
    }

    RTC_Output* out = (RTC_Output*)output;
    out->ret = RET_OK;
    out->len = OUTPUT_HEADER_SIZE;

    return 0;
}

void Klipper::readCallBack(void *arg)
{
    Klipper *klippy = (Klipper*)arg;
    klippy->handleRead();
}

void Klipper::handleRead()
{
    ssize_t len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    std::string message(buffer, len);

    PLOGI << "Klipper::readCallBack buffer = " << std::string(buffer, len);

    Json::Value response;
    if (!str2Json(message, response)) {
        return;
    }

    if (response["id"].asInt() == SOCKET_SESSION_ID) {
        parseJson(response);
    } else if (response["id"].asInt() == SEND_SOCKET_ID) { 
        recvQueue.push(response);
    }
}

void Klipper::disconnect() {
    if (sock > 0) {
        close(sock); 
        sock = -1;
    }
}

// 获取打印状态 打印状态 当前打印层  打印时间
int Klipper::queryStatus() {
    std::string subscribeGCode = R"(
    {
        "id": 1021,
        "method": "objects/subscribe",
        "params": {
            "objects": {
                "toolhead": ["position"]
                "toolhead": ["homed_axes"]
            },
            "response_template": {}
        }
    })";

    int ret = sendGCode(subscribeGCode);
    if ( ret < 0 ){
        return -1;
    }

    return 0;
}

int Klipper::sendGCode(std::string& gcode) {
    std::cout << "sendGCode: " << gcode << std::endl;
    gcode.push_back(char(SOCKET_END_CHAR));
    ssize_t sendLen = ::send(sock, gcode.c_str(), gcode.size(), 0);
    if (sendLen < 0 || sendLen != gcode.size()) {
        std::cout << "Failed to send, sendLen:" << sendLen << ",gcodeSize:" << gcode.size() << " ,errno = " << errno << std::endl;
        return -1;
    }

    return 0;
}

bool Klipper::str2Json(const std::string& message, Json::Value& response_json) {
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream stream(message);
    try {
        if (!Json::parseFromStream(reader, stream, &response_json, &errors)) {
            std::cout << "parse json fail : " << errors << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        return false;
    }

    return true;
}

void Klipper::parseJson(const Json::Value &json)
{
    if (!json.isMember("params") ||
        !json["params"].isMember("status") ||
        !json["params"]["status"].isMember("toolhead") ||
        !json["params"]["status"]["toolhead"].isMember("position")) {
        return;
    }

    // if (!json]])
    const Json::Value & position = json["params"]["status"]["toolhead"]["position"];
    double x = position[0].asDouble();
    double y = position[1].asDouble();
    double z = position[2].asDouble();

    if (posChangeCallback) {
        posChangeCallback(x, y, z);
    }

    if (!json["params"]["status"]["toolhead"].isMember("homed_axes")) {
        return;
    }

    homeAxes = json["params"]["status"]["toolhead"]["homed_axes"].asString();
}

int Klipper::parseRecvJson(const Json::Value &json)
{
    if (json.isMember("result") && json["result"].asString() == "ok") {
        return 0;
    }

    return -1;
}

int Klipper::sendCmd(const std::string &cmd, bool block)
{
    static std::string temp = R"({
        "id": 1023, 
        "method": "gcode/script",
        "params": {"script": ")";

    std::string gcode = cmd;
    if (block) {
        gcode.append(" M400\"}}");
    } else {
        gcode.append("\"}}");
    }

    gcode = temp + gcode;

    PLOGI << "Klipper::sendCmd gcode = " << gcode;
    int ret = sendGCode(gcode);
    if (ret != 0) {
        PLOGE << "Failed to send gcode, errno = " << errno << " gcode = " << gcode;
        return -1;
    }

    if (block) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait_for(lock, std::chrono::seconds(RECEIVE_TIMEOUT), [this] { 
            return !recvQueue.empty();
        });

        if (recvQueue.empty()) {
            PLOGE << "Failed to receive response, timeout";
            return -1;
        }
        
        Json::Value& json = recvQueue.front();
        ret = parseRecvJson(json);
        recvQueue.pop();
    }

    return ret;
}


