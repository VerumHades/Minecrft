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

/**
 * @brief A helper class that maintains a structure of open file streams based on a root path and relative paths
 * 
 */
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

        /**
         * @brief Registers filestream save
         * 
         * @tparam T filestream type
         * @tparam Args arguments for filestream constructor
         * @param name name of the filestream
         * @param relative_path its relative path
         * @param args arguments for the filestream constructor
         * @return T* reference to created filestream
         */
        template <typename T, typename... Args>
        T* RegisterSave(const std::string& name, const fs::path& relative_path, Args&&... args){
            auto new_save = std::make_unique<T>(std::forward<Args>(args)...);

            T* ptr = new_save.get();

            saves.emplace(name, RegisteredSave{name, relative_path, std::move(new_save)});

            return ptr;
        }

        /**
         * @brief Actually opens all registered streams
         * 
         */
        void Open();

        /**
         * @brief Returns a reference to a filestream by name
         * 
         * @param name 
         * @return FileStream* 
         */
        FileStream* get(const std::string& name);
};
