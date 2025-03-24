#pragma once

#include <logging.hpp>

#include <unordered_map>
#include <string>
#include <functional>
#include <atomic>
#include <thread>

class Service{
    private:
        using ModuleFunction = std::function<void(std::atomic<bool>& stop_signal)>;    
        struct Module{
            std::atomic<bool> running = false;
            std::atomic<bool> should_stop = false;
            ModuleFunction function;
        };

        std::unordered_map<std::string, std::unique_ptr<Module>> registered_modules;    

        Module* GetModule(const std::string& name);
        const Module* GetModule(const std::string& name)  const;

    public:
        Service() = default;
        ~Service();

        Service(const Service& other) = delete;
        Service& operator=(const Service& other) = delete;
        Service(Service&& other) = delete;
        Service& operator=(Service&& other) = delete;

        void Stop(const std::string& name, int timeout = 10);
        void StopAll(int timeout = 10);

        void Start(const std::string& name, bool restart = false);
        void StartAll(bool restart = false);

        bool IsRunning(const std::string& name) const;

        void AddModule(const std::string& name, const ModuleFunction& module_function);
};  