#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>

#include "Scheduler.h"
#include "SelectPoller.h"

static int createEventFd()
{
    int evtFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtFd < 0) {
        return -1;
    }

    return evtFd;
}

EventScheduler* EventScheduler::createNew(PollerType type)
{
    if (type >= POLLER_TYPE_BUT) {
        return NULL;
    }

    int evtFd = createEventFd();
    if (evtFd < 0) {
        return NULL;
    }

    return new EventScheduler(type, evtFd);
}

EventScheduler::EventScheduler(PollerType type, int fd) : mQuit(false), mWakeupFd(fd)
{
    switch (type) {
        case POLLER_SELECT:
            mPoller = SelectPoller::createNew();
            break;
    }

    mTimerManager = TimerManager::createNew(mPoller);
    mWakeIOEvent = IOEvent::createNew(mWakeupFd, this);
    mWakeIOEvent->setReadCallback(handleReadCallback);
    mWakeIOEvent->enableReadHandling();
    mPoller->addIOEvent(mWakeIOEvent);
}

EventScheduler::~EventScheduler()
{
    mPoller->removeIOEvent(mWakeIOEvent);
    ::close(mWakeupFd);

    delete mWakeIOEvent;
    delete mTimerManager;
    delete mPoller;
}

TimerId EventScheduler::addTimedEventRunAfater(TimerEvent* event, TimeInterval delay)
{
    Timestamp when = Timer::getCurTime();
    when += delay;

    return mTimerManager->addTimer(event, when, 0);
}

TimerId EventScheduler::addTimedEventRunAt(TimerEvent* event, Timestamp when)
{
    return mTimerManager->addTimer(event, when, 0);
}

TimerId EventScheduler::addTimedEventRunEvery(TimerEvent* event, TimeInterval interval)
{
    Timestamp when = Timer::getCurTime();
    when += interval;

    return mTimerManager->addTimer(event, when, interval);
}

bool EventScheduler::removeTimedEvent(TimerId timerId)
{
    return mTimerManager->removeTimer(timerId);
}

bool EventScheduler::addIOEvent(IOEvent* event)
{
    return mPoller->addIOEvent(event);
}

bool EventScheduler::updateIOEvent(IOEvent* event)
{
    return mPoller->updateIOEvent(event);
}

bool EventScheduler::removeIOEvent(IOEvent* event)
{
    return mPoller->removeIOEvent(event);
}

void EventScheduler::loop()
{
    while(mQuit != true) {
        mPoller->handleEvent();
        this->handleOtherEvent();
    }
}

void EventScheduler::wakeup()
{
    uint64_t one = 1;
    int ret;
    ret = ::write(mWakeupFd, &one, sizeof(one));
}

void EventScheduler::handleReadCallback(void* arg)
{
    if(!arg) {
        return;
    }

    EventScheduler* scheduler = (EventScheduler*)arg;
    scheduler->handleRead();
}

void EventScheduler::handleRead()
{
    uint64_t one;
    while(::read(mWakeupFd, &one, sizeof(one)) > 0);
}

void EventScheduler::runInLocalThread(Callback callBack, void* arg)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mCallBackQueue.push(std::make_pair(callBack, arg));
}

void EventScheduler::handleOtherEvent()
{
    std::lock_guard<std::mutex> lock(mMutex);
    while (!mCallBackQueue.empty()) {
        std::pair<Callback, void*> event = mCallBackQueue.front();
        event.first(event.second);
    }
}