
#pragma once

#include <glad/glad.h>

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
#include <cxxabi.h>
#include <mutex>

#include <cpptrace/cpptrace.hpp>
#include <path_config.hpp>


#define LOG_ERROR
#define LOG_WARNING
#define LOG_MESSAGE
#define LOG_INFO
#define LOG_OPENGL


#ifdef LOG_MESSAGE
    #define LogMessage(...) Logging::Get().Message("MESSAGE", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogMessage(...)
#endif

#ifdef LOG_WARNING
    #define LogWarning(...) Logging::Get().Message("WARNING", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogWarning(...)
#endif

#ifdef LOG_ERROR
    #define LogError(...) {Logging::Get().Message("ERROR", std::format(__VA_ARGS__), __LINE__, __FILE__); }
#else
    #define LogError(...)
#endif

#ifdef LOG_INFO
    #define LogInfo(...) Logging::Get().Message("INFO", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogInfo(...)
#endif

#ifdef LOG_OPENGL
    #define LogOpengl(...) Logging::Get().Message("OPENGL", std::format(__VA_ARGS__), __LINE__, __FILE__)
#else
    #define LogOpengl(...)
#endif

class Logging{
    private:
        std::ofstream outfile;
        std::mutex mtx;

        Logging();

        bool isLogOld(const fs::path& file, int daysThreshold);
        void deleteOldLogs(const fs::path& directory, int daysThreshold);

    public:
        void SetPath(const fs::path& path);
        void Message(const std::string& descriptor, const std::string& message, int line, const char* file);
        void SaveTrace();

        static Logging& Get(){
            static Logging logging;
            return logging;
        }
};

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,const GLchar *message, const void *param);


#define GL_CALL(call) call; CheckGLError(__FILE__, __LINE__);
void CheckGLError(const char *file, int line);