#pragma once

#include <tinyxml2.h>
#include <regex>
#include <filesystem>
#include <ui/tvalue.hpp>
#include <ui/color.hpp>

class UIFrame;
class UIManager;
class UIWindow;
class UILayer;

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
                ANY,
                ID,
                CLASS,
                TAG
            } type;
            UIElementState state;
            std::string value;

            bool isSelectorMatch(UIFrame* element);
            std::string to_string(){
                std::string prefix = "";
                switch(type){
                    case ID: prefix = "#"; break;
                    case CLASS: prefix = "."; break; 
                    case ANY: prefix = "*"; break;
                    default: break;
                }
                return prefix + value; 
            }
        };

        struct UIStyleQuery{
            std::vector<UIStyleSelector> selector;
            int id; // Index of attributes in the attributes registry
        };

        /*
            Queries defined for multiple selectors at once with same attributes, so that meaningless duplicates arent stored
        */
        std::vector<std::vector<UIStyleQueryAttribute>> attribute_registry;
        /*
            
        */
        std::vector<UIStyleQuery> queries;

        std::vector<UIStyleQueryAttribute> parseQueryAttributes(std::string source);
        UIStyleSelector parseQuerySelector(std::string source);
        void parseQuery(std::string selectors, std::string source);

        friend struct UIStyleQuery;

    public:
        UIStyle(){};
        // Load style from file, new styles are addded to the registry nothing is erased
        void loadFromFile(const std::string& path);
        void applyTo(std::shared_ptr<UIFrame> element);
        void applyToAndAllChildren(std::shared_ptr<UIFrame> element);
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
        bool loadWindowFromXML(UIWindow& window, const std::string& path);
        UIStyle& getCurrentStyle() {return style;};
};

TValue parseTValue(std::string source);

#include <ui/manager.hpp>