#include <game/threadpool.hpp>

SleepyThread::SleepyThread(){
    thread = std::thread(&SleepyThread::run, this);
}
SleepyThread::~SleepyThread(){
    stop();
}
void SleepyThread::run(){
    while(true){
        std::unique_lock<std::mutex> lock(mutex);

        var.wait(lock, [this] { return isAwake || stopThread; });

        if(stopThread) break;

        isAwake = false;
        
        if (func) {
            func();
            func = nullptr;
        }

        onAvailable();
    }
}

void SleepyThread::awake(const std::function<void(void)>& func){
    {
        std::lock_guard<std::mutex> lock(mutex);
        this->func = func;
        isAwake = true;
    }
    var.notify_one();
}

void SleepyThread::stop(){
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopThread = true; // Mark as awake
    }
    var.notify_one();
    thread.join();
}