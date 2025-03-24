#include <structure/service.hpp>

Service::~Service(){
    StopAll();
}

Service::Module* Service::GetModule(const std::string& name){
    if(!registered_modules.contains(name)) return nullptr;
    return &registered_modules.at(name);
}

void Service::Stop(const std::string& name, int timeout){
    auto* module = GetModule(name);
    if(!module) return;
    if(!module->running) return;

    module->should_stop = true;
    while(module->running) std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    module->should_stop = false;
}

void Service::StopAll(int timeout){
    for(auto& [name, module]: registered_modules) Stop(name);
}

void Service::Start(const std::string& name, bool restart = false){
    auto* module = GetModule(name);
    if(!module) return;

    std::thread module_thread([module](){
        module->running = true;
        module->function(module->should_stop);
        module->running = false;
    });

    module_thread.detach();
}
void Service::StartAll(bool restart = false){
    for(auto& [name, module]: registered_modules) Start(name);
}

void Service::AddModule(const std::string& name, const ModuleFunction& module_function){
    for(auto& [name, module]: registered_modules){
        if(!module.running) continue;
        LogError("Cannot add module to service when any modules are running.");
        return;
    }

    registered_modules.emplace(name, Module{false, false, module_function});
}
