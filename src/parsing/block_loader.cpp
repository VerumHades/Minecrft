#include <parsing/block_loader.hpp>

// https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
// trim from start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch) && ch != '\n';
    }));
}

// trim from end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch) && ch != '\n';
    }).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

void BlockLoader::loadFromSource(std::string source){
    size_t offset = 0;
    size_t next_end = 0;
    
    while(true){
        size_t next_end = source.find(";", offset);
        if(next_end == std::string::npos) break;

        size_t size = next_end - offset;

        std::string line = source.substr(offset, size);
        trim(line);

        if(line.starts_with("#")) continue; // Ignore comments

        auto command = split(line, " ");

        std::cout << line << std::endl;
        
        if(command.size() >= 8 && command[0] == "FULL_BLOCK")
            global_block_registry.addFullBlock(command[1], {command[2],command[3],command[4],command[5],command[6],command[7]});

        else if(command.size() >= 8 && command[0] == "FULL_BLOCK_TRANSPARENT")
            global_block_registry.addFullBlock(command[1], {command[2],command[3],command[4],command[5],command[6],command[7]}, true);

        else if(command.size() >= 3 && command[0] == "FULL_BLOCK")
            global_block_registry.addFullBlock(command[1], command[2]);

        else if(command.size() >= 3 && command[0] == "FULL_BLOCK_TRANSPARENT")
            global_block_registry.addFullBlock(command[1], command[2], true);

        else if(command.size() >= 3 && command[0] == "BILBOARD")
            global_block_registry.addBillboardBlock(command[1], command[2]);

        else std::cerr << "Invalid block definition: " << line << std::endl;
        
        offset += size + 1;
    }
}

void BlockLoader::loadFromFile(const std::string& path){
    loadFromSource(ShaderProgram::getSource(path));
}