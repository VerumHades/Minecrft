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

void Service::Stop(const std::string& name, int timeout) {
    auto* module = GetModule(name);
    if (!module)
        return;
    if (!module->running)
        return;

    module->should_stop = true;
    while (module->running)
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
    module->should_stop = false;
}

void Service::StopAll(int timeout) {
    for (auto& [name, module] : registered_modules)
        Stop(name, timeout);
}

void Service::Start(const std::string& name, bool restart) {
    auto* module = GetModule(name);
    if (!module)
        return;
    if (module->running && !restart)
        return;
    if (module->running)
        Stop(name);

    module->should_stop = false;
    module->running = true;
    std::thread module_thread([module, name]() {
        module->function(module->should_stop);
        module->running = false;
    });

    module_thread.detach();
}

void Service::StartAll(bool restart) {
    for (auto& [name, module] : registered_modules)
        Start(name, restart);
}

void Service::AddModule(const std::string& name, const ModuleFunction& module_function) {
    registered_modules.emplace(name, std::make_unique<Module>(false, false, module_function));
}

bool Service::IsRunning(const std::string& name) const {
    auto* module = GetModule(name);
    if (!module)
        return false;

    return module->running.load();
}
