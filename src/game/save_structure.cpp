#include <game/save_structure.hpp>

namespace fs = fs;

FileSaveStructure::FileSaveStructure(const std::string& root_path): root_path(root_path) {}

FileStream* FileSaveStructure::get(const std::string& name){
    if(!saves.contains(name)) return nullptr;
    return saves.at(name).file_stream.get();
}

void FileSaveStructure::Open(){
    for(auto& [name,save]: saves){
        auto full_path = root_path / save.relative_path / fs::path(name + ".dat");

        save.file_stream->Open(full_path);
    }
}