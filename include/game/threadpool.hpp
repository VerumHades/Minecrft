#pragma once
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <iostream>

class SleepyThread{
    private:
        std::thread thread;
        std::mutex mutex;
        std::condition_variable var;

        std::function<void(void)> func;
        bool isAwake = false;
        bool stopThread = false;

        void run();
        void stop();
    public:
        SleepyThread();
        ~SleepyThread();

        std::function<void(void)> onAvailable;
        void awake(const std::function<void(void)>& func);
};

class ThreadPool{
    private:
        std::mutex updateMutex;
        std::queue<size_t> nextThread;
        std::vector<std::unique_ptr<SleepyThread>> threads = {};
        size_t maxThreads = 0;

        std::queue<std::function<void(void)>> pendingJobs;

        bool deployInternal(const std::function<void(void)>& func);

    public:
        ThreadPool(size_t maxThreads);
        /*
            Deploys the job immidiately or adds it to processing queue
        */
        bool deploy(const std::function<void(void)>& func);

        /*
            Deploys pending jobs if possible
        */
        void deployPendingJobs();

        bool finished();
};