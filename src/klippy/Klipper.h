#ifndef KLIPPER_COMMAND_H
#define KLIPPER_COMMAND_H

#include <string>
#include <json/json.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include "Event.h"
#include "Poller.h"

constexpr int BUFFER_SIZE = 4096;

#define RECEIVE_TIMEOUT         120 //120s 接收消息超时时间处理

#define KLIPPER_STOP_EXECUTE_ERRNO         0xC0002110 //外部停止打印，停止执行当前命令

//订阅状态
struct SubState {
    std::string print_state;
    std::string webhook_state;
    std::string homed_axes;
};

class Klipper {
public:
    explicit Klipper() {
    }

    ~Klipper(){
        disconnect();
    };

    int init(const char *socketPath, Poller *mPoller);

    void onPosChange(std::function<void(double, double, double)> callback) {
        posChangeCallback = callback;
    }

    int processMove(void *input, void *output);

    int processXyZero(void *input, void *output);

    int processZZero(void *input, void *output);

    const std::string& getHomeAxes() const {
        return homeAxes;
    }

    int sendCmd(const std::string &cmd, bool block);

private:
    static void readCallBack(void *arg);

    void handleRead();

    void disconnect();

    int queryStatus();

    int sendGCode(std::string& gcode);

    bool str2Json(const std::string& message, Json::Value& response);

    void parseJson(const Json::Value& json);

    int parseRecvJson(const Json::Value& json);

private:
    std::mutex mutex;
    std::condition_variable condition;
    std::queue<Json::Value> recvQueue;
    int sock = -1;
    IOEvent* mIOEvent = nullptr;
    Poller* mPoller = nullptr;
    std::function<void(double, double, double)> posChangeCallback; 
    char buffer[BUFFER_SIZE];
    std::string homeAxes;
};

#endif 

