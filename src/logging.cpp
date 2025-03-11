#include <logging.hpp>

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
    auto& instance = Get();
    if(!instance.outfile.is_open()) return;
    
    std::time_t now = std::time(0);  // Get current system time
    std::tm* localTime = std::localtime(&now);  // Convert to local time

    instance.outfile << std::put_time(localTime, "%Y-%m-%d %H:%M:%S \"") << file << "\" [at line " << line << "] <" << descriptor << "> " << message << std::endl;
}

void Logging::SetPath(const fs::path& path){
    auto& instance = Get();
    instance.outfile.open(path);

    if(!instance.outfile.is_open()){
        std::cerr << "Failed to open log file '" << path << "'." << std::endl;
    }
};