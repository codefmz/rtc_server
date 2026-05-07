#include "TimerManager.h"
#include "rtc_utils.h"
#include <sys/timerfd.h>

TimerManager::TimerManager(Poller* poller) : mPoller(poller), mLastTimerId(0)
{
    int timerFd = timerFdCreate(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd < 0) {
        return;
    }

    mTimerFd = timerFd;
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
    Timer *timer = new Timer(event, timestamp, timeInterval);

    ++mLastTimerId;
    timer->setTimerId(mLastTimerId);
    mTimerQueue.push(timer);

    modifyTimeout();

    return mLastTimerId;
}

bool TimerManager::removeTimer(TimerId timerId)
{
    mCancelTimers.insert(timerId);
    modifyTimeout();

    return true;
}

bool TimerManager::timerFdSetTime(int fd, Timestamp when, TimeInterval period)
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

void TimerManager::modifyTimeout()
{
    while (!mTimerQueue.empty()) {
        auto timer = mTimerQueue.top();
        if (mCancelTimers.count(timer->getTimerId()) > 0) {
            mTimerQueue.pop();
            mCancelTimers.erase(timer->getTimerId());
            delete timer;
            continue;
        }

        timerFdSetTime(mTimerFd, timer->getTimestamp(), timer->getTimeInterval());
    }

    if (mTimerQueue.empty()) {
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
    uint64_t timePoint = Utils::getNowMs();
    while (!mTimerQueue.empty()) {
        Timer* timer = mTimerQueue.top();
        /* timer is canceled */
        if (mCancelTimers.count(timer->getTimerId()) > 0) {
            mTimerQueue.pop();
            mCancelTimers.erase(timer->getTimerId());
            delete timer;
            continue;
        }

        if (timer->getTimestamp() > timePoint) {
            break;
        }

        timer->handleEvent();
        mTimerQueue.pop();
        if(timer->repeat()) {
            timer->updateTimerStamp();
            mTimerQueue.push(timer);
        }
    }

    modifyTimeout();
}
