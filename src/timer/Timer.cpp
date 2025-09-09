#include <sys/timerfd.h>
#include "Timer.h"

static int timerFdCreate(int clockid, int flags)
{
    return timerfd_create(clockid, flags);
}

static bool timerFdSetTime(int fd, Timestamp when, TimeInterval period)
{
    struct itimerspec newVal;

    newVal.it_value.tv_sec = when / 1000; //ms->s
    newVal.it_value.tv_nsec = when % 1000 * 1000 * 1000; //ms->ns
    newVal.it_interval.tv_sec = period / 1000;
    newVal.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

    if (timerfd_settime(fd, TFD_TIMER_ABSTIME, &newVal, NULL) < 0) {
        return false;
    }

    return true;
}

Timer::Timer(TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval) : mTimerEvent(event),
    mTimestamp(timestamp), mTimeInterval(timeInterval)
{
    if (timeInterval > 0) {
        mRepeat = true;
    } else {
        mRepeat = false;
    }
}

Timer::~Timer()
{
}

Timestamp Timer::getCurTime()
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec*1000 + now.tv_nsec/1000000);
}

void Timer::handleEvent()
{
    if (!mTimerEvent) {
        return;
    }

    mTimerEvent->handleEvent();
}

TimerManager* TimerManager::createNew(Poller* poller)
{
    if (!poller) {
        return NULL;
    }

    int timerFd = timerFdCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd < 0) {
        return NULL;
    }

    return new TimerManager(timerFd, poller);
}

TimerManager::TimerManager(int timerFd, Poller* poller) : mTimerFd(timerFd), mPoller(poller),
    mLastTimerId(0)
{
    mTimerIOEvent = IOEvent::createNew(mTimerFd, this);
    mTimerIOEvent->setReadCallback(handleRead);
    mTimerIOEvent->enableReadHandling();
    modifyTimeout();
    mPoller->addIOEvent(mTimerIOEvent);
}

TimerManager::~TimerManager()
{
    mPoller->removeIOEvent(mTimerIOEvent);
    delete mTimerIOEvent;
}

TimerId TimerManager::addTimer(TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval)
{
    Timer timer(event, timestamp, timeInterval);

    ++mLastTimerId;
    mTimers.insert(std::make_pair(mLastTimerId, timer));
    mEvents.insert(std::make_pair(TimerIndex(timestamp, mLastTimerId), timer));

    modifyTimeout();

    return mLastTimerId;
}

bool TimerManager::removeTimer(TimerId timerId)
{
    std::map<TimerId, Timer>::iterator it = mTimers.find(timerId);
    if (it != mTimers.end()) {
        Timestamp timestamp = it->second.mTimestamp;
        TimerId timerId = it->first;
        mEvents.erase(TimerIndex(timestamp, timerId));
        mTimers.erase(timerId);
    }

    modifyTimeout();

    return true;
}

void TimerManager::modifyTimeout()
{
    std::multimap<TimerIndex, Timer>::iterator it = mEvents.begin();
    if (it != mEvents.end()) {
        Timer timer = it->second;
        timerFdSetTime(mTimerFd, timer.mTimestamp, timer.mTimeInterval);
    } else {
        timerFdSetTime(mTimerFd, 0, 0);
    }
}

void TimerManager::handleRead(void* arg)
{
    if (!arg) {
        return;
    }

    TimerManager* timerManager = (TimerManager*)arg;
    timerManager->handleTimerEvent();
}

void TimerManager::handleTimerEvent()
{
    if(!mTimers.empty()) {
        int64_t timePoint = Timer::getCurTime();
        while(!mTimers.empty() && mEvents.begin()->first.first <= timePoint) {
            TimerId timerId = mEvents.begin()->first.second;
            Timer timer = mEvents.begin()->second;

            timer.handleEvent();
            mEvents.erase(mEvents.begin());
            if(timer.mRepeat == true) {
                timer.mTimestamp = timer.mTimestamp + timer.mTimeInterval;
                mEvents.insert(std::make_pair(TimerIndex(timer.mTimestamp, timerId), timer));
            } else {
                mTimers.erase(timerId);
            }
        }
    }

    modifyTimeout();
}
