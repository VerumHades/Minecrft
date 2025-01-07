#include <parsing/source_template.hpp>

SourceTemplate SourceTemplate::fromSource(std::string source){
    SourceTemplate output_template{};

    size_t current_start = 0;
    while(current_start < source.size()){
        size_t tag_start = source.find("[[", current_start);
        if(tag_start == 0) output_template.first_is_part = false;

        if(tag_start == std::string::npos) break; // No tag found
        
        size_t tag_end = source.find("]]", tag_start);

        if(tag_end == std::string::npos){
            std::cerr << "Tag with no end in template!" << std::endl;
            break; // No tag found
        }

        std::string tag = source.substr(tag_start + 2, tag_end - (tag_start + 2));

        if(output_template.tags.contains(tag)) {
            std::cerr << "Duplicit tags in source template '" << tag << std::endl;
            continue;
        }

        output_template.tags[tag] = output_template.source_parts.size();
        output_template.source_parts.push_back(
            source.substr(current_start, tag_start - current_start)
        );

        current_start = tag_end + 2;
    }

    if(current_start != source.size())
        output_template.source_parts.push_back(
            source.substr(current_start, source.size() - current_start)
        );
    

    return output_template;
}

SourceTemplate SourceTemplate::fromFile(std::string path){
    std::ifstream file(path);  // Open the file
    if (!file.is_open()) {              // Check if the file is open
        std::cerr << "Failed to open template file: " << path << std::endl;
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();             

    std::string source = buffer.str();

    file.close();  // Close the file

    return SourceTemplate::fromSource(source);
}

std::string SourceTemplate::fill(std::vector<TagValue> values){
    if(values.size() != tags.size()){
        std::cerr << "Invalid number of input tag values. Expected: " << tags.size() << std::endl;
        return "";
    }
    std::vector<std::string> filled_tags(tags.size(), " ");

    for(auto& [tag_name, value]: values){
        if(!tags.contains(tag_name)){
            std::cerr << "Tag name '" << tag_name << "' not found in template." << std::endl;
            return "";
        }

        filled_tags[tags[tag_name]] = value;
    }

    size_t total_size = 0;
    for(auto& value: filled_tags) total_size += value.size();
    for(auto& part: source_parts) total_size += part.size();

    bool next_is_part = first_is_part;

    size_t parts_index = 0;
    size_t tags_index =  0;

    std::string output(total_size, ' ');

    size_t offset = 0;
    while(offset < total_size){
        std::string* piece;

        if(next_is_part) piece = &source_parts[parts_index++];
        else piece = &filled_tags[tags_index++];
        next_is_part = !next_is_part;

        size_t length = piece->size();
        std::memcpy(output.data() + offset, piece->data(), length);
        offset += length;
    }

    return output;
}