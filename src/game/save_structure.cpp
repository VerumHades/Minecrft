#include <game/save_structure.hpp>

namespace fs = std::filesystem;

bool FileStream::open(const std::string& relative_path){
    fs::path full_path = fs::path(relative_path).append(path).append(name + ".dat");
    fs::path dir_path = full_path.parent_path();

    if (!dir_path.empty() && !fs::exists(dir_path)) {
        if (!fs::create_directories(dir_path)) return false;
    }

    bool newlyCreated = false;
    
    if(!fs::exists(full_path)){
        std::ofstream file(full_path, std::ios::binary);
        if(!file.is_open()){
            std::terminate();
        }
        file.close();
        newlyCreated = true;
    }
    
    file_stream = std::fstream(full_path, std::ios::in | std::ios::out | std::ios::binary);

    if(!file_stream.is_open()) return false;

    if(newlyCreated && init) init(this);
    else if(load) load(this);

    return true;
}
FileSaveStructure::FileSaveStructure(const std::string& root_path): root_path(root_path) {}

FileStream* FileSaveStructure::get(const std::string& name){
    if(!saves.contains(name)) return nullptr;
    return saves.at(name).get();
}

void FileSaveStructure::open(){
    for(auto& [name,save]: saves){
        save->open(root_path);
    }
}