#include <ui/loader.hpp>

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

    std::vector<std::string> filtered_tokens{};
    for(auto& token: tokens){
        if(token == "") continue;
        filtered_tokens.push_back(token); 
    }

    return filtered_tokens;
}

static inline void remove_spaces(std::string& source){
    source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());
}

UIColor parseColor(std::string source) {
    remove_spaces(source);

    // Regex for RGB and RGBA formats
    std::regex RGBpattern(R"(rgb\((\d+),(\d+),(\d+)\))");
    std::regex RGBApattern(R"(rgba\((\d+),(\d+),(\d+),(\d+)\))");

    // Regex for hex colors
    std::regex hexPattern(R"((#?)(?:([0-9a-fA-F]{3})|([0-9a-fA-F]{6})|([0-9a-fA-F]{8})))");

    std::smatch matches;

    if (std::regex_match(source, matches, RGBpattern)) {
        int r = std::stoi(matches[1].str());
        int g = std::stoi(matches[2].str());
        int b = std::stoi(matches[3].str());
        return {r, g, b};
    }
    else if (std::regex_match(source, matches, RGBApattern)) {
        int r = std::stoi(matches[1].str());
        int g = std::stoi(matches[2].str());
        int b = std::stoi(matches[3].str());
        int a = std::stoi(matches[4].str());
        return {r, g, b, a};
    }
    else if (std::regex_match(source, matches, hexPattern)) {
        // Remove the leading '#' if it's there
        std::string hexCode = matches[0].str();
        if (hexCode[0] == '#') {
            hexCode = hexCode.substr(1);
        }

        int r = 0, g = 0, b = 0, a = 255;

        if (hexCode.length() == 3) {
            // Short form: #RGB
            r = std::stoi(hexCode.substr(0, 1) + std::string(1, hexCode[0]), nullptr, 16);
            g = std::stoi(hexCode.substr(1, 1) + std::string(1, hexCode[1]), nullptr, 16);
            b = std::stoi(hexCode.substr(2, 1) + std::string(1, hexCode[2]), nullptr, 16);
        } else if (hexCode.length() == 6) {
            // Full form: #RRGGBB
            r = std::stoi(hexCode.substr(0, 2), nullptr, 16);
            g = std::stoi(hexCode.substr(2, 2), nullptr, 16);
            b = std::stoi(hexCode.substr(4, 2), nullptr, 16);
        } else if (hexCode.length() == 8) {
            // With alpha: #RRGGBBAA
            r = std::stoi(hexCode.substr(0, 2), nullptr, 16);
            g = std::stoi(hexCode.substr(2, 2), nullptr, 16);
            b = std::stoi(hexCode.substr(4, 2), nullptr, 16);
            a = std::stoi(hexCode.substr(6, 2), nullptr, 16);
        }

        return {r, g, b, a};
    }

    std::cerr << "Invalid color string: " << source << std::endl;
    return {255, 0, 0};  
}

#define ATTRIBUTE_LAMBDA(attr, func) \
    [](std::shared_ptr<UIFrame> element, std::string value, UIElementState state) { \
        element->setAttribute(&UIFrame::Style::attr, func(value), state); \
    }

#define BORDER_LAMBDA(attr, func, index) \
    [](std::shared_ptr<UIFrame> element, std::string value, UIElementState state) { \
        auto parsed = func(value); \
        auto attrVec = element->getAttribute(&UIFrame::Style::attr); \
        if (index == -1) { \
            attrVec = {parsed, parsed, parsed, parsed}; \
        } else { \
            attrVec[index] = parsed; \
        } \
        element->setAttribute(&UIFrame::Style::attr, attrVec, state); \
    }

UISideSizesT parseSideSizes(std::string source){
    auto split_source = split(source, " ");

    UISideSizesT output{0_px,0_px,0_px,0_px};

    if(
        split_source.size() != 4 &&
        split_source.size() != 2 &&
        split_source.size() != 1
    ){  
        std::cerr << "Invalid arguments for size parsing: " << source << std::endl;
        return output;
    }

    if(split_source.size() == 4)
        output = {
            parseTValue(split_source[0]),
            parseTValue(split_source[1]),
            parseTValue(split_source[2]),
            parseTValue(split_source[3])
        };
    else if(split_source.size() == 2)
        output = {
            parseTValue(split_source[0]),
            parseTValue(split_source[1]),
            parseTValue(split_source[0]),
            parseTValue(split_source[1])
        };
    else if(split_source.size() == 1)
        output = {
            parseTValue(split_source[0]),
            parseTValue(split_source[0]),
            parseTValue(split_source[0]),
            parseTValue(split_source[0])
        };
    
    return output;
}

UIStyle::UIStyle(){
    attributeApplyFunctions = {
        {"background-color", ATTRIBUTE_LAMBDA(backgroundColor, parseColor)},
        {"color",            ATTRIBUTE_LAMBDA(textColor, parseColor)},
        {"margin",           ATTRIBUTE_LAMBDA(margin, parseSideSizes)},
        {"padding",          ATTRIBUTE_LAMBDA(padding, parseSideSizes)},
        {"font-size",        ATTRIBUTE_LAMBDA(fontSize, parseTValue)},
        {
            "text-align", [](auto element, auto value, auto){
                remove_spaces(value);

                if     (value == "center") element->setAttribute(&UIFrame::Style::textPosition, UIFrame::Style::CENTER);
                else if(value == "left"  ) element->setAttribute(&UIFrame::Style::textPosition, UIFrame::Style::LEFT  );
                else if(value == "right" ) element->setAttribute(&UIFrame::Style::textPosition, UIFrame::Style::RIGHT );
                else std::cerr << "Invalid text-align: " << value << std::endl;
            }
        },
        {
            "display", [](auto element, auto value, auto){
                remove_spaces(value);

                if(value == "flex") element->setLayout(std::make_shared<UIFlexLayout>());
                else element->setLayout(std::make_shared<UILayout>());
            }
        },
        {
            "flex-direction", [](auto element, auto value, auto){
                remove_spaces(value);

                if(auto flex_frame = std::dynamic_pointer_cast<UIFlexLayout>(element->getLayout())){
                    if     (value == "column") flex_frame->setDirection(UIFlexLayout::VERTICAL);
                    else if(value == "row")    flex_frame->setDirection(UIFlexLayout::HORIZONTAL);
                    else std::cerr << value << " is not a valid flex direction. Use 'column' or 'row'." << std::endl;
                }
                else std::cerr << "Settings flex properties for an element that doesnt use a flex layout?" << std::endl;
            }
        },

        {"left",   [](auto element, auto value, auto) { element->setX(parseTValue(value)); }},
        {"top",    [](auto element, auto value, auto) { element->setY(parseTValue(value)); }},
        {"right",  [](auto element, auto value, auto) { element->setX((TValue{PERCENT,100} - TValue{MY_PERCENT,100}) - parseTValue(value)); }},
        {"bottom", [](auto element, auto value, auto) { element->setY((TValue{PERCENT,100} - TValue{MY_PERCENT,100}) - parseTValue(value)); }},
        {"width",  [](auto element, auto value, auto) { element->setWidth(parseTValue(value)); }},
        {"height", [](auto element, auto value, auto) { element->setHeight(parseTValue(value)); }},
        {"translate", [](auto element, auto value, auto) { 
            auto split_source = split(value, " ");

            if(split_source.size() != 2){
                std::cerr << "Invalid values for 'translate'." << std::endl;
                return;
            }

            auto value1 = parseTValue(split_source[0]);
            auto value2 = parseTValue(split_source[1]);

            if(value1.unit == PERCENT) value1.unit = MY_PERCENT;
            if(value2.unit == PERCENT) value2.unit = MY_PERCENT;

            //std::cout << "'" << split_source[0]  << "' " << value1.unit << " " << value1.value << std::endl;
            //std::cout << "'" << split_source[1]  << "' " << value2.unit << " " << value2.value << std::endl;

            element->setAttribute(&UIFrame::Style::translation, {value1, value2});
        }},
        
        {"border-width",        ATTRIBUTE_LAMBDA(borderWidth, parseSideSizes)},
        {"border-color",        BORDER_LAMBDA(borderColor, parseColor, -1)},
    };
}

std::vector<UIStyle::UIStyleQueryAttribute> UIStyle::parseQueryAttributes(std::string source){
    std::vector<UIStyleQueryAttribute> attributes;
    //source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());

    std::regex pattern(R"(([a-zA-Z\-]+):([a-zA-Z0-9,()\- %#]+);)"); 

    auto matches_begin = std::sregex_iterator(source.begin(), source.end(), pattern);
    auto matches_end = std::sregex_iterator();

    for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
        std::smatch match = *i;
        
        if(attributeApplyFunctions.count(match[1]) == 0) {
            std::cerr << "Unsuported attribute in css file: " << match[1] << std::endl;
            continue;
        }

        attributes.push_back({match[1],match[2]});
    }

    return attributes;
}
UIStyle::UIStyleSelector UIStyle::parseQuerySelector(std::string source){
    source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());

    if(source == "*") return {UIStyleSelector::ANY, UIElementState::BASE};

    std::regex pattern("((\\.|#|)([a-zA-Z0-9_]+)(:([a-zA-Z]+))?)");

    std::string state, type, value;

    std::smatch matches;
    if (std::regex_match(source, matches, pattern)) {
        type  = matches[2];
        value = matches[3];
        state = matches[5]; 
    } else {
        std::cerr << "Invalid selector: " << source << std::endl; 
        return {};
    }

    UIStyleSelector query = {};

    if(state == "hover") query.state = UIElementState::HOVER;
    else if(state == "focus") query.state = UIElementState::FOCUS;
    else query.state = UIElementState::BASE;

    if     (type == "#") query.type = UIStyleSelector::ID;
    else if(type == ".") query.type = UIStyleSelector::CLASS;
    else                 query.type = UIStyleSelector::TAG;
    
    query.value = value;

    return query;
}

void UIStyle::parseQuery(std::string selectors, std::string source){
    auto attributes = parseQueryAttributes(source);
    int attribute_index = attribute_registry.size();
    attribute_registry.push_back(attributes);

    for(auto selector_whole_string: split(selectors, ",")){
        std::vector<UIStyleSelector> selectors = {};
        for(auto selector_string: split(selector_whole_string, " ")){
            auto selector = parseQuerySelector(selector_string);
            
            if(selector.type == UIStyleSelector::NONE){
                std::cerr << "Invalid selector: '" << selector_string << "'." << std::endl; 
                continue;
            }
            selectors.insert(selectors.begin(), selector);
        }

        queries.push_back({
            selectors,
            attribute_index
        });
    }
}

void UIStyle::loadFromFile(const std::string& path){
    std::ifstream f(path, std::ios::in | std::ios::binary);

    const auto sz = std::filesystem::file_size(path);
    std::string source(sz, '\0');
    f.read(source.data(), sz);   
    // Regex for individual queries
    std::regex pattern(R"(([^{]+)?\{([\s\S]*?)\})"); 
    
    auto matches_begin = std::sregex_iterator(source.begin(), source.end(), pattern);
    auto matches_end = std::sregex_iterator();

    for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
        std::smatch match = *i;
        
        parseQuery(match[1],match[2]);
    }
}

bool UIStyle::UIStyleSelector::isSelectorMatch(UIFrame* element){
    switch (type)
    {
        case ANY: return true;
        case ID:  return element->getIdentifiers().id  == value;
        case TAG: return element->getIdentifiers().tag == value;
        case CLASS: 
            for(auto element_class: element->getIdentifiers().classes){
                if(element_class == value) return true;
            }
            return false;
        default:
            return false;
    }
}

void UIStyle::applyToAndAllChildren(std::shared_ptr<UIFrame> element){
    applyTo(element);
    for(auto& child: element->getChildren()){
        applyToAndAllChildren(child);
    }
}

void UIStyle::applyTo(
    std::shared_ptr<UIFrame> element
){
    for(auto query: queries){
        auto& selectors = query.selector; // Selectors of the element, and parents
        //std::cout << "Solving query." << std::endl;

        bool failed = false;
        UIFrame* current_element = element.get();
        for(auto& selector: selectors){
            if(
                !current_element ||
                !selector.isSelectorMatch(current_element)
            ){
                failed = true;
                break;
            }

            current_element = current_element->parent;
        }

        if(failed) continue;

        for(auto& attr: attribute_registry[query.id]){
            attributeApplyFunctions[attr.name](element, attr.value, query.selector[0].state);
        }   
    }

    element->calculateTransforms();
}