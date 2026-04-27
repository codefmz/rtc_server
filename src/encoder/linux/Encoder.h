#ifndef _ENCODER_H_
#define _ENCODER_H_ 

#include <vector>
#include <cstdint>

class Encoder {
public:
    Encoder();

    ~Encoder() = default;

    int init(int srcWidth, int srcHeight, int dstWidht, int dstHeight);

    int encoder(uint8_t *data, int size, std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr);

private:
    int mSrcWidth;
    int mSrcHeight;
    int mDstWidth;
    int mDstHeight;
    int mSrcFmt;
    int mDstFmt;
    int mNaluOffset = 4;
};

#endif