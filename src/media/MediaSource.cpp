#include "MediaSource.h"

MediaSource::MediaSource(std::shared_ptr<ThreadPool> pool) : mPool(pool)
{
    mTask.setTaskCallback(taskCallback, this);
}

MediaSource::~MediaSource()
{
}

Frame* MediaSource::getFrame()
{
    std::lock_guard<std::mutex> lock(mMutex);
    if(mFrameOutputQueue.empty()) {
        return nullptr;
    }

    Frame* frame = mFrameOutputQueue.front();
    mFrameOutputQueue.pop();

    return frame;
}

void MediaSource::putFrame(Frame* frame)
{
    std::lock_guard<std::mutex> lock(mMutex);
    frame->mSizeArr.clear();
    mFrameInputQueue.push(frame);
    mPool->addTask(mTask);
}

void MediaSource::taskCallback(void* arg)
{
    MediaSource* source = (MediaSource*)arg;
    source->readFrame();
}