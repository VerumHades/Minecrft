#pragma once

#include <logging.hpp>

#include <unordered_map>
#include <string>
#include <functional>
#include <atomic>
#include <thread>

/**
 * @brief A thread job manager that allows termination of jobs (modules)
 * 
 */
class Service{
    private:
        using ModuleFunction = std::function<void(std::atomic<bool>& stop_signal)>;    
        struct Module{
            std::atomic<bool> should_stop = false;
            ModuleFunction function;
            std::optional<std::thread> thread;
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

        /**
         * @brief Stop a module
         * 
         * @param name 
         */
        void Stop(const std::string& name);
        /**
         * @brief Stop all modules
         * 
         */
        void StopAll();

        /**
         * @brief Start a module by name
         * 
         * @param name 
         * @param restart whether to restart it if its already running
         */
        void Start(const std::string& name, bool restart = false);
        /**
         * @brief Start all modules
         * 
         * @param restart whether to restart a module if its already running
         */
        void StartAll(bool restart = false);

        bool IsRunning(const std::string& name) const;

        /**
         * @brief Add a job
         * 
         * @param name 
         * @param module_function a function that takes and atomic singnal to stop
         */
        void AddModule(const std::string& name, const ModuleFunction& module_function);
};  