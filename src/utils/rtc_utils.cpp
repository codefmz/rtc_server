#include "rtc_utils.h"
#include "plog/Log.h"
#include "Base.h"
#include "PlatformCompat.h"
#include <array>
#include <memory>
#include <dirent.h>

bool Utils::popenCmd(const std::string &cmd, std::string &output)
{
    output.clear();
    if (cmd.empty()) {
        return false;
    }
    // Use RAII to ensure pipe is always closed
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), &pclose);
    if (!pipe) {
        return false;
    }

    std::array<char, 256> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe.get())) {
        output += buffer.data();
    }

    // Remove trailing newline if exists
    if (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }

    return true;
}

int Utils::killProcessUsingDevice(const std::string &devName)
{
    std::string cmd = "fuser -k " + devName;
    return system(cmd.c_str());
}

int Utils::parseH264(uint8_t *h264Data, int length, int offset,
    std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr, int &num)
{
    int i = 0;
    int before = 0;

    while (i + offset <= length) {
        if (h264Data[i] != 0) {
            i++;
            continue;
        }

        if (h264Data[i + 1] != 0) {
            i += 2;
            continue;
        }

        if (h264Data[i + 2] != 0) {
            i += 3;
            continue;
        }

        if (h264Data[i + 3] != 1) {
            i += 4;
            continue;
        }

        int len = i - before - offset;
        if (len > 0 && len <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
            memcpy(bufferArr[num].data(), h264Data + before + offset, len);
            sizeArr.push_back(len);
            num++;
        } else {
            if (i != 0) {
                PLOGE << "h264Data error, length = " << length << " len = "  << len << " num = " << num;
            }

        }

        before = i;
        i += offset;
    }

    int len = length - before - offset;
    if (len > 0 && len <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
        memcpy(bufferArr[num].data(), h264Data + before + offset, len);
        sizeArr.push_back(len);
        num++;
    } else {
        PLOGE << "h264Data error, length = " << length << " len = "  << len << " num = " << num;
    }

    return 0;
}

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
    if (devNames[0] > devNames[1]) { //readdir 涓嶆槸鎸夌収椤哄簭璇诲彇鐨? usb浼氬姞杞戒袱涓紝鍙湁搴忓彿杈冨皬鐨勯偅涓彲浠ョ敤
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

int Utils::startCode3(uint8_t * buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
        return 1;
    } else {
        return 0;
    }
}

int Utils::startCode4(uint8_t * buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        return 1;
    } else {
        return 0;
    }
}
