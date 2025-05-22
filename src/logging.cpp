#include <logging.hpp>

//%localappdata%/Majnkraft/logs

Logging::Logging() {
    std::string log_name = "log.txt";
    std::filesystem::path final_path;

    // 1. Try LocalAppData (or whatever Paths::LOG_PATH points to)
    auto path = Paths::Get(Paths::LOG_PATH);
    if (path) {
        std::error_code ec;
        std::filesystem::create_directories(path.value(), ec); // Ensure directory exists
        if (ec) {
            std::cerr << "Failed to create log directory: " << ec.message() << std::endl;
        } else {
            final_path = path.value() / log_name;
            outfile.open(final_path);
        }
    }

    // 2. Fallback to Current Working Directory
    if (!outfile.is_open()) {
        final_path = std::filesystem::current_path() / log_name;
        outfile.open(final_path);
        if (outfile.is_open()) {
            std::cout << "Fell back to current working directory: " << final_path.string() << "\n";;
        }
    }

    // 3. Fallback to Temporary Directory
    if (!outfile.is_open()) {
        final_path = std::filesystem::temp_directory_path() / log_name;
        outfile.open(final_path);
        if (outfile.is_open()) {
            std::cout << "Fell back to temporary directory: " << final_path.string() << "\n";;
        }
    }

    // Logging result
    if (outfile.is_open()) {
        std::cout << "Logging to: " << std::filesystem::absolute(final_path) << std::endl;
    } else {
        std::cerr << "Failed to open log file in all attempted locations.\n";
        std::cerr << "Last attempted path: " << final_path << "\n";
        std::cerr << "Error: " << strerror(errno) << std::endl;
    }

    if (outfile.bad()) {
        std::cerr << "A serious error occurred while opening the log file." << std::endl;
    }
}

void Logging::Message(const std::string& descriptor, const std::string& message, int line, const char* file) {
    std::lock_guard<std::mutex> lock(mtx);

    
    std::time_t now    = std::time(0);         // Get current system time
    std::tm* localTime = std::localtime(&now); // Convert to local time

    if (!outfile.is_open())
        std::cout << std::put_time(localTime, "%Y-%m-%d %H:%M:%S \"") << file << "\" [at line " << line << "] <" << descriptor << "> " << message << std::endl;
    else{
        outfile << std::put_time(localTime, "%Y-%m-%d %H:%M:%S \"") << file << "\" [at line " << line << "] <" << descriptor << "> " << message << std::endl;

        if (outfile.tellp() >= boundary) {
            outfile.seekp(0);  // wrap around
        }
    }
    // std::cout << std::put_time(localTime, "%Y-%m-%d %H:%M:%S \"") << file << "\" [at line " << line << "] <" << descriptor << "> " << message << std::endl;
}

void Logging::SetPath(const fs::path& path) {
    std::lock_guard<std::mutex> lock(mtx);

    outfile.open(path);

    if (!outfile.is_open()) {
        std::cerr << "Failed to open log file '" << path << "'." << std::endl;
    }
};

void Logging::SaveTrace() {
    std::lock_guard<std::mutex> lock(mtx);

    
    auto trace = cpptrace::generate_trace();

    if (!outfile.is_open()){
        std::cout  << "------------ Stack trace ---------------" << std::endl;
        for (const auto& frame : trace) {
            std::cout << frame << "\n";
        }
        std::cout << "----------------------------------------" << std::endl;   
    }
    else{
        outfile << "------------ Stack trace ---------------" << std::endl;
        for (const auto& frame : trace) {
            outfile << frame << "\n";
        }
        outfile << "----------------------------------------" << std::endl;
    }
}

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* param) {
    const char *source_, *type_, *severity_;

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        source_ = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        source_ = "WINDOW_SYSTEM";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        source_ = "SHADER_COMPILER";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        source_ = "THIRD_PARTY";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        source_ = "APPLICATION";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        source_ = "OTHER";
        break;
    default:
        source_ = "<SOURCE>";
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        type_ = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        type_ = "DEPRECATED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        type_ = "UDEFINED_BEHAVIOR";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        type_ = "PORTABILITY";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        type_ = "PERFORMANCE";
        break;
    case GL_DEBUG_TYPE_OTHER:
        type_ = "OTHER";
        break;
    case GL_DEBUG_TYPE_MARKER:
        type_ = "MARKER";
        break;
    default:
        type_ = "<TYPE>";
        break;
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        severity_ = "HIGH";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severity_ = "MEDIUM";
        break;
    case GL_DEBUG_SEVERITY_LOW:
        severity_ = "LOW";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severity_ = "NOTIFICATION";
        return; // Ignore
    default:
        severity_ = "<SEVERITY>";
        break;
    }

    static int opengl_error_timout = 10000;
    static int opengl_error_counter = 0;

    if(opengl_error_counter <= 0) {    
        LogOpengl("{}: GL {} {} ({}): {}", id, severity_, type_, source_, message);
        opengl_error_counter = opengl_error_timout;
    }
    else opengl_error_counter--;

    // if(severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) Logging::Get().SaveTrace();
}

void CheckGLError(const char* file, int line) {
    
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        const char* errorString;
        switch (error) {
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        // case GL_STACK_OVERFLOW:                errorString = "GL_STACK_OVERFLOW"; break;
        // case GL_STACK_UNDERFLOW:               errorString = "GL_STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        default:
            errorString = "Unknown error";
            break;
        }

        static int opengl_error_timout = 10000;
        static int opengl_error_counter = 0;

        if(opengl_error_counter <= 0) {    
            Logging::Get().Message("OPENGL_ERROR", errorString, line, file);
            Logging::Get().SaveTrace();
            opengl_error_counter = opengl_error_timout;
        }
        else opengl_error_counter--;
    }
}
