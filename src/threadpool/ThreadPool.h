#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include <queue>
#include <vector>

#include <atomic>
#include "Thread.h"
#include <mutex>
#include <condition_variable>
#include <thread>


class Task
{
public:
    typedef void (*TaskCallback)(void*);
    Task() { };

    void setTaskCallback(TaskCallback cb, void* arg) {
        mTaskCallback = cb;
        mArg = arg;
    }

    void handle() {
        if(mTaskCallback) {
            mTaskCallback(mArg);
        }
    }

    bool operator=(const Task& task) {
        this->mTaskCallback = task.mTaskCallback;
        this->mArg = task.mArg;
        return true;
    }

private:
    void (*mTaskCallback)(void*);
    void* mArg;
};

class ThreadPool
{
public:
    ThreadPool(int num);
    ~ThreadPool();

    void addTask(Task& task);

    void createThreads();
    void cancelThreads();

    class MThread : public Thread
    {
    protected:
        virtual void run(void *arg);
    };
private:
    void handleTask();

private:
    int mNum;
    std::queue<Task> mTaskQueue;
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::vector<MThread> mThreads;
    std::atomic<bool> mQuit;
};

#endif //_THREADPOOL_H_