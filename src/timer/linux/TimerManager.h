#ifndef __TIMER_MANAGER_H__
#define __TIMER_MANAGER_H__

#include "Poller.h"
#include "Event.h"
#include "Timer.h"
#include <unordered_set>
#include <queue>
#include <memory>

typedef uint32_t TimerId;

class TimerManager
{
public:
    TimerManager(std::shared_ptr<Poller> poller);
    ~TimerManager();

    TimerId addTimer(TimerEvent event, Timestamp timestamp, TimeInterval timeInterval);
    bool removeTimer(TimerId timerId);

private:
    void modifyTimeout();
    static void handleRead(void*);
    void handleTimerEvent();
    bool timerFdSetTime(int fd, Timestamp when, TimeInterval period);

private:
    std::shared_ptr<Poller> mPoller;
    int mTimerFd;
    uint32_t mLastTimerId;
    std::priority_queue<Timer*, std::vector<Timer*>, Timer::TimerCompare> mTimerQueue;
    std::unordered_set<TimerId> mCancelTimers;
    IOEvent* mTimerIOEvent;
};

#endif //__TIMER_MANAGER_H__