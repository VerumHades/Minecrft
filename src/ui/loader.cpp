#include <ui/loader.hpp>

static const std::vector<std::tuple<std::string, int, Units>> operators = {
    {"-", 0, OPERATION_MINUS},
    {"+", 0, OPERATION_PLUS }
};

std::string strip(const std::string& input) {
    const std::string whitespace = " \t\n\r\f\v";
    
    size_t start = input.find_first_not_of(whitespace);
    if (start == std::string::npos) {return "";}
    size_t end = input.find_last_not_of(whitespace);

    return input.substr(start, end - start + 1);
}

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
    source = strip(std::string(source));
    
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
            unit = Units::PFRACTION;
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
    return {0};
}


using namespace tinyxml2;

static inline TValue getAttributeValue(XMLElement* source, std::string name, TValue def = {PIXELS,0}){
    const char* attr = source->Attribute(name.c_str());
    if(!attr) return def;
    return parseTValue(attr);
}

std::shared_ptr<UIFrame> createElement(XMLElement* source) {
    // Map each tag name to a specific constructor
    static const std::unordered_map<std::string, std::function<std::shared_ptr<UIFrame>()>> elements = {
        {   
            "frame", 
            [source]() {
                return std::make_shared<UIFrame>(
                    getAttributeValue(source,"x"),
                    getAttributeValue(source,"y"),
                    getAttributeValue(source,"width"),
                    getAttributeValue(source,"height"),
                    UIColor(150,150,150)
                ); 
            }
        },
        {
            "label", 
            [source]() {
                const char* content = source->GetText();
                std::string text = "";
                if(content) text = std::string(content);

                std::cout << text << std::endl;

                return std::make_shared<UILabel>(
                    text,
                    getAttributeValue(source,"x"),
                    getAttributeValue(source,"y"),
                    getAttributeValue(source,"width"),
                    getAttributeValue(source,"height"),
                    UIColor(150,150,150)
                ); 
            }
        },
    };

    auto it = elements.find(source->Name());
    if (it != elements.end()) {
        return it->second();  // Call the factory function to create the object
    }
    std::cerr << "No tag '" << source->Name() << "' found!" << std::endl;
    return nullptr;  // Return nullptr if no handler is found for the tag name
}

std::shared_ptr<UIFrame> processElement(XMLElement* source){
    std::shared_ptr<UIFrame> element = createElement(source);
    
    for (
        XMLElement* child = source->FirstChildElement(); 
        child != nullptr; 
        child = child->NextSiblingElement()
    ) {
        std::shared_ptr<UIFrame> proccessed = processElement(child);
        if(!proccessed){
            std::cerr << "Failed to proccess element: " << source->Name() << std::endl;
            continue;
        }
        element->appendChild(proccessed);
    }

    return element;
}

bool loadWindowFromXML(UIWindow& window, std::string path){
    XMLDocument doc;

    if (doc.LoadFile(path.c_str()) != XML_SUCCESS) {
        std::cerr << "Error loading ui XML file." << std::endl;
        return false;
    }

    // Get the root element (bookstore)
    XMLElement* root = doc.FirstChildElement("window");
    if (!root) {
        std::cerr << "No root window element found." << std::endl;
        return false;
    }

    // Iterate over all 'book' elements in 'bookstore'
    for (
        XMLElement* layer = root->FirstChildElement(); 
        layer != nullptr; 
        layer = layer->NextSiblingElement()
    ) {
        std::string name = layer->Name();
        window.setCurrentLayer(name);

        for (
            XMLElement* layer_child = layer->FirstChildElement(); 
            layer_child != nullptr; 
            layer_child = layer_child->NextSiblingElement()
        ) {
            auto processed = processElement(layer_child);
            if(!processed){
                std::cerr << "Failed to proccess element: " << layer_child->Name() << std::endl;
                continue;
            }
            window.getLayer(name).addElement(processed);
        }
    }

    return true;
}