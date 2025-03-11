
#pragma once

#include <fstream>
#include <ctime>  // For time functions
#include <string>
#include <iostream>
#include <iomanip>
#include <format>
#include <ostream>
#include <filesystem>
#include <sstream>
#include <cstring>

#include <path_config.hpp>

#define LOG_ERROR
#define LOG_WARNING
#define LOG_MESSAGE
#define LOG_INFO
#define LOG_OPENGL


#ifdef LOG_MESSAGE
    #define LogMessage(...) Logging::Message("MESSAGE", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogMessage(...)
#endif

#ifdef LOG_WARNING
    #define LogWarning(...) Logging::Message("WARNING", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogWarning(...)
#endif

#ifdef LOG_ERROR
    #define LogError(...) Logging::Message("ERROR", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogError(...)
#endif

#ifdef LOG_INFO
    #define LogInfo(...) Logging::Message("INFO", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogInfo(...)
#endif

#ifdef LOG_OPENGL
    #define LogOpengl(...) Logging::Message("OPENGL", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogOpengl(...)
#endif

class Logging{
    private:
        std::ofstream outfile;

        Logging();

        bool isLogOld(const fs::path& file, int daysThreshold);
        void deleteOldLogs(const fs::path& directory, int daysThreshold);
        

        static Logging& Get(){
            static Logging logging;
            return logging;
        }

    public:
        static void SetPath(const fs::path& path);
        static void Message(const std::string& descriptor, const std::string& message, int line, const char* file);
};