#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "V4l2MediaSource.h"
#include <iostream>

V4l2MediaSource::V4l2MediaSource(std::shared_ptr<ThreadPool> pool, const std::string& dev) :
    MediaSource(pool),
    mPool(pool),
    mDev(dev),
    mWidth(FRAME_WIDTH),
    mHeight(FRAME_HEIGHT)
{
    bool ret;

    setFps(FPS);

    ret = videoInit();
    assert(ret == true);

    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mPool->addTask(mTask);
    }
}

V4l2MediaSource::~V4l2MediaSource()
{
    videoExit();
}

void V4l2MediaSource::start()
{

}

static inline int startCode3(uint8_t* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static inline int startCode4(uint8_t* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        return 1;
    } else {
        return 0;
    }
}

void V4l2MediaSource::readFrame()
{
    using Clock = std::chrono::high_resolution_clock;
    using Milliseconds = std::chrono::milliseconds;
    auto start = Clock::now();
    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<Milliseconds>(end - start);

    std::lock_guard<std::mutex> lock(mMutex);
    if (mFrameInputQueue.empty()) {
        std::cout << "V4l2MediaSource::readFrame mAVFrameInputQueue.empty()" << std::endl;
        return;
    }

    Frame* frame = mFrameInputQueue.front();
    for (auto i = 0; i < 1; ++i) {
        int ret = v4l2_poll(mFd);
        if (ret < 0) {
            return;
        }

        mV4l2BufUnit = v4l2_dqbuf(mFd, mV4l2Buf);
        if (!mV4l2BufUnit) {
            return;
        }

        parseH264(frame, (uint8_t*)mV4l2BufUnit->start, mV4l2BufUnit->length);
        //不知道为什么，在t113 或者 x2600上这里需要占着cpu等一会，去掉后会出现 errno = 51 或者 53
        // LOG_INFO("");
        ret = v4l2_qbuf(mFd, mV4l2BufUnit);
        if (ret < 0) {
            return;
        }
    }
    mFrameInputQueue.pop();
    mFrameOutputQueue.push(frame);

    end = Clock::now();
    duration = std::chrono::duration_cast<Milliseconds>(end - start);
    // std::cout << "执行时间: " << duration.count() << " 毫秒" << std::endl;
}

bool V4l2MediaSource::videoInit()
{
    int ret;
    char devName[100];
    struct v4l2_capability cap;

    mFd = v4l2_open(mDev.c_str(), O_RDWR);
    if(mFd < 0) {
        return false;
    }

    ret = v4l2_querycap(mFd, &cap);
    if(ret < 0) {
        return false;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        return false;
    }

    ret = v4l2_enum_fmt(mFd, V4L2_PIX_FMT_H264, V4L2_BUF_TYPE_VIDEO_CAPTURE);
    if(ret < 0) {
        return false;
    }

    ret = v4l2_s_fmt(mFd, &mWidth, &mHeight, V4L2_PIX_FMT_H264, V4L2_BUF_TYPE_VIDEO_CAPTURE);
    if(ret < 0) {
        return false;
    }

    struct v4l2_streamparm parm = { 0 };
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = FPS;  // 设置为10fps
    ret = v4l2_s_parm(mFd, &parm);
    if (ret < 0) {
        return false;
    }

    mV4l2Buf = v4l2_reqbufs(mFd, V4L2_BUF_TYPE_VIDEO_CAPTURE, 4);
    if(!mV4l2Buf) {
        return false;
    }

    ret = v4l2_querybuf(mFd, mV4l2Buf);
    if(ret < 0) {
        return false;
    }

    ret = v4l2_mmap(mFd, mV4l2Buf);
    if(ret < 0) {
        return false;
    }

    ret = v4l2_qbuf_all(mFd, mV4l2Buf);
    if(ret < 0) {
        return false;
    }

    ret = v4l2_streamon(mFd);
    if(ret < 0) {
        return false;
    }

    ret = v4l2_poll(mFd);
    if(ret < 0) {
        return false;
    }

    return true;
}

bool V4l2MediaSource::videoExit()
{
    int ret;

    ret = v4l2_streamoff(mFd);
    if(ret < 0)
        return false;

    ret = v4l2_munmap(mFd, mV4l2Buf);
    if(ret < 0)
        return false;

    ret = v4l2_relbufs(mV4l2Buf);
    if(ret < 0)
        return false;

    v4l2_close(mFd);

    return true;
}

void V4l2MediaSource::parseH264(Frame* frame, uint8_t *h264Data, int length)
{
    int i = 0;
    int before = 0;
    int num = 0;
    int offset = 0;


    // std::cout << " length = " << length << std::endl;
    if (startCode4(h264Data)) {
        offset = 4;
        // std::cout << "start code 4" << std::endl; 
    } else if (startCode3(h264Data)) {
        offset = 3;
        // std::cout << "start code 3" << std::endl; 
    } else {
        // std::cout << "start code error." << std::endl; 
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
                    std::cout << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num << std::endl;
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
                    std::cout << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num << std::endl;
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
        std::cout << "h264Data error, length = " << length << " i - before - offset = "  << i - before - offset << " num = " << num << std::endl;
    }
}
