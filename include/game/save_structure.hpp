#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>
#include <functional>
#include <filesystem>

#include <structure/bitworks.hpp>
#include <structure/streams/file_stream.hpp>

#include <path_config.hpp>


class FileSaveStructure{
    private:
        std::string root_path = "";
        struct RegisteredSave{
            std::string name;
            fs::path relative_path;
            std::unique_ptr<FileStream> file_stream;
        };

        std::unordered_map<std::string, RegisteredSave> saves;

    public:
        FileSaveStructure(const std::string& root_path);

        template <typename T, typename... Args>
        T* RegisterSave(const std::string& name, const fs::path& relative_path, Args&&... args){
            auto new_save = std::make_unique<T>(std::forward<Args>(args)...);
            
            T* ptr = new_save.get();

            saves.emplace(name, RegisteredSave{name, relative_path, std::move(new_save)});

            return ptr;
        }

        void Open();
        FileStream* get(const std::string& name);
};