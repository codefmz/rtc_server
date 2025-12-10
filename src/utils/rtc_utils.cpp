#include "rtc_utils.h"
#include "plog/Log.h"
#include "Base.h"
#include <memory>

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

int Utils::setLight(const std::string &devName, int lightSet)
{
    std::string cmd = "cam_util -i " + devName + " -s " + std::to_string(lightSet);
    std::string str;
    if (!Utils::popenCmd(cmd, str)) {
        PLOGE << "setLight failed.";
        return -1;
    }

    if (str.find("set irlight ok") == std::string::npos) {
        return -1;
    }

    return 0;
}

int Utils::killProcessUsingDevice(const std::string &devName)
{
    std::string cmd = "fuser -k " + devName;
    return  system(cmd.c_str());
}

int Utils::parseH264(uint8_t *h264Data, int length, int offset, std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr, int &num)
{
    int i = 0;
    int before = 0;

    /* startcode : 0 - 3,  SSP : 4 - 14,  startcode : 15 - 18, PSS : 19 - 22， startcode : 23 - 26, 剩余 */
    /*  00000000  00 00 00 01 67 4d 00 1f  89 89 40 3c 01 12 20 00  |....gM....@<.. .|
        00000010  00 00 01 68 ee 3c 80 00  00 00 01 65 88 80 04 00  |...h.<.....e....|
        00000020  00 97 4f 06 3e d0 85 fd  67 ff 73 35 6d 1f eb c3  |..O.>...g.s5m...|
    */
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