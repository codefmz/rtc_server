#include "Event.h"

TimerEvent::TimerEvent(void* arg) : mArg(arg) { }

void TimerEvent::handleEvent()
{
    if (mTimeoutCallback) {
        mTimeoutCallback(mArg);
    }
}

IOEvent::IOEvent(int fd, void* arg) : mFd(fd), mArg(arg),
    mEvent(EVENT_NONE), mREvent(EVENT_NONE), mReadCallback(nullptr),
    mWriteCallback(nullptr), mErrorCallback(nullptr)
{

}

void IOEvent::handleEvent()
{
    if (mReadCallback && (mREvent & EVENT_READ)) {
        mReadCallback(mArg);
    }

    if (mWriteCallback && (mREvent & EVENT_WRITE)) {
        mWriteCallback(mArg);
    }

    if (mErrorCallback && (mREvent & EVENT_ERROR)) {
        mErrorCallback(mArg);
    }
};