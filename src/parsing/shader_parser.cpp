#include <parsing/shader_parser.hpp>

ShaderProgramSource::ShaderProgramSource(std::string source){

    bool capturing = false;
    size_t capture_start = 0;
    std::string capture_name = "";

    bool first_token = true;

    Token current_token = Token::from(source, 0);
    do{
        if(!first_token) current_token = current_token.next();
        else first_token = false;

        if(current_token.type() != Token::Type::MACRO) continue;

        Token macro_id = current_token.subtoken(1);

        if(macro_id.content() != "shader") continue;

        Token shader_type = macro_id.next().next();

        std::string shader_type_name = shader_type.content();
        
        if(!capturing){
            capture_start = current_token.end();
            capture_name = shader_type_name;
            capturing = true;
            continue;
        }

        ShaderSource shader_source = {};

        if     (capture_name == "vertex")   shader_source.shader_type = GL_VERTEX_SHADER;
        else if(capture_name == "fragment") shader_source.shader_type = GL_FRAGMENT_SHADER;
        else if(capture_name == "compute")  shader_source.shader_type = GL_COMPUTE_SHADER;
        else {
            std::cerr << "Shader type '" << capture_name << "' not recognized." << std::endl;
            continue;
        }

        shader_source.source = source.substr(capture_start, (current_token.start() - 1) - capture_start);
        shader_sources.push_back(shader_source);

        capture_start = current_token.end();
        capture_name = shader_type_name;
        capturing = true;

    } while(current_token.hasNext());

    if(capturing){
        ShaderSource shader_source = {};

        if     (capture_name == "vertex")   shader_source.shader_type = GL_VERTEX_SHADER;
        else if(capture_name == "fragment") shader_source.shader_type = GL_FRAGMENT_SHADER;
        else if(capture_name == "compute")  shader_source.shader_type = GL_COMPUTE_SHADER;
        else {
            std::cerr << "Shader type '" << capture_name << "' not recognized." << std::endl;
            return;
        }

        shader_source.source = source.substr(capture_start, source.size() - capture_start);
        shader_sources.push_back(shader_source);
    }
}

ShaderProgramSource ShaderProgramSource::fromFile(std::string path){
    std::ifstream file(path);  // Open the file
    if (!file.is_open()) {              // Check if the file is open
        std::cerr << "Failed to open shader source file: " << path << std::endl;
        throw std::runtime_error("");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();             

    std::string source = buffer.str();

    file.close();  // Close the file

    return ShaderProgramSource::fromSource(source);
}

ShaderProgramSource ShaderProgramSource::fromSource(std::string source){
    return ShaderProgramSource(source);
}