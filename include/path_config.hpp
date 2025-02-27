#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <optional>

namespace fs = std::filesystem;
using path = fs::path;

class Paths {
    public:
        enum Configured{
            GAME_SAVES,
            GAME_STRUCTURES
        };
    private:
        std::unordered_map<Configured, path> configured_paths = {
            {GAME_SAVES, "saves"},
            {GAME_STRUCTURES, "structures"}
        };
        std::optional<path> save_path = std::nullopt;
        Paths(){}

        static Paths& GetInstance(){
            static Paths instance{};
            return instance;
        }

        path* GetSavePath(){
            if(!save_path){
                #ifdef WINDOWS_PACKAGED_BUILD
                const char* appData = std::getenv("LOCALAPPDATA");
                const char* userProfile = std::getenv("USERPROFILE");

                if (appData)          save_path = path(appData) / "Majnkraft";
                else if (userProfile) save_path = path(userProfile) / "AppData" / "Local" / "Majnkraft";

                #else
                save_path = path("appdata");
                #endif

                if(!fs::exists(save_path.value()) && !fs::create_directories(save_path.value())) 
                    return nullptr;
            }

            return &save_path.value();
        };

    public:
        std::optional<path> RequireDirectory(const path& path){
            if(
                !fs::exists(path) &&
                !fs::create_directories(path)
            ) return std::nullopt;

            return path;
        }
        std::optional<path> RequireSaveDirectory(const path& path){
            auto save_path = GetSavePath();
            if(!save_path) return std::nullopt;
            return RequireDirectory(*save_path / path);
        }

        static std::optional<path> Get(const Configured& name){
            return GetInstance().RequireSaveDirectory(GetInstance().configured_paths.at(name));
        }
};