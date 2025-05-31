#include <structure/streams/file_stream.hpp>

FileStream::FileStream(const Callback& init, const Callback& load) : init(init), load(load) {

}

bool FileStream::Read(size_t offset, size_t size, byte* buffer) {
    std::lock_guard lock(mutex);

    if (offset + size > Size())
        return false;

    stream.seekg(offset, std::ios::beg);
    stream.read(reinterpret_cast<char*>(buffer), size);
    if (stream.fail()){
        LogError("Failed to read from filestream: {} Error: {}", this->path.string(), strerror(errno));
        return false;
    }


    return true;
}
bool FileStream::Write(size_t offset, size_t size, const byte* buffer) {
    std::lock_guard lock(mutex);

    stream.seekp(offset, std::ios::beg);
    stream.write(reinterpret_cast<const char*>(buffer), size);

    if (offset + size > filesize)
        filesize = offset + size;

    if (stream.fail()){
        LogError("Failed to write to filestream: {} Error: {}", this->path.string(), strerror(errno));
        return false;
    }

    return true;
}

size_t FileStream::Size() {
    return filesize;
}

void FileStream::SetCallbacks(const Callback& init, const Callback& load) {
    this->init = init;
    this->load = load;
};

bool FileStream::Open(const fs::path& path) {
    bool newlyCreated = false;

    this->path = path;
    {
        std::lock_guard lock(mutex);

        fs::path dir_path = path.parent_path();

        if (!dir_path.empty() && !fs::exists(dir_path)) {
            if (!fs::create_directories(dir_path)){
                LogError("Failed to create path for: {} Error: {}", this->path.string(), strerror(errno));
                throw std::logic_error("Failed to create required path.");
            }
        }

        if (!fs::exists(path)) {
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open()) {
                LogError("Failed to create path for: {} Error: {}", this->path.string(), strerror(errno));
                throw std::logic_error("Failed to create required path.");
            }
            file.close();
            newlyCreated = true;
        }

        stream = std::fstream(path, std::ios::out | std::ios::in | std::ios::binary);
        stream.seekg(0, std::ios::end); // Move to the end of the file
        filesize = stream.tellg();
        stream.seekg(0, std::ios::beg); // Move back

        if (!stream.is_open()){
            LogError("Failed to open filestream: {} Error: {}", this->path.string(), strerror(errno));
            return false;
        }
    }

    if (newlyCreated && init)
        init(this);
    else if (load)
        load(this);

    return true;
}
