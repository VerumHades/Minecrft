#include <ui/loader.hpp>

static const std::vector<std::tuple<std::string, int, Units>> operators = {
    {"-", 0, OPERATION_MINUS},
    {"+", 0, OPERATION_PLUS }
};

/*
    Return a tuple of [operator position in the string, operator index in registry]
*/
std::tuple<size_t, size_t> findMostImportantOperator(std::string source){    
    size_t selectedOperatorPosition = std::string::npos;
    size_t selectedOperatorIndex = std::string::npos;

    size_t offset = 0;
    while(offset < source.size()){
        size_t closestDistance = std::string::npos;
        size_t closestOperatorIndex = std::string::npos;

    
        for(int i = 0; i < operators.size();i++){
            auto& [name, value, unit] = operators[i];

            size_t distance = source.find(name);
            if(distance >= closestDistance) continue;
            
            closestDistance = distance;
            closestOperatorIndex = i;
        }

        if(closestOperatorIndex == std::string::npos) 
            return {selectedOperatorPosition, selectedOperatorIndex};

        if(
            selectedOperatorIndex == std::string::npos || 
            (
                std::get<1>(operators[closestOperatorIndex]) > 
                std::get<1>(operators[selectedOperatorIndex])
            )
        ){
            selectedOperatorIndex = closestOperatorIndex;
            selectedOperatorPosition = offset + closestDistance;
        }
        
        offset += closestDistance;
    }

    return {selectedOperatorPosition, selectedOperatorIndex};
}

TValue parseTValue(std::string source){
    source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());
    
    if(source == "fit-content") return TValue(Units::FIT_CONTENT, 0);
    else if(source == "auto") return TValue(Units::AUTO, 0);

    auto [op_position,op_index] = findMostImportantOperator(source);

    if(op_position != std::string::npos){
        Units unit = std::get<2>(operators[op_index]);
        std::string name = std::get<0>(operators[op_index]);
        return TValue(
            unit,
            parseTValue(source.substr(0, op_position).c_str()),
            parseTValue(source.substr(op_position + name.length()).c_str())
        );
    }

    std::regex pattern(R"((\d+)(%|px|m))");
    std::smatch matches;

    if (std::regex_match(source, matches, pattern)) {
        int value = std::stoi(matches[1].str());  
        std::string unitType = matches[2].str();     

        Units unit;
        if (unitType == "%") {
            unit = Units::PERCENT;
        } else if (unitType == "px") {
            unit = Units::PIXELS;
        } else if (unitType == "m") {
            unit = Units::MY_PERCENT;
        } else {
            unit = Units::PIXELS;
        }

        return TValue(unit, value);  
    }
    std::cerr << "Failed to parse value: " << source << std::endl;
    return TNONE;
}

using namespace tinyxml2;

static inline TValue getAttributeValue(XMLElement* source, std::string name, TValue def = TNONE){
    const char* attr = source->Attribute(name.c_str());
    if(!attr) return def;
    return parseTValue(attr);
}

static inline std::string optGetAttribute(XMLElement* source, std::string name, std::string def = ""){
    const char* attr = source->Attribute(name.c_str());
    if(!attr) return def;
    return attr;
}

std::shared_ptr<UIFrame> UILoader::createElement(XMLElement* source) {
    auto it = elements.find(source->Name());
    if (it != elements.end()) {
        auto el = it->second(source);
        if(!el) return nullptr;

        el->setPosition(
            getAttributeValue(source,"x"),
            getAttributeValue(source,"y")
        );
        
        el->setSize(
            getAttributeValue(source,"width"),
            getAttributeValue(source,"height")
        );

        auto split_classes_array = split(optGetAttribute(source,"class"), " ");
        el->identifiers.classes.insert(split_classes_array.begin(), split_classes_array.end());
        el->identifiers.id = optGetAttribute(source,"id");
        el->identifiers.tag = source->Name();

        auto id = source->Attribute("id");
        if(id) elements_with_ids[id] = el;
        

        return el; 
    }
    std::cerr << "No tag '" << source->Name() << "' found!" << std::endl;
    return nullptr; 
}

void UILoader::registerElement(const std::string& name, XMLElementCreationFunction creation_function){
    elements[name] = creation_function;
}

std::shared_ptr<UIFrame> UILoader::processElement(XMLElement* source){
    if(source->Name() && std::string(source->Name()) == "style"){
        auto path = source->Attribute("src");
        if(!path){
            std::cerr << "Style missing source path." << std::endl;
            return nullptr;
        }

        style.loadFromFile(path);
        
        return nullptr;
    }

    std::shared_ptr<UIFrame> element = createElement(source);
    
    for (
        XMLElement* child = source->FirstChildElement(); 
        child != nullptr; 
        child = child->NextSiblingElement()
    ) {
        auto proccessed = processElement(child);
        if(!proccessed) continue;
        element->appendChild(proccessed);
    }

    return element;
}

bool UILoader::loadWindowFromXML(UIWindow& window, const std::string& path){
    XMLDocument doc;

    if (doc.LoadFile(path.c_str()) != XML_SUCCESS) {
        std::cerr << "Error loading ui XML file." << std::endl;
        return false;
    }

    XMLElement* root = doc.FirstChildElement("window");
    if (!root) {
        std::cerr << "No root window element found." << std::endl;
        return false;
    }

    for (
        XMLElement* layer = root->FirstChildElement(); 
        layer != nullptr; 
        layer = layer->NextSiblingElement()
    ) {
        std::string name = layer->Name();
        //window.setCurrentLayer(name);

        for (
            XMLElement* layer_child = layer->FirstChildElement(); 
            layer_child != nullptr; 
            layer_child = layer_child->NextSiblingElement()
        ) {
            //std::cout << layer_child->Name() << std::endl;
            auto processed = processElement(layer_child);
            if(!processed) continue;
            window.getLayer(name).addElement(processed);

            style.applyToAndAllChildren(processed);
        }
    }

    return true;
}