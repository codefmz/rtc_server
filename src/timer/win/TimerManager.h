#ifndef __WIN_TIMER_MANAGER_H__
#define __WIN_TIMER_MANAGER_H__

#include "Timer.h"
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

typedef uint32_t TimerId;

class TimerManager
{
public:
    TimerManager();
    ~TimerManager();

    TimerId addTimer(TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval);
    bool removeTimer(TimerId timerId);

private:
    void timerLoop();
    void clearTimers();
    void removeCanceledTimers();
    static Timestamp getNowMs();

private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    bool mQuit;
    uint32_t mLastTimerId;
    std::thread mThread;
    std::priority_queue<Timer*, std::vector<Timer*>, Timer::TimerCompare> mTimerQueue;
    std::unordered_set<TimerId> mCancelTimers;
};

#endif //__WIN_TIMER_MANAGER_H__
