#include "TimerManager.h"
#include <chrono>

TimerManager::TimerManager() : mQuit(false), mLastTimerId(0)
{
    mThread = std::thread(&TimerManager::timerLoop, this);
}

TimerManager::~TimerManager()
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mQuit = true;
    }

    mCondition.notify_all();

    if (mThread.joinable()) {
        mThread.join();
    }

    clearTimers();
}

TimerId TimerManager::addTimer(TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval)
{
    Timer* timer = new Timer(event, timestamp, timeInterval);
    TimerId timerId;

    {
        std::lock_guard<std::mutex> lock(mMutex);
        ++mLastTimerId;
        timerId = mLastTimerId;
        timer->setTimerId(timerId);
        mTimerQueue.push(timer);
    }

    mCondition.notify_one();

    return timerId;
}

bool TimerManager::removeTimer(TimerId timerId)
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mCancelTimers.insert(timerId);
    }

    mCondition.notify_one();

    return true;
}

void TimerManager::timerLoop()
{
    std::unique_lock<std::mutex> lock(mMutex);

    while (!mQuit) {
        removeCanceledTimers();

        if (mTimerQueue.empty()) {
            mCondition.wait(lock, [this] {
                return mQuit || !mTimerQueue.empty();
            });
            continue;
        }

        Timer* timer = mTimerQueue.top();
        Timestamp now = getNowMs();
        Timestamp timestamp = timer->getTimestamp();
        if (timestamp > now) {
            mCondition.wait_for(lock, std::chrono::milliseconds(timestamp - now));
            continue;
        }

        mTimerQueue.pop();
        TimerId timerId = timer->getTimerId();
        if (mCancelTimers.count(timerId) > 0) {
            mCancelTimers.erase(timerId);
            delete timer;
            continue;
        }

        lock.unlock();
        timer->handleEvent();
        lock.lock();

        if (mQuit || mCancelTimers.count(timerId) > 0) {
            mCancelTimers.erase(timerId);
            delete timer;
        } else if (timer->repeat()) {
            timer->updateTimerStamp();
            mTimerQueue.push(timer);
        } else {
            delete timer;
        }
    }
}

void TimerManager::clearTimers()
{
    std::lock_guard<std::mutex> lock(mMutex);

    while (!mTimerQueue.empty()) {
        Timer* timer = mTimerQueue.top();
        mTimerQueue.pop();
        delete timer;
    }

    mCancelTimers.clear();
}

void TimerManager::removeCanceledTimers()
{
    while (!mTimerQueue.empty()) {
        Timer* timer = mTimerQueue.top();
        if (mCancelTimers.count(timer->getTimerId()) == 0) {
            break;
        }

        mTimerQueue.pop();
        mCancelTimers.erase(timer->getTimerId());
        delete timer;
    }
}

Timestamp TimerManager::getNowMs()
{
    using namespace std::chrono;

    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch()
    ).count();
}
