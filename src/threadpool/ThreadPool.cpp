#include "ThreadPool.h"

ThreadPool::ThreadPool(int num) : mNum(num)
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
    mCondition.notify_one();
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
    mQuit = false;
    for (int i = 0; i < mNum; i++) {
        mThreads.emplace_back(std::thread(&ThreadPool::handleTask, this));
    }
}

void ThreadPool::cancelThreads()
{
    while (true) {
        std::lock_guard<std::mutex>lock(mMutex);
        if (mTaskQueue.empty()) { // 先等待线程执行完所有的任务，任务队列为空
            mQuit = true;
            mCondition.notify_all();
            break;
        }
    }

    for (std::thread& t : mThreads) {
        t.join();
    }
    mThreads.clear();
}