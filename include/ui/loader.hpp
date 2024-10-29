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
    UIFrame::State state;
    std::string value;
    std::vector<UIStyleQueryAttribute> attributes;
    
    void applyTo(std::shared_ptr<UIFrame> element);
};

class UIStyle{
    private:
        using QueryMap = std::unordered_map<std::string, std::vector<UIStyleQuery>>;

        QueryMap tag_queries;
        QueryMap class_queries;
        QueryMap id_queries;

        void addQuery(std::string name, QueryMap& map, UIStyleQuery& query){
            if(map.count(name) == 0) map[name] = {};
            map[name].push_back(query);
        }

        void parseQuery(std::string type, std::string value, std::string state, std::string source);

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

        UIManager& manager;
    public:
        UILoader(UIManager& manager): manager(manager){}
        /*
            Loads a window its layers and elements from an xml source file.
        */
        bool loadWindowFromXML(UIWindow& window, std::string path);
        std::unique_ptr<UIStyle>& getCurrentStyle(){return currentStyle;};
};

TValue parseTValue(std::string source);

