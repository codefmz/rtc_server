#include "Timer.h"

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
    if (mTimerEvent) {
        delete mTimerEvent;
        mTimerEvent = nullptr;
    }
}

void Timer::handleEvent()
{
    if (!mTimerEvent) {
        return;
    }

    mTimerEvent->handleEvent();
}

