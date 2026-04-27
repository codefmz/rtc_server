#ifndef _MEDIA_SOURCE_H_
#define _MEDIA_SOURCE_H_
#include <queue>
#include <stdint.h>
#include <memory>
#include <mutex>
#include <vector>
#include "Base.h"
#include "ThreadPool.h"

class Frame
{
public:
    Frame() {
        mBufferArr.resize(H264_FRAME_MAX_NALU_NUM);
        for (int i = 0; i < H264_FRAME_MAX_NALU_NUM; ++i) {
            mBufferArr[i].resize(H264_FRAME_MAX_NALU_SIZE);
        }
    }

    ~Frame() {
    }

    std::vector<std::vector<uint8_t>> mBufferArr;
    std::vector<int> mSizeArr;
};

class MediaSource
{
public:
    virtual ~MediaSource();

    Frame* getFrame();
    void putFrame(Frame* frame);
    int getFps() const {
        return mFps;
    }

protected:
    MediaSource(std::shared_ptr<ThreadPool> pool);
    virtual void readFrame() = 0;
    void setFps(int fps) {
        mFps = fps;
    }

private:
    static void taskCallback(void*);

protected:
    std::shared_ptr<ThreadPool> mPool;
    // TODO : 可以使用无锁环形队列优化
    Frame mFrames[DEFAULT_FRAME_NUM];
    std::queue<Frame*> mFrameInputQueue;
    std::queue<Frame*> mFrameOutputQueue;
    std::mutex mMutex;
    Task mTask;
    int mFps;
};

#endif //_MEDIA_SOURCE_H_