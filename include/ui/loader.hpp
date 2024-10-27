#pragma once

#include <ui/manager.hpp>
#include <tinyxml2.h>
#include <regex>
#include <filesystem>

struct UIStyleQueryAttribute{
    std::string name;
    std::string value;
};

struct UIStyleQuery{
    enum {
        ID,
        CLASS,
        TAG
    } type;
    std::string value;
    std::vector<UIStyleQueryAttribute> attributes;
    
    void applyTo(std::shared_ptr<UIFrame> element);
};

class UIStyle{
    private:
        std::unordered_map<std::string, UIStyleQuery> tag_queries;
        std::unordered_map<std::string, UIStyleQuery> class_queries;
        std::unordered_map<std::string, UIStyleQuery> id_queries;

        void parseQuery(std::string type, std::string value, std::string source);

    public:
        UIStyle(std::string path);

        void applyTo(
            std::shared_ptr<UIFrame> element,
            std::string tag,
            std::string id,
            const std::vector<std::string>& classes
        );
};

class UILoader{
    private:
        std::unique_ptr<UIStyle> currentStyle;
        std::shared_ptr<UIFrame> createElement(tinyxml2::XMLElement* source, UILayer& layer);
        std::shared_ptr<UIFrame> processElement(tinyxml2::XMLElement* source, UILayer& layer);

    public:
        UILoader(){}
        /*
            Loads a window its layers and elements from an xml source file.
        */
        bool loadWindowFromXML(UIWindow& window, std::string path);
};

TValue parseTValue(std::string source);

