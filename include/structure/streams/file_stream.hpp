#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <functional>
#include <filesystem>
#include <mutex>

#include <structure/streams/buffer.hpp>

#include <path_config.hpp>

class FileStream: public Buffer{
    public:
        using Callback = std::function<void(FileStream*)>;

    private:
        std::string name;
        fs::path path;

        std::fstream stream;
        std::mutex mutex;

        Callback init;
        Callback load;

    public:
        FileStream(const Callback& init = nullptr, const Callback& load = nullptr);
        virtual ~FileStream() = default;

        bool Read(size_t offset, size_t size, byte* buffer) override;
        bool Write(size_t offset, size_t size, const byte* buffer) override;
        size_t Size() override;

        void SetCallbacks(const Callback& init, const Callback& load);
        bool Open(const fs::path& path);
};
