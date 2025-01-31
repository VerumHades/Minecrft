#pragma once

#include <tinyxml2.h>
#include <regex>
#include <filesystem>
#include <ui/tvalue.hpp>
#include <ui/color.hpp>
#include <typeinfo>
#include <iostream>

class UIFrame;
class UICore;
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

        std::unordered_map<std::string, std::function<void(std::shared_ptr<UIFrame>, std::string, UIElementState)>> attributeApplyFunctions;

        std::vector<UIStyleQueryAttribute> parseQueryAttributes(std::string source);
        UIStyleSelector parseQuerySelector(std::string source);
        void parseQuery(std::string selectors, std::string source);

        friend struct UIStyleQuery;

    public:
        UIStyle();
        // Load style from file, new styles are addded to the registry nothing is erased
        void loadFromFile(const std::string& path);
        void applyTo(std::shared_ptr<UIFrame> element);
        void applyToAndAllChildren(std::shared_ptr<UIFrame> element);
};

#define XML_ELEMENT_LAMBDA_LOAD(class_) \
    [](tinyxml2::XMLElement*){return std::make_shared<class_>();}

class UILoader{
    public:
        using XMLElementCreationFunction = std::function<std::shared_ptr<UIFrame>(tinyxml2::XMLElement* source)>;

    private:
        std::unordered_map<std::string, XMLElementCreationFunction> elements{};
        std::unordered_map<std::string, std::shared_ptr<UIFrame>> elements_with_ids{};

        UIStyle style;
        std::shared_ptr<UIFrame> createElement(tinyxml2::XMLElement* source);
        std::shared_ptr<UIFrame> processElement(tinyxml2::XMLElement* source);

    public:
        void registerElement(const std::string& name, XMLElementCreationFunction creation_function);
        /*
            Loads a window its layers and elements from an xml source file.
        */
        bool loadWindowFromXML(UIWindow& window, const std::string& path);
        UIStyle& getCurrentStyle() {return style;};

        /*
            Parses xml source to create an element
        */
        template <typename T>
        std::shared_ptr<T> createElement(const std::string& source){
            tinyxml2::XMLDocument doc;

            if (doc.Parse(source.c_str()) != tinyxml2::XML_SUCCESS) {
                std::cerr << "Parsing of source for element failed: " << source << std::endl;
                return nullptr;
            }

            auto element = processElement(doc.FirstChildElement());

            if(auto result = std::dynamic_pointer_cast<T>(element))
                return result;
            else{
                std::cerr  << "Resulting element type does not match the one required." << std::endl;
                return nullptr;
            }
        }

        template <typename T>
        std::shared_ptr<T> getElementById(const std::string& id){
            if(!elements_with_ids.contains(id)){
                std::cerr << "No element under id '" << id << "' found."<< std::endl;
                return nullptr;
            }
            
            if(auto result = std::dynamic_pointer_cast<T>(elements_with_ids[id]))
                return result;
            else{
                std::cerr  << "Resulting element type does not match the one required:" << typeid(T).name() << std::endl;
                return nullptr;
            }
        }

        void cleanup();
};

TValue parseTValue(std::string source);

#include <ui/core.hpp>