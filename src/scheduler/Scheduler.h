#ifndef _EVENT_SCHEDULER_H_
#define _EVENT_SCHEDULER_H_
#include <vector>
#include <queue>

#include "Poller.h"
#include "Timer.h"
#include <mutex>

class EventScheduler
{
public:
    typedef void (*Callback)(void*);

    enum PollerType {
        POLLER_SELECT = 0,
        POLLER_POLL,
        POLLER_EPOLL,
        POLLER_TYPE_BUT
    };

    static EventScheduler* createNew(PollerType type);

    EventScheduler(PollerType type, int fd);
    virtual ~EventScheduler();
    Poller *getPoller() {
        return mPoller;
    }

    TimerId addTimedEventRunAfater(TimerEvent* event, TimeInterval delay);
    TimerId addTimedEventRunAt(TimerEvent* event, Timestamp when);
    TimerId addTimedEventRunEvery(TimerEvent* event, TimeInterval interval);
    bool removeTimedEvent(TimerId timerId);
    bool addIOEvent(IOEvent* event);
    bool updateIOEvent(IOEvent* event);
    bool removeIOEvent(IOEvent* event);

    void loop();
    void wakeup();

    void runInLocalThread(Callback callBack, void* arg);
    void handleOtherEvent();

private:
    static void handleReadCallback(void*);
    void handleRead();

private:
    int mWakeupFd;
    bool mQuit;
    Poller* mPoller;
    TimerManager* mTimerManager;
    IOEvent* mWakeIOEvent;
    std::queue<std::pair<Callback, void*>> mCallBackQueue;
    std::mutex mMutex;
};

#endif //_EVENT_SCHEDULER_H_