#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <optional>

namespace fs = std::filesystem;
using path   = fs::path;

/**
 * @brief A configuration class with some preset paths for saving
 * 
 */
class Paths {
  public:
    enum Configured { GAME_SAVES, GAME_STRUCTURES, LOG_PATH };

  private:
    std::unordered_map<Configured, path> configured_paths = {
        {GAME_SAVES, "saves"}, {GAME_STRUCTURES, "structures"}, {LOG_PATH, "logs"}};
    std::optional<path> save_path = std::nullopt;
    Paths() {}

    static Paths& GetInstance() {
        static Paths instance{};
        return instance;
    }

    std::optional<fs::path> GetSavePath() {
        if (!save_path) {
            const char* userProfile = std::getenv("USERPROFILE");

            if (userProfile) {
                save_path = fs::path(userProfile) / "AppData" / "Local" / "Majnkraft";
            } else {
                // Fallback if USERPROFILE is not set
                save_path = fs::path("appdata");
            }

            // Try to create the directory if it doesn't exist
            if (!fs::exists(save_path.value()) && !fs::create_directories(save_path.value())) {
                return std::nullopt;
            }
        }

        return save_path;
    }

  public:
    std::optional<path> RequireDirectory(const path& path) {
        if (!fs::exists(path) && !fs::create_directories(path))
            return std::nullopt;

        return path;
    }
    std::optional<path> RequireSaveDirectory(const path& path) {
        auto save_path = GetSavePath();
        if (!save_path)
            return std::nullopt;
        return RequireDirectory(*save_path / path);
    }

    static std::optional<path> Get(const Configured& name) {
        return GetInstance().RequireSaveDirectory(GetInstance().configured_paths.at(name));
    }
};
