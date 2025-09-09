#include "ThreadPool.h"

ThreadPool::ThreadPool(int num) : mThreads(num), mQuit(false)
{
    createThreads();
}

ThreadPool::~ThreadPool()
{
    cancelThreads();
}

void ThreadPool::addTask(Task& task)
{
    std::lock_guard<std::mutex>lock(mMutex);
    mTaskQueue.push(task);
    mCondition.notify_all();
}

void ThreadPool::handleTask()
{
    while(mQuit != true) {
        Task task;
        {
            std::unique_lock<std::mutex>lock(mMutex);
            while (mTaskQueue.empty() && mQuit == false) {
                mCondition.wait(lock);
            }

            if (mQuit == true) {
                break;
            }

            if (mTaskQueue.empty()) {
                continue;
            }

            task = mTaskQueue.front();
            mTaskQueue.pop();
        }
        task.handle();
    }
}

void ThreadPool::createThreads()
{
    for(std::vector<MThread>::iterator it = mThreads.begin(); it != mThreads.end(); ++it) {
        (*it).start(this);
    }
}

void ThreadPool::cancelThreads()
{
    while (true) {
        std::lock_guard<std::mutex>lock(mMutex);
        if (mTaskQueue.empty()) {
            mQuit = true;
            mCondition.notify_all();
            break;
        }
    }

    for(std::vector<MThread>::iterator it = mThreads.begin(); it != mThreads.end(); ++it) {
        (*it).join();
    }

    mThreads.clear();
}

void ThreadPool::MThread::run(void* arg)
{
    ThreadPool* threadPool = (ThreadPool*)arg;
    threadPool->handleTask();
}