#include "Base.h"
#include <iostream>
#include <cstring>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <fstream>
std::string Utils::getCameraDevName(const std::string &camName)
{
    DIR* dir = opendir("/sys/class/video4linux/");
    if (!dir) {
        return "";
    }

    std::array<std::string, 2> devNames;
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string path = "/sys/class/video4linux/" + std::string(entry->d_name) + "/name";
        std::ifstream ifs(path);
        std::string device_name;
        if (ifs && std::getline(ifs, device_name)) {
            if (device_name.find(camName) != std::string::npos) {
                devNames[count++] = std::string(entry->d_name);
                if (count == 2) {
                    break;
                }
            }
        }
    }

    closedir(dir);
    if (devNames[0] > devNames[1]) { //readdir 不是按照顺序读取的, usb会加载两个，只有序号较小的那个可以用
        devNames[0] = devNames[1];
    }

    return "/dev/" + devNames[0];
}

void Utils::getVideoDevs(std::vector<std::string> &devs)
{
    FILE *fp = popen("ls /dev/video* 2>/dev/null", "r");
    char buf[1024] = { 0 };
    while (fgets(buf, 1024, fp)) {
        buf[strlen(buf) - 1] = '\0';  
        devs.push_back(std::string(buf));
    }

    pclose(fp);
}
