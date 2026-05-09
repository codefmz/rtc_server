#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdint.h>
#include "Event.h"

typedef int64_t Timestamp; //ms
typedef uint32_t TimeInterval; //ms

class Timer
{
public:
    ~Timer();
    Timer(TimerEvent event, Timestamp timestamp, TimeInterval timeInterval);
    void handleEvent();
    bool repeat() const { return mRepeat; }

    Timestamp getTimestamp() const { 
        return mTimestamp; 
    }

    TimeInterval getTimeInterval() const { 
        return mTimeInterval; 
    }

    void setTimerStamp(Timestamp timestamp) { 
        mTimestamp = timestamp; 
    }

    uint32_t getTimerId() const {
        return mTimerId;
    }

    void setTimerId(uint32_t timerId) {
        mTimerId = timerId;
    }

    struct TimerCompare {
        bool operator()(const Timer* a, const Timer* b) const {
            return a->getTimestamp() > b->getTimestamp();
        }
    };

    void updateTimerStamp() {
        mTimestamp += mTimeInterval;
    }

private:
    TimerEvent mTimerEvent;
    Timestamp mTimestamp;
    TimeInterval mTimeInterval;
    uint32_t mTimerId;
    bool mRepeat;
};

#endif //_TIMER_H_