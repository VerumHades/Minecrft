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

        if (func) {
            func();
            func = nullptr;
        }

        isAwake = false;
        onAvailable();
    }
}

void SleepyThread::awake(std::function<void(void)> func){
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

ThreadPool::ThreadPool(size_t mthreads){
    maxThreads = mthreads;

    for(int i = 0;i < mthreads;i++){
        auto t = std::make_unique<SleepyThread>();
        t->onAvailable = [this,i]{
            std::lock_guard<std::mutex> lock(this->updateMutex);
            this->nextThread.push(i);
            //std::cout << "Thread made available: " << i << std::endl;
        };
        this->nextThread.push(i);
        threads.push_back(std::move(t));
    }
}

bool ThreadPool::deploy(std::function<void(void)> func){
    if(nextThread.empty()) return false;

    //std::cout << "Deploying thread!" << std::endl;

    auto& next = threads[nextThread.front()];
    nextThread.pop();

    next->awake(func);

    return true;
}