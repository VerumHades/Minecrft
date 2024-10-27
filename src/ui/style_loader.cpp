#include <ui/loader.hpp>

UIColor parseColor(std::string source){
    //source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());

    std::regex RGBpattern(R"(rgb\((\d+),(\d+),(\d+)\))");
    std::regex RGBApattern(R"(rgba\((\d+),(\d+),(\d+),(\d+)\))");
    std::smatch matches;

    if (std::regex_match(source, matches, RGBpattern)) {
        int r = std::stoi(matches[1].str());      
        int g = std::stoi(matches[2].str());   
        int b = std::stoi(matches[3].str());      

        return {r,g,b};  
    }
    else if (std::regex_match(source, matches, RGBApattern)) {
        int r = std::stoi(matches[1].str());      
        int g = std::stoi(matches[2].str());   
        int b = std::stoi(matches[3].str());         
        int a = std::stoi(matches[4].str());  

        return {r,g,b,a};  
    }

    std::cerr << "Invalid color string: " << source << std::endl;
    return {255,0,0};
}



static std::unordered_map<std::string, std::function<void(std::shared_ptr<UIFrame>, std::string, UIFrame::State)>> attributeApplyFunctions = {
    {
        "background-color",
        [](std::shared_ptr<UIFrame> element, std::string value, UIFrame::State state){
            element->setAttribute(&UIFrame::Style::backgroundColor, parseColor(value), state);
        }
    },
    {
        "color",
        [](std::shared_ptr<UIFrame> element, std::string value, UIFrame::State state){
            element->setAttribute(&UIFrame::Style::textColor, parseColor(value), state);
        }
    },
    {"border-width",[](std::shared_ptr<UIFrame> element, std::string value, UIFrame::State state){
        auto v = parseTValue(value);
        element->setAttribute(&UIFrame::Style::borderWidth, {v,v,v,v}, state);
    }},
    {"border-color",[](std::shared_ptr<UIFrame> element, std::string value, UIFrame::State state){
        auto c = parseColor(value);
        element->setAttribute(&UIFrame::Style::borderColor, {c,c,c,c}, state);
    }},
    {"margin",[](std::shared_ptr<UIFrame> element, std::string value, UIFrame::State state){
        element->setAttribute(&UIFrame::Style::margin, parseTValue(value), state);
    }}
};

void UIStyle::parseQuery(std::string type, std::string value, std::string state, std::string source){
    UIStyleQuery query = {};
    query.value = value;

    // Match for individual attributes
    std::regex pattern(R"(([a-zA-Z\-]+):([a-zA-Z0-9,()]+);)"); 

    auto matches_begin = std::sregex_iterator(source.begin(), source.end(), pattern);
    auto matches_end = std::sregex_iterator();

    for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
        std::smatch match = *i;
        
        if(attributeApplyFunctions.count(match[1]) == 0) {
            std::cerr << "Unsuported attribute in css file: " << match[1] << std::endl;
            continue;
        }
        query.attributes.push_back({match[1],match[2]});
    }

    if(state == ":hover") query.state = UIFrame::HOVER;
    else query.state = UIFrame::BASE;

    if(type == "#"){
        query.type = UIStyleQuery::ID;
        addQuery(value, id_queries, query);
    }
    else if(type == "."){
        query.type = UIStyleQuery::CLASS;
        addQuery(value, class_queries, query);
    }
    else{
        query.type = UIStyleQuery::TAG;
        addQuery(value, tag_queries, query);
    }
}

UIStyle::UIStyle(std::string path){
    std::ifstream f(path, std::ios::in | std::ios::binary);

    const auto sz = std::filesystem::file_size(path);
    std::string source(sz, '\0');
    f.read(source.data(), sz);   

    // Regex for individual queries
    std::regex pattern(R"((#|\.|)([a-zA-Z_]+)(:hover|)?\{([\s\S]*?)\})"); 
    // Remove all spaces
    source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());

    auto matches_begin = std::sregex_iterator(source.begin(), source.end(), pattern);
    auto matches_end = std::sregex_iterator();

    for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
        std::smatch match = *i;
        
        parseQuery(match[1],match[2],match[3],match[4]);
    }
}

void UIStyle::applyTo(
    std::shared_ptr<UIFrame> element,
    std::string tag,
    std::string id,
    const std::vector<std::string>& classes
){
    if(tag_queries.count(tag) != 0)
        for(auto& q: tag_queries[tag]) q.applyTo(element);

    for(auto& classname: classes)
        if(class_queries.count(classname) != 0)
            for(auto& q: class_queries[classname]) q.applyTo(element);

    if(id_queries.count(id) != 0)
        for(auto& q: id_queries[id]) q.applyTo(element);
}

void UIStyleQuery::applyTo(std::shared_ptr<UIFrame> element){
    for(auto& attr: attributes){
        attributeApplyFunctions[attr.name](element, attr.value, state);
    }    
}