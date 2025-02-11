#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>
#include <functional>
#include <filesystem>

class FileSaveStructure;

class FileStream{
    public:
        using Callback = std::function<void(FileStream*)>;
    private:
        std::string name;
        std::string path;
        std::fstream file_stream;
        Callback init;
        Callback load;

        friend class FileSaveStructure;

    public:
        FileStream(const std::string& name, const std::string& path, const Callback& init = nullptr, const Callback& load = nullptr):
        name(name), path(path){ set_callbacks(init,load); }

        void set_callbacks(const Callback& init, const Callback& load) {
            this->init = init;
            this->load = load;
        };
        void go_to(size_t position){
            stream().seekg(position, std::ios::beg);
        }
        void move(size_t position){
            stream().seekg(position, std::ios::cur);
        }
        bool open(const std::string& relative_path = "");
        std::fstream& stream() { return file_stream;};
};

class FileSaveStructure{
    private:
        std::string root_path = "";
        std::unordered_map<std::string, std::unique_ptr<FileStream>> saves;

    public:
        FileSaveStructure(const std::string& root_path);

        template <typename T, typename... Args>
        T* save(Args&&... args){
            auto new_save = std::make_unique<T>(std::forward<Args>(args)...);
            
            T* ptr = new_save.get();

            saves.emplace(new_save->name,std::move(new_save));

            return ptr;
        }

        void open();
        FileStream* get(const std::string& name);
};