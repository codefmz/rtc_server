#ifndef _AI_UTILS_H_
#define _AI_UTILS_H_

#include <string>
#include <vector>
#include <cstdint>

namespace Utils {

/* 执行命令， 获取结果 */
bool popenCmd(const std::string& cmd, std::string& output);

int killProcessUsingDevice(const std::string &devName);

int parseH264(uint8_t *h264Data, int length, int offset, std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr, int &num);

/* 根据摄像头名字获取设备名 */
std::string getCameraDevName(const std::string &camName);

void getVideoDevs(std::vector<std::string> &devs);

int startCode3(uint8_t* buf);

int startCode4(uint8_t* buf);

void dumpToFile(const std::string &filePath, uint8_t *data, int length);

uint64_t getNowMs();

};

#endif