#include <logging.hpp>

//%localappdata%/Majnkraft/logs

Logging::Logging(){
    std::time_t now = std::time(0);  // Get current system time
    std::tm* localTime = std::localtime(&now);  // Convert to local time
    std::ostringstream time_path;
    time_path << std::put_time(localTime, "%Y_%m_%d_%H_%M_%S_log.txt");

    auto path = Paths::Get(Paths::LOG_PATH);
    if(!path){
        SetPath(time_path.str());
        return;
    }
    
    outfile.open(path.value() / time_path.str());

    if (outfile.fail()) {
        std::cerr << "Log file open failed: " << strerror(errno) << std::endl; // Use strerror to get system error
    }
    if (outfile.bad()) {
        std::cerr << "A serious error occurred while opening the log file." << std::endl;
    }

    deleteOldLogs(path.value(), 3);
}

bool Logging::isLogOld(const fs::path& file, int daysThreshold) {
    // Extract the timestamp part from the filename
    std::string filename = file.filename().string();
    std::string timestamp_str = filename.substr(0, 19); // e.g. "2025-03-11_14:30:00"
    
    // Create a time_point from the extracted timestamp
    std::tm tm = {};
    std::istringstream ss(timestamp_str);
    ss >> std::get_time(&tm, "%Y_%m_%d_%H_%M_%S");
    if (ss.fail()) {
        LogError("Failed to parse timestamp: ", timestamp_str);
        return false;
    }

    // Convert to time_point
    auto file_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));

    // Get the current time and calculate the threshold time
    auto now = std::chrono::system_clock::now();
    auto threshold = now - std::chrono::hours(24 * daysThreshold);

    // Compare if the file is older than the threshold
    return file_time < threshold;
}

void Logging::deleteOldLogs(const fs::path& directory, int daysThreshold) {
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.is_regular_file() && isLogOld(entry.path(), daysThreshold)) {
                LogInfo("Deleting old log: ", entry.path().string());
                fs::remove(entry.path());
            }
        }
    } catch (const std::exception& e) {
        LogError("Error: ", e.what());
    }
}

void Logging::Message(const std::string& descriptor, const std::string& message, int line, const char* file){
    std::lock_guard<std::mutex> lock(mtx);

    if(!outfile.is_open()) return;
    
    std::time_t now = std::time(0);  // Get current system time
    std::tm* localTime = std::localtime(&now);  // Convert to local time

    outfile << std::put_time(localTime, "%Y-%m-%d %H:%M:%S \"") << file << "\" [at line " << line << "] <" << descriptor << "> " << message << std::endl;
    //std::cout << std::put_time(localTime, "%Y-%m-%d %H:%M:%S \"") << file << "\" [at line " << line << "] <" << descriptor << "> " << message << std::endl;
}

void Logging::SetPath(const fs::path& path){
    std::lock_guard<std::mutex> lock(mtx);

    outfile.open(path);

    if(!outfile.is_open()){
        std::cerr << "Failed to open log file '" << path << "'." << std::endl;
    }
};

void Logging::SaveTrace(){
    std::lock_guard<std::mutex> lock(mtx);

    if(!outfile.is_open()) return;

    auto trace = cpptrace::generate_trace();
    
    outfile << "------------ Stack trace ---------------" << std::endl;
    for (const auto& frame : trace) {
        outfile << frame << "\n";
    }
    outfile << "----------------------------------------" << std::endl;
}

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei length,
    const GLchar *message, const void *param)
{
    const char *source_, *type_, *severity_;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             source_ = "API";             break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_ = "WINDOW_SYSTEM";   break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: source_ = "SHADER_COMPILER"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     source_ = "THIRD_PARTY";     break;
    case GL_DEBUG_SOURCE_APPLICATION:     source_ = "APPLICATION";     break;
    case GL_DEBUG_SOURCE_OTHER:           source_ = "OTHER";           break;
    default:                              source_ = "<SOURCE>";        break;
    }

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               type_ = "ERROR";               break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_ = "DEPRECATED_BEHAVIOR"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_ = "UDEFINED_BEHAVIOR";   break;
    case GL_DEBUG_TYPE_PORTABILITY:         type_ = "PORTABILITY";         break;
    case GL_DEBUG_TYPE_PERFORMANCE:         type_ = "PERFORMANCE";         break;
    case GL_DEBUG_TYPE_OTHER:               type_ = "OTHER";               break;
    case GL_DEBUG_TYPE_MARKER:              type_ = "MARKER";              break;
    default:                                type_ = "<TYPE>";              break;
    }

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         severity_ = "HIGH";         break;
    case GL_DEBUG_SEVERITY_MEDIUM:       severity_ = "MEDIUM";       break;
    case GL_DEBUG_SEVERITY_LOW:          severity_ = "LOW";          break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: severity_ = "NOTIFICATION"; return; // Ignore
    default:                             severity_ = "<SEVERITY>";   break;
    }

    LogOpengl("{}: GL {} {} ({}): {}", id, severity_, type_, source_, message);

    //if(severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) Logging::Get().SaveTrace();
}

void CheckGLError(const char *file, int line){
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        const char *errorString;
        switch (error) {
            case GL_INVALID_ENUM:                  errorString = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 errorString = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             errorString = "GL_INVALID_OPERATION"; break;
            //case GL_STACK_OVERFLOW:                errorString = "GL_STACK_OVERFLOW"; break;
            //case GL_STACK_UNDERFLOW:               errorString = "GL_STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 errorString = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               errorString = "Unknown error"; break;
        }

        //Logging::Get().Message("OPENGL_ERROR",errorString,line,file);
    }
}