#ifndef _AI_UTILS_H_
#define _AI_UTILS_H_

#include <string>
#include <vector>

namespace Utils {

/* 执行命令， 获取结果 */
bool popenCmd(const std::string& cmd, std::string& output);

int setLight(const std::string &devName, int lightSet);

int killProcessUsingDevice(const std::string &devName);

int parseH264(uint8_t *h264Data, int length, int offset, std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr, int &num);
};

#endif