#pragma once

#include <ui/manager.hpp>
#include <tinyxml2.h>
#include <regex>
#include <filesystem>

std::vector<std::string> split(std::string s, const std::string& delimiter);

class UIStyle{
    private:
        struct UIStyleQueryAttribute{
            std::string name;
            std::string value;
        };

        struct UIStyleSelector{
            enum {
                NONE,
                ID,
                CLASS,
                TAG
            } type;
            UIFrame::State state;
            std::string value;
        };

        struct UIStyleQuery{
            UIStyleSelector selector;
            int registry_index; // Index of attributes in the attributes registry
            
            void applyTo(std::shared_ptr<UIFrame> element, UIStyle* style);
        };

        std::vector<std::vector<UIStyleQueryAttribute>> attribute_registry;
        using QueryMap = std::unordered_map<std::string, std::vector<UIStyleQuery>>;

        QueryMap tag_queries;
        QueryMap class_queries;
        QueryMap id_queries;

        void addQuery(std::string name, QueryMap& map, UIStyleQuery& query){
            if(map.count(name) == 0) map[name] = {};
            map[name].push_back(query);
        }

        std::vector<UIStyleQueryAttribute> parseQueryAttributes(std::string source);
        UIStyleSelector parseQuerySelector(std::string source);
        void parseQuery(std::string selectors, std::string source);

        friend struct UIStyleQuery;

    public:
        UIStyle(){};
        // Load style from file, new styles are addded to the registry nothing is erased
        void loadFromFile(std::string path);
        void applyTo(std::shared_ptr<UIFrame> element);
};

class UILoader{
    private:
        UIStyle style;
        std::shared_ptr<UIFrame> createElement(tinyxml2::XMLElement* source, UILayer& layer);
        std::shared_ptr<UIFrame> processElement(tinyxml2::XMLElement* source, UILayer& layer);

        UIManager& manager;
    public:
        UILoader(UIManager& manager): manager(manager){}
        /*
            Loads a window its layers and elements from an xml source file.
        */
        bool loadWindowFromXML(UIWindow& window, std::string path);
        UIStyle& getCurrentStyle() {return style;};
};

TValue parseTValue(std::string source);

