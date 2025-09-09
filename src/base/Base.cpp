#include "Base.h"
#include <dirent.h>
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