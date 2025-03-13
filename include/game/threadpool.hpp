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

template <typename... Args>
class ThreadPool{
    private:
        std::mutex updateMutex;
        std::queue<size_t> nextThread;
        std::vector<std::unique_ptr<SleepyThread>> threads = {};
        
        using TaskArguments = std::tuple<Args...>;
        
        std::queue<TaskArguments> pending_tasks;
        size_t max_batch_size = 20;
        std::function<void(Args...)> proccess_task;

        void DeployNextBatch(){
            if(pending_tasks.size() <= 0 || nextThread.size() == 0) return;

            size_t batch_size = std::min(max_batch_size, pending_tasks.size());

            auto& next = threads[nextThread.front()];
            nextThread.pop();
            
            std::vector<TaskArguments> arguments{};
            argument.reserve(batch_size);
            for(int i = 0;i < batch_size;i++){
                arguments.push_back(pending_tasks.front());
                pending_tasks.pop();
            }

            next->awake([arguments](){
                for(auto& argument: arguments) std::apply(process_task, argument);
            });
        }

    public:
        ThreadPool::ThreadPool(std::function<void(Args...)> proccess_task): proccess_task(proccess_task){
            size_t maxThreads = std::thread::hardware_concurrency();
        
            for(int i = 0;i < mthreads;i++){
                auto t = std::make_unique<SleepyThread>();
                auto* threadPointer = t.get();
                t->onAvailable = [this,i,threadPointer]{
                    std::lock_guard<std::mutex> lock(this->updateMutex);
                    
                    this->nextThread.push(i);
                    DeployNextBatch();
                };
                this->nextThread.push(i);
                threads.push_back(std::move(t));
            }
        }

        void Schedule(Args... arguments){
            if(nextThread.size() > 0) DeployNextBatch();
            else pending_tasks.push({arguments...});
        }
};

template <typename... Args>
class IterativeThreadPool{
    private:
        std::vector<std::unique_ptr<SleepyThread>> threads = {};
        
        using TaskArguments = std::tuple<Args...>;

        std::function<void(Args...)> proccess_task;
        std::function<std::tuple<bool,TaskArguments>(void)> get_next_task;

        std::atomic<bool> halt = false;
        std::atomic<int> finished_count = 0;

        size_t maxThreads = 1;

    public:
        ThreadPool::ThreadPool(std::function<void(Args...)> proccess_task): proccess_task(proccess_task){
            maxThreads = std::thread::hardware_concurrency();
        
            for(int i = 0;i < mthreads;i++){
                auto t = std::make_unique<SleepyThread>();
                auto* threadPointer = t.get();
                t->onAvailable = [this,i,threadPointer]{};
                threads.push_back(std::move(t));
            }
        }

        void Start(const std::function<std::tuple<bool,TaskArguments>(void)>& get_next_task){
            finished_count = 0;
            
            for(auto& thread: threads){
                thread->awake([this, &get_next_task](){
                    while(!halt){
                        auto [has_next_task, arguments] = get_next_task();

                        if(!has_next_task) break;

                        std::apply(proccess_task, arguments);
                    }
                    finished_count++;
                });
            }
        }

        void WaitForStop(){
            while(finished_count != maxThreads){
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }

        void Halt(){
            halt = true;
            WaitForStop();
        }
};