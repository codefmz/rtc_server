#include "Encoder.h"
#include <memoryAdapter.h>
#include "plog/Log.h"
#include "rtc_utils.h"
#include <fstream>

#define ALIGN_16B(x) (((x) + (15)) & ~(15))
const std::string filePath = "./test1.264";

Encoder::Encoder()
{
    memset(&mBaseConfig, 0, sizeof(VencBaseConfig));
    memset(&mBufferParam, 0 ,sizeof(VencAllocateBufferParam));
    memset(&mSpsPpsData, 0, sizeof(VencHeaderData));
    memset(&mBufferParam, 0, sizeof(VencAllocateBufferParam));
}

int Encoder::init(int srcWidth, int srcHeight, int dstWidht, int dstHeight)
{
    mSrcWidth = srcWidth;
    mSrcHeight = srcHeight;
    mSrcFmt = VENC_PIXEL_YUV420SP;
    mDstWidth = dstWidht;
    mDstHeight = 1080;
    mDstFmt = VENC_CODEC_H264;

    int frameNum = 1;
    int pixels = dstWidht * dstHeight;

    VencH264Param h264Param;
    memset(&h264Param, 0, sizeof(VencH264Param));
    h264Param.bEntropyCodingCABAC = 1;
    h264Param.nBitrate = BIT_RATE;
    h264Param.nFramerate = FRAME_RATE;
    h264Param.nCodingMode = VENC_FRAME_CODING;

    h264Param.nMaxKeyInterval = MAX_KEY_FRAME;
    h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
    h264Param.sProfileLevel.nLevel = VENC_H264Level31;
    h264Param.sQPRange.nMinqp = 10;
    h264Param.sQPRange.nMaxqp = 40;

    mpVideoEnc = VideoEncCreate(VENC_CODEC_H264);
    if (mpVideoEnc == NULL) {
        PLOGE << " VideoEncCreate failed!";
        return -1;
    }

    int value = 0;
    int vbvSize = 2 * 1024 * 1024;
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamSetVbvSize, &vbvSize);
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamH264Param, &h264Param);
    value = 0;
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamIfilter, &value);
    value = 0; //degree
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamRotation, &value);
    value = 0;
    VideoEncSetParameter(mpVideoEnc, VENC_IndexParamSetPSkip, &value);

    mBaseConfig.memops = MemAdapterGetOpsS();
    if (mBaseConfig.memops == NULL) {
        PLOGE << "MemAdapterGetOpsS failed!";
        if(mpVideoEnc) {
            VideoEncDestroy(mpVideoEnc);
        }

        return -1;
    }

    CdcMemOpen(mBaseConfig.memops);
    mBaseConfig.nInputHeight = srcHeight;
    mBaseConfig.nInputWidth = srcWidth;
    mBaseConfig.nStride = ALIGN_16B(mSrcWidth);
    mBaseConfig.nDstWidth = mDstWidth;
    mBaseConfig.nDstHeight = mDstHeight;
    mBaseConfig.eInputFormat = (VENC_PIXEL_FMT)mSrcFmt;

    mBufferParam.nSizeY = ALIGN_16B(mBaseConfig.nInputWidth) * ALIGN_16B(mBaseConfig.nInputHeight) + 1024;
    mBufferParam.nSizeC = ALIGN_16B(mBaseConfig.nInputWidth) * ALIGN_16B(mBaseConfig.nInputHeight) / 2 + 1024;
    mBufferParam.nBufferNum = 4;

    if (VideoEncInit(mpVideoEnc, &mBaseConfig) != 0) {
        PLOGE << "MemAdapterGetOpsS failed!";
        if(mpVideoEnc) {
            VideoEncDestroy(mpVideoEnc);
        }
    }

    VideoEncGetParameter(mpVideoEnc, VENC_IndexParamH264SPSPPS, &mSpsPpsData);
    /* alloc input buffer */
    AllocInputBuffer(mpVideoEnc, &mBufferParam);

    return 0;
}

void dumpToFile(VencOutputBuffer &outputBuffer, VencHeaderData &spsPpsData)
{
    std::ofstream file(filePath, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        PLOGE << "open file failed, errno = " << errno;
        return;
    }

    if (outputBuffer.nFlag & VENC_BUFFERFLAG_KEYFRAME) {
        PLOGI << "write sps/pps to file, nLength = " << spsPpsData.nLength;
        file.write((const char*)spsPpsData.pBuffer, spsPpsData.nLength);
    }

    file.write((const char*)outputBuffer.pData0, outputBuffer.nSize0);
    if (outputBuffer.nSize1) {
        file.write((const char*)outputBuffer.pData1, outputBuffer.nSize1);
    }

    PLOGI << "write file success, size = " << outputBuffer.nSize0 + outputBuffer.nSize1;

    file.close();
}

int Encoder::encoder(uint8_t *data, int size, std::vector<std::vector<uint8_t>> &bufferArr, std::vector<int> &sizeArr)
{
    VencInputBuffer inputBuffer;
    memset(&inputBuffer, 0, sizeof(VencInputBuffer));
    GetOneAllocInputBuffer(mpVideoEnc, &inputBuffer);

    memcpy(inputBuffer.pAddrVirY, data, mBaseConfig.nInputWidth * mBaseConfig.nInputHeight);
    memcpy(inputBuffer.pAddrVirC, data + mBaseConfig.nInputWidth * mBaseConfig.nInputHeight, mBaseConfig.nInputWidth * mBaseConfig.nInputHeight / 2);

    inputBuffer.bEnableCorp = 0;
    inputBuffer.sCropInfo.nLeft =  240;
    inputBuffer.sCropInfo.nTop =  240;
    inputBuffer.sCropInfo.nWidth =  240;
    inputBuffer.sCropInfo.nHeight =  240;

    FlushCacheAllocInputBuffer(mpVideoEnc, &inputBuffer);

    /* encode */
    AddOneInputBuffer(mpVideoEnc, &inputBuffer);
    VideoEncodeOneFrame(mpVideoEnc);

    /* return inputbuffer to encoder */
    AlreadyUsedInputBuffer(mpVideoEnc,&inputBuffer);
    ReturnOneAllocInputBuffer(mpVideoEnc, &inputBuffer);

    VencOutputBuffer outputBuffer;
    memset(&outputBuffer, 0, sizeof(VencOutputBuffer));
    int ret = GetOneBitstreamFrame(mpVideoEnc, &outputBuffer);
    if (ret == -1) {
        PLOGE << " GetOneBitstreamFrame failed.";
        return -1;
    }

    int num = 0;
    if (outputBuffer.nFlag & VENC_BUFFERFLAG_KEYFRAME) {
        Utils::parseH264((uint8_t *)mSpsPpsData.pBuffer, mSpsPpsData.nLength, mNaluOffset, bufferArr, sizeArr, num);
    }

    //解析H264 获取NALU
    Utils::parseH264((uint8_t *)outputBuffer.pData0, outputBuffer.nSize0, mNaluOffset, bufferArr, sizeArr, num);
    if (outputBuffer.nSize1) {
        Utils::parseH264((uint8_t *)outputBuffer.pData1, outputBuffer.nSize1, mNaluOffset, bufferArr, sizeArr, num);
    }

    /* return outbuffer */
    FreeOneBitStreamFrame(mpVideoEnc, &outputBuffer);

    return 0;
}
