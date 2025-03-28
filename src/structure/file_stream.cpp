#include <structure/streams/file_stream.hpp>

FileStream::FileStream(
    const Callback& init = nullptr, 
    const Callback& load = nullptr):
    init(init), load(load)
{

}

bool FileStream::Read(size_t offset, size_t size, byte* buffer) {
    std::lock_guard lock(mutex);

    stream.seekg(offset, std::ios::beg);
    stream.read(reinterpret_cast<char*>(buffer), size);
}
bool FileStream::Write(size_t offset, size_t size, byte* buffer) {
    std::lock_guard lock(mutex);

    stream.seekp(offset, std::ios::beg);
    stream.write(reinterpret_cast<char*>(buffer), size);
}
size_t FileStream::Size() {
    std::lock_guard lock(mutex);

    stream.seekg(0, std::ios::end); // Move to the end of the file
    return stream.tellg();
}

void FileStream::SetCallbacks(const Callback& init, const Callback& load) {
    this->init = init;
    this->load = load;
};

bool FileStream::Open(const fs::path& path){
    std::lock_guard lock(mutex);
    
    fs::path dir_path = path.parent_path();

    if (!dir_path.empty() && !fs::exists(dir_path)) {
        if (!fs::create_directories(dir_path)) return false;
    }

    bool newlyCreated = false;
    
    if(!fs::exists(path)){
        std::ofstream file(path, std::ios::binary);
        if(!file.is_open()){
            std::terminate();
        }
        file.close();
        newlyCreated = true;
    }
    
    stream = std::fstream(path, std::ios::out | std::ios::in | std::ios::binary);

    if(!stream.is_open()) return false;

    if(newlyCreated && init) init(this);
    else if(load) load(this);

    return true;
}