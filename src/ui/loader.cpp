#include <ui/loader.hpp>

static const std::vector<std::tuple<std::string, int, Units>> operators = {
    {"-", 0, OPERATION_MINUS},
    {"+", 0, OPERATION_PLUS }
};

std::vector<std::string> split(std::string s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
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
    source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());
    
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

std::shared_ptr<UIFrame> UILoader::createElement(XMLElement* source, UILayer& layer) {
    // Map each tag name to a specific constructor
    std::unordered_map<std::string, std::function<std::shared_ptr<UIFrame>()>> elements = {
        {   
            "frame", 
            [source, this]() {return this->manager.createElement<UIFrame>(); }
        },
        {
            "label", 
            [source, this]() {
                const char* content = source->GetText();
                std::string text = "";
                if(content) text = std::string(content);

                auto label = this->manager.createElement<UILabel>();
                label->setText(text);
                return label; 
            }
        },
        {
            "flex_frame",
            [source, this]() {
                auto frm = this->manager.createElement<UIFlexFrame>(); 

                frm->setElementDirection(source->BoolAttribute("horizontal", true) ? UIFlexFrame::COLUMN : UIFlexFrame::ROWS);
                frm->setExpand(source->BoolAttribute("expand", false));

                return frm;
            }
        },
        {
            "input",
            [source, this]() {
                return this->manager.createElement<UIInput>(); 
            }
        },
        {
            "scrollable",
            [source, this]() {
                return this->manager.createElement<UIScrollableFrame>(); 
            }
        }
    };

    auto it = elements.find(source->Name());
    if (it != elements.end()) {
        auto el = it->second();
        el->setPosition(
            getAttributeValue(source,"x"),
            getAttributeValue(source,"y")
        );
        
        el->setSize(
            getAttributeValue(source,"width"),
            getAttributeValue(source,"height")
        );

        if(currentStyle){
            std::vector<std::string> classes = split(optGetAttribute(source,"class"), " ");
            currentStyle->applyTo(
                el,
                source->Name(),
                optGetAttribute(source,"id"),
                classes
            );
        }

        auto id = source->Attribute("id");
        if(id) layer.addElementWithID(id, el);

        return el; 
    }
    std::cerr << "No tag '" << source->Name() << "' found!" << std::endl;
    return nullptr; 
}

std::shared_ptr<UIFrame> UILoader::processElement(XMLElement* source, UILayer& layer){
    if(source->Name() && std::string(source->Name()) == "style"){
        auto path = source->Attribute("src");
        if(!path){
            std::cerr << "Style missing source path." << std::endl;
            return nullptr;
        }

        currentStyle = std::make_unique<UIStyle>(path);
        
        return nullptr;
    }

    std::shared_ptr<UIFrame> element = createElement(source, layer);
    
    for (
        XMLElement* child = source->FirstChildElement(); 
        child != nullptr; 
        child = child->NextSiblingElement()
    ) {
        auto proccessed = processElement(child, layer);
        if(!proccessed) continue;
        element->appendChild(proccessed);
    }

    return element;
}

bool UILoader::loadWindowFromXML(UIWindow& window, std::string path){
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
            auto processed = processElement(layer_child, window.getLayer(name));
            if(!processed) continue;
            window.getLayer(name).addElement(processed);
        }
    }

    return true;
}