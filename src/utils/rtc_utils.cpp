#include "rtc_utils.h"
#include "plog/Log.h"
#include "Base.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

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
    const std::filesystem::path videoClassPath("/sys/class/video4linux");
    std::error_code ec;
    if (!std::filesystem::exists(videoClassPath, ec) || !std::filesystem::is_directory(videoClassPath, ec)) {
        return "";
    }

    std::string devName;
    for (const auto &entry : std::filesystem::directory_iterator(videoClassPath, ec)) {
        if (ec) {
            break;
        }

        const auto namePath = entry.path() / "name";
        std::ifstream ifs(namePath);
        std::string device_name;
        if (ifs && std::getline(ifs, device_name)) {
            if (device_name.find(camName) != std::string::npos) {
                const std::string currentDevName = entry.path().filename().string();
                if (devName.empty() || currentDevName < devName) {
                    devName = currentDevName;
                }
            }
        }
    }

    if (devName.empty()) {
        return "";
    }

    return "/dev/" + devName;
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

void Utils::dumpToFile(const std::string &filePath, uint8_t *data, int length)
{
    std::ofstream file(filePath, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        PLOGE << "open file failed, errno = " << errno;
        return;
    }

    file.write(reinterpret_cast<char*>(data), length);
    file.close();
}
