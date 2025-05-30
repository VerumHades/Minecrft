#include <structure/service.hpp>

Service::~Service() {
    StopAll();
}

Service::Module* Service::GetModule(const std::string& name) {
    if (!registered_modules.contains(name))
        return nullptr;
    return registered_modules.at(name).get();
}

const Service::Module* Service::GetModule(const std::string& name) const {
    if (!registered_modules.contains(name))
        return nullptr;
    return registered_modules.at(name).get();
}

void Service::Stop(const std::string& name) {
    auto* module = GetModule(name);
    if (!module || !module->thread || !module->thread->joinable())
        return;

    module->should_stop = true;
    if(module->thread->joinable())
        module->thread->join();

    module->thread = std::nullopt;
    module->should_stop = false;
}

void Service::StopAll() {
    for (auto& [name, module] : registered_modules)
        Stop(name);
}

void Service::Start(const std::string& name, bool restart) {
    auto* module = GetModule(name);
    if (!module)
        return;
    if (module->thread->joinable()){
        if(!restart) return
        Stop(name);
    }

    module->should_stop = false;
    module->thread = std::thread([module, name]() {
        module->function(module->should_stop);
    });
}

void Service::StartAll(bool restart) {
    for (auto& [name, module] : registered_modules)
        Start(name, restart);
}

void Service::AddModule(const std::string& name, const ModuleFunction& module_function) {
    registered_modules.emplace(name, std::make_unique<Module>(false, module_function));
}

bool Service::IsRunning(const std::string& name) const {
    auto* module = GetModule(name);
    if (!module)
        return false;

    return module->thread->joinable();
}
