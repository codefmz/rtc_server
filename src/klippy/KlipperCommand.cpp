#include "KlipperCommand.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/time.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "Poller.h"
#include "RtcPacket.h"

#define SOCKET_SESSION_ID       1021 //订阅消息id
#define SEND_SOCKET_ID          1022 //发送消息id
#define SEND_SOCKET_ONE_WAY_ID  1023 //oneway消息的发送id
#define SOCKET_END_CHAR         0x3
#define WAIT_TIME_PER           1 /* 每次等待时长，秒*/

int KlipperCommand::init(const char *socketPath) {
    if (sock > 0) {
        return 0;
    }
 
    sock = socket(AF_UNIX, SOCK_STREAM, 0); 
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_un addr = { 0 };
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        goto err;
    }

    mIOEvent = IOEvent::createNew(sock, this);
    mIOEvent->enableReadHandling();
    mIOEvent->setReadCallback(readCallBack);
    mPoller->addIOEvent(mIOEvent);

    if (queryStatus() < 0){ //订阅状态
        goto err;
    }

    return 0;
err:
    close(sock);
    sock = -1;
    return -1;
}

int KlipperCommand::processMove(void *input, void *output)
{
    RTC_Pos_Param *pos = (RTC_Pos_Param*)((uintptr_t)input + INPUT_HEADER_SIZE);
    double x = pos->x;
    double y = pos->y;
    double z = pos->z;

    std::string cmd = "G1 X" + std::to_string(x) + " Y" + std::to_string(y) + " Z" + std::to_string(z) + " F8000";

    std::cout << " cmd: " << cmd << std::endl;

    if (sendCmd(cmd, true) != 0) {
        std::cout << "send cmd error." << std::endl;
        return -1;
    }

    RTC_Output* out = (RTC_Output*)output;
    out->ret = RET_OK;
    out->len = OUTPUT_HEADER_SIZE;

    return 0;
}

int KlipperCommand::processXyZero(void *input, void *output)
{
    std::string cmd = "G28 x y";

    if (sendCmd(cmd, true) != 0) {
        std::cout << "send cmd error." << std::endl;
        return -1;
    }

    RTC_Output* out = (RTC_Output*)output;
    out->ret = RET_OK;
    out->len = OUTPUT_HEADER_SIZE;

    return 0;
}

int KlipperCommand::processZZero(void *input, void *output)
{
    std::string cmd = "G28 z";

    if (sendCmd(cmd, true) != 0) {
        std::cout << "send cmd error." << std::endl;
        return -1;
    }

    RTC_Output* out = (RTC_Output*)output;
    out->ret = RET_OK;
    out->len = OUTPUT_HEADER_SIZE;

    return 0;
}

void KlipperCommand::readCallBack(void *arg)
{
    KlipperCommand *klippy = (KlipperCommand*)arg;
    klippy->handleRead();
}

void KlipperCommand::handleRead()
{
    ssize_t len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    std::string message(buffer, len);

    std::cout << "KlipperCommand::readCallBack buffer = " << std::string(buffer, len) << std::endl;

    Json::Value response;
    if (!str2Json(message, response)) {
        return;
    }

    parseJson(response);
}

void KlipperCommand::disconnect() {
    if (sock > 0) {
        close(sock); 
        sock = -1;
    }
}

// 获取打印状态 打印状态 当前打印层  打印时间
int KlipperCommand::queryStatus() {
    std::string subscribeGCode = R"(
    {
        "id": 1021,
        "method": "objects/subscribe",
        "params": {
            "objects": {
                "toolhead": ["position"]
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

#include <iostream>

int KlipperCommand::sendGCode(std::string& gcode) {
    std::cout << "sendGCode: " << gcode << std::endl;
    gcode.push_back(char(SOCKET_END_CHAR));
    ssize_t sendLen = ::send(sock, gcode.c_str(), gcode.size(), 0);
    if (sendLen < 0 || sendLen != gcode.size()) {
        std::cout << "Failed to send, sendLen:" << sendLen << ",gcodeSize:" << gcode.size() << " ,errno = " << errno << std::endl;
        return -1;
    }

    return 0;
}

bool KlipperCommand::str2Json(const std::string& message, Json::Value& response_json) {
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

void KlipperCommand::parseJson(const Json::Value &json)
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
}

int KlipperCommand::sendCmd(std::string &cmd, bool block)
{
    static std::string temp = R"({
        "id": 1023, 
        "method": "gcode/script",
        "params": {"script": ")";

    if (block) {
        cmd.append(" M400\"}}");
    } else {
        cmd.append("\"}}");
    }

    std::string gcode = temp + cmd;
    return sendGCode(gcode);
}


