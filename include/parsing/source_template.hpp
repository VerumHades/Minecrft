#pragma once 

#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>

class SourceTemplate{
    public:
        struct TagValue{
            std::string tag_name;
            std::string value;
        };

    private:
        std::unordered_map<std::string, size_t> tags{};
        std::vector<std::string> source_parts{};
        bool first_is_part = true; // If first is source part, if false first pick from tags

        SourceTemplate() {}

    public:
        static SourceTemplate fromSource(std::string source);
        static SourceTemplate fromFile(const std::string& path);

        std::string fill(std::vector<TagValue> values);
};
