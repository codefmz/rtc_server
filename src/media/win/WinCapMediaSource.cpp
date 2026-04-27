#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <string.h>
#include <iostream>
#include "V4l2MediaSource.h"
#include "plog/Log.h"
#include "rtc_utils.h"

WinCapMediaSource::WinCapMediaSource(std::shared_ptr<ThreadPool> pool, WinCapMediaSourceParam &param) : MediaSource(pool)
{
    setFps(param.fps);
    mHeight = param.height;
    mWidth = param.width;
    mDriverType = param.driverType;
    mFmt = param.fmt;

    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mFrameInputQueue.push(&mFrames[i]);
    }

    mPool->createThreads();
    mEncoder = std::make_unique<Encoder>();
}

V4l2MediaSource::~V4l2MediaSource()
{
    mPool->cancelThreads();
    videoExit();
}

int V4l2MediaSource::init(const std::string &dev)
{
    auto iter = devFdMap.find(dev);
    if (iter != devFdMap.end()) {
        setFd(iter->second);
        mV4l2Buf = mFdV4l2BufMap[iter->second];
        for(int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
            mPool->addTask(mTask);
        }

        PLOGD << "reopen " << dev << " fd = " << iter->second << " mV4lfBuf = " << mV4l2Buf;
        return 0;
    }

    Utils::killProcessUsingDevice(dev);
    int fd = -1;
    int ret = videoInit(dev, fd);
    if (ret != 0) {
        return ret;
    }

    if (mEncoder->init(mWidth, mHeight, mWidth, mHeight) != 0) {
        PLOGE << "encoder init failed";
        return -1;
    }

    setFd(fd);
    devFdMap.try_emplace(dev, fd);
    mFdV4l2BufMap.try_emplace(fd, mV4l2Buf);
    PLOGD << " open " << dev << " fd = " << fd << " mV4lfBuf = " << mV4l2Buf;
    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mPool->addTask(mTask);
    }

    return 0;
}

int V4l2MediaSource::deInit()
{
    setFd(-1);
    return 0;
}

void V4l2MediaSource::readFrame()
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mFrameInputQueue.empty()) {
        PLOGE << "V4l2MediaSource::readFrame mAVFrameInputQueue.empty()";
        return;
    }

    if (mFd < 0) {
        PLOGE << " fd < 0";
        return;
    }

    Frame* frame = mFrameInputQueue.front();
    for (auto i = 0; i < 1; ++i) {
        int ret = v4l2_poll(mFd);
        if (ret < 0) {
            PLOGE << " poll fail, errno = " << errno;
            return;
        }

        mV4l2BufUnit = v4l2_dqbuf(mFd, mV4l2Buf);
        if (!mV4l2BufUnit) {
            PLOGE << " dqbuf fail, errno = " << errno;
            return;
        }

        mEncoder->encoder((uint8_t*)mV4l2BufUnit->start[0], mV4l2BufUnit->bytesused, frame->mBufferArr, frame->mSizeArr);
        ret = v4l2_qbuf(mFd, mV4l2BufUnit, mV4l2Buf);
        if (ret < 0) {
            PLOGE << " qbuf fail, errno = " << errno;
            return;
        }
    }

    mFrameInputQueue.pop();
    mFrameOutputQueue.push(frame);
}

int V4l2MediaSource::videoInit(const std::string &dev, int &fd)
{
    int ret;
    char devName[100];
    struct v4l2_capability cap = { 0 };

    fd = v4l2_open(dev.c_str(), O_RDWR | O_NONBLOCK);
    if(fd < 0) {
        PLOGE << "v4l2_open " << dev << " failed, errno = " << errno;
        return -1;
    }

    ret = v4l2_querycap(fd, &cap);
    if(ret < 0) {
        PLOGE << "v4l2_querycap " << dev << " failed, errno = " << errno;
        return ret;
    }

    if(!(cap.capabilities & mDriverType)) {
        PLOGE << mDriverType << "  not supported.";
        return -1;
    }

    ret = v4l2_s_input(fd, 0);
    if (ret < 0) {
        PLOGE << "v4l2_s_input " << dev << " failed, errno = " << errno;
        return ret;
    }

    enum v4l2_buf_type bufType = mDriverType == V4L2_CAP_VIDEO_CAPTURE ?
        V4L2_BUF_TYPE_VIDEO_CAPTURE : V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = v4l2_enum_fmt(fd, mFmt, bufType);
    if(ret < 0) {
        PLOGE << " v4l2_enum_fmt " << mFmt << " failed, errno = " << errno;
        return -1;
    }

    int mPlanesNum = 0;
    ret = v4l2_s_fmt(fd, mFmt, bufType, &mWidth, &mHeight, &mPlanesNum);
    if(ret < 0) {
        PLOGE << " v4l2_s_fmt " <<  mFmt << " failed, errno = " << errno;
        return -1;
    }

    PLOGI << " bufType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : " << (bufType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    struct v4l2_streamparm parm = { 0 };
    parm.type = bufType;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = mFps;
    ret = v4l2_s_parm(fd, &parm);
    if (ret < 0) {
        PLOGE << " v4l2_s_parm " <<  mFmt << " failed, errno = " << errno;
        return ret;
    }

    mV4l2Buf = v4l2_reqbufs(fd, bufType, 4);
    if(!mV4l2Buf) {
        PLOGE << " v4l2_reqbufs " <<  mFmt << " failed, errno = " << errno;
        return -1;
    }

    mV4l2Buf->num_planes = mPlanesNum;
    ret = v4l2_querybuf(fd, mV4l2Buf);
    if(ret < 0) {
        PLOGE << " v4l2_querybuf " <<  mFmt << " failed, errno = " << errno;
        return -1;
    }

    ret = v4l2_mmap(fd, mV4l2Buf);
    if(ret < 0) {
        PLOGE << " v4l2_mmap " <<  mFmt << " failed, errno = " << errno;
        return -1;
    }

    ret = v4l2_qbuf_all(fd, mV4l2Buf);
    if(ret < 0) {
        PLOGE << " v4l2_qbuf_all " <<  mFmt << " failed, errno = " << errno;
        return -1;
    }

    PLOGI << " v4l2_querybuf " <<  mFmt << " success";
    ret = v4l2_streamon(fd, bufType);
    if(ret < 0) {
        PLOGE << " v4l2_streamon " << mFmt << " failed, errno = " << errno;
        return -1;
    }

    PLOGI << " v4l2_streamon " <<  mFmt << " success";
    return 0;
}

int V4l2MediaSource::videoExit()
{
    int ret;

    enum v4l2_buf_type bufType = mDriverType == V4L2_CAP_VIDEO_CAPTURE ?
        V4L2_BUF_TYPE_VIDEO_CAPTURE : V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ret = v4l2_streamoff(mFd, bufType);
    if (ret < 0) {
        PLOGE << "streamoff failed, errno = " << errno;
        return ret;
    }

    ret = v4l2_munmap(mFd, mV4l2Buf);
    if (ret < 0) {
        PLOGE << "munmap failed, errno = " << errno;
        return ret;
    }

    ret = v4l2_relbufs(mFd, mV4l2Buf);
    if(ret < 0) {
        PLOGE << "relbufs failed, errno = " << errno;
        return ret;
    }

    free(mV4l2Buf);
    free(mV4l2Buf->buf);
    ret = v4l2_close(mFd);
    mFd = -1;

    return ret;
}

/* 将h264 数据解析为单个NALU单元， Webrtc 只支持传送NALU 单元 */
void V4l2MediaSource::parseH264(Frame* frame, uint8_t *h264Data, int length)
{
    int i = 0;
    int before = 0;
    int num = 0;
    int offset = 0;

    if (startCode4(h264Data)) {
        offset = 4;
    } else if (startCode3(h264Data)) {
        offset = 3;
    }

    while (i + offset <= length) {
        if (h264Data[i] != 0) {
            i++;
            continue;
        }

        if (offset == 3) {
            if (h264Data[i + 1] == 0 && h264Data[i + 2] == 1) {
                if (i - before - offset > 0 && i - before - offset <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
                    memcpy(frame->mBufferArr[num].data(), h264Data + before + offset, i - before - offset);
                    frame->mSizeArr.push_back(i - before - offset);
                    num++;
                } else {
                    PLOGE << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num;
                }
                before = i;
            }
        } else if (offset == 4) {
            if (h264Data[i + 1] == 0 && h264Data[i + 2] == 0 && h264Data[i + 3] == 1) {
                if (i - before - offset > 0 && i - before - offset <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
                    memcpy(frame->mBufferArr[num].data(), h264Data + before + offset, i - before - offset);
                    frame->mSizeArr.push_back(i - before - offset);
                    num++;
                } else if (num != 0){
                    PLOGE << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num;
                }
                before = i;
            }
        }
        i++;
    }

    if (i - before > 0 && i - before - offset <= H264_FRAME_MAX_NALU_SIZE && num < H264_FRAME_MAX_NALU_NUM) {
        memcpy(frame->mBufferArr[num].data(), h264Data + before + offset, i - before);
        frame->mSizeArr.push_back(i - before);
        num++;
    }  else if (num != 0){
        PLOGE << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num;
    }
}

void V4l2MediaSource::setFd(int fd)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mFd = fd;
}
