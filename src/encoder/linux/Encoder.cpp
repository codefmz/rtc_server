#include "Encoder.h"
#include "plog/Log.h"
#include "rtc_utils.h"

Encoder::Encoder()
{
}

int Encoder::init(int srcWidth, int srcHeight, int dstWidht, int dstHeight)
{
    return 0;
}

int Encoder::encoder(uint8_t *data, int size, std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr)
{
    int num = 0;
    Utils::parseH264(data, size, mNaluOffset, bufferArr, sizeArr, num);
    return 0;
}
