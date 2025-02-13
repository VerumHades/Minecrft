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

ThreadPool::ThreadPool(size_t mthreads){
    maxThreads = mthreads;

    for(int i = 0;i < mthreads;i++){
        auto t = std::make_unique<SleepyThread>();
        auto* threadPointer = t.get();
        t->onAvailable = [this,i,threadPointer]{
            std::lock_guard<std::mutex> lock(this->updateMutex);
            
            this->nextThread.push(i);
            //std::cout << "Thread made available: " << i << std::endl;
        };
        this->nextThread.push(i);
        threads.push_back(std::move(t));
    }
}

void ThreadPool::deployPendingJobs(){
    std::lock_guard<std::mutex> lock(this->updateMutex);

    while(!pendingJobs.empty()){
        auto job = pendingJobs.front();
        if(deployInternal(job)) pendingJobs.pop();
        else break;
    }
}

bool ThreadPool::deployInternal(const std::function<void(void)>& func){
    if(nextThread.empty()) return false;
    //std::cout << "Deploying thread!" << std::endl;

    auto& next = threads[nextThread.front()];
    nextThread.pop();

    next->awake(func);

    return true;
}

bool ThreadPool::deploy(const std::function<void(void)>& func){
    std::lock_guard<std::mutex> lock(this->updateMutex);
    
    if(nextThread.empty()){
        pendingJobs.push(func);
        return true;
    }

    deployInternal(func);

    return true;
}

bool ThreadPool::finished(){
    return nextThread.size() == maxThreads && pendingJobs.empty();
}