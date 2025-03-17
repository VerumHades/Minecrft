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