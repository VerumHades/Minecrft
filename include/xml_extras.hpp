#pragma once

#include <tinyxml2.h>
#include <initializer_list>
#include <tuple>
#include <string>
#include <iostream>
#include <functional>

#include <logging.hpp>

using namespace tinyxml2;

#define xml_for_each_child_as(of, child_name) for ( \
    tinyxml2::XMLElement* child_name = of->FirstChildElement(); \
    child_name != nullptr; \
    child_name = child_name->NextSiblingElement()\
)

namespace XMLExtras{
    struct AttributeDefinition{
        std::string name;
        size_t offset;
        enum Type {
                FLOAT,
                STRING,
                INT,
                BOOL
        } type;
    };

    #define XML_ATTR_TYPE_CASE(type, query_name) \
    if(element->query_name((type*)((unsigned char*)&instance + attribute.offset)) != tinyxml2::XML_SUCCESS) LogError("Element '{}' failed to parse.", attribute.name);

    template <typename T>
    bool ParseAttribute(tinyxml2::XMLElement* element, const AttributeDefinition& attribute, T& instance){
        if(element->Name() != attribute.name) return false;
        switch (attribute.type)
        {
        case AttributeDefinition::FLOAT: XML_ATTR_TYPE_CASE(float, QueryFloatText) return true;
        case AttributeDefinition::INT: XML_ATTR_TYPE_CASE(int, QueryIntText) return true;
        case AttributeDefinition::BOOL: XML_ATTR_TYPE_CASE(bool, QueryBoolText) return true;
        case AttributeDefinition::STRING: {
            const char* text = element->GetText();
            if(!text) return false;
            *(std::string*)((unsigned char*)&instance + attribute.offset) = std::string(text);
        } return true; 
        default: return false;
        }
    }

    #undef XML_ATTR_TYPE_CASE

    template <typename T>
    void Load(
        T& instance, 
        tinyxml2::XMLElement* element, 
        std::initializer_list<AttributeDefinition> attribute_definitions, 
        const std::function<void(XMLElement*, T& insance)>& unparsed = nullptr
    ) {

        xml_for_each_child_as(element, attribute){
            bool parsed = false;

            for(auto& attribute_definition: attribute_definitions){
                if(ParseAttribute<T>(attribute, attribute_definition, instance)){
                    parsed = true;
                    break;
                }
            }

            if(!parsed && unparsed) unparsed(attribute, instance);
        }
    }

    template <typename T>
    T Load(
        tinyxml2::XMLElement* element, 
        std::initializer_list<AttributeDefinition> attribute_definitions, 
        const std::function<void(XMLElement*, T& insance)>& unparsed = nullptr
    ){
        T instance{};
        Load<T>(instance,element,attribute_definitions,unparsed);
        return instance;
    }

}

using XType = XMLExtras::AttributeDefinition::Type;