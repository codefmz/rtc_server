#ifndef _EVENT_H_
#define _EVENT_H_

typedef void (*EventCallback)(void*);

class TimerEvent
{
public:
    TimerEvent(void* arg = nullptr);
    ~TimerEvent() { }

    void setArg(void* arg) {
        mArg = arg;
    }

    void setTimeoutCallback(EventCallback cb) {
        mTimeoutCallback = cb;
    }

    void handleEvent();

private:
    void* mArg;
    EventCallback mTimeoutCallback;
};

class IOEvent
{
public:
    enum IOEventType {
        EVENT_NONE = 0,
        EVENT_READ = 1,
        EVENT_WRITE = 2,
        EVENT_ERROR = 4,
    };

    IOEvent(int fd, void* arg);
    ~IOEvent() { }

    int getFd() const {
        return mFd;
    }

    int getEvent() const {
        return mEvent;
    }

    void setREvent(int event) {
        mREvent = event;
    }

    void setArg(void* arg) {
        mArg = arg;
    }

    void setReadCallback(EventCallback cb) {
        mReadCallback = cb;
    };

    void setWriteCallback(EventCallback cb) {
         mWriteCallback = cb;
    };

    void setErrorCallback(EventCallback cb) {
        mErrorCallback = cb;
    };

    void enableReadHandling() {
        mEvent |= EVENT_READ;
    }

    void enableWriteHandling() {
        mEvent |= EVENT_WRITE;
    }

    void enableErrorHandling() {
        mEvent |= EVENT_ERROR;
    }

    void disableReadeHandling() {
        mEvent &= ~EVENT_READ;
    }

    void disableWriteHandling() {
        mEvent &= ~EVENT_WRITE;
    }
    void disableErrorHandling() {
        mEvent &= ~EVENT_ERROR;
    }

    bool isNoneHandling() const {
        return mEvent == EVENT_NONE;
    }

    bool isReadHandling() const {
        return (mEvent & EVENT_READ) != 0;
    }

    bool isWriteHandling() const {
        return (mEvent & EVENT_WRITE) != 0;
    }

    bool isErrorHandling() const {
        return (mEvent & EVENT_ERROR) != 0;
    };

    void handleEvent();

private:
    int mFd;
    void* mArg;
    int mEvent;
    int mREvent;
    EventCallback mReadCallback;
    EventCallback mWriteCallback;
    EventCallback mErrorCallback;
};

#endif //_EVENT_H_