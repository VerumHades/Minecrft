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



static std::unordered_map<std::string, std::function<void(std::shared_ptr<UIFrame>, std::string)>> attributeApplyFunctions = {
    {
        "background-color",
        [](std::shared_ptr<UIFrame> element, std::string value){
            element->setColor(parseColor(value));
        }
    },
    {
        "color",
        [](std::shared_ptr<UIFrame> element, std::string value){
            if(auto label = std::dynamic_pointer_cast<UILabel>(element)){
                label->setTextColor(parseColor(value));
            }
        }
    },
    {"border-width",[](std::shared_ptr<UIFrame> element, std::string value){element->setBorderWidth(parseTValue(value));}},
    {"border-color",[](std::shared_ptr<UIFrame> element, std::string value){element->setBorderColor(parseColor(value));}}
};

void UIStyle::parseQuery(std::string type, std::string value, std::string source){
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

    if(type == "#"){
        query.type = UIStyleQuery::ID;
        id_queries[value] = query;
    }
    else if(type == "."){
        query.type = UIStyleQuery::CLASS;
        class_queries[value] = query;
    }
    else{
        query.type = UIStyleQuery::TAG;
        tag_queries[value] = query;
    }
}

UIStyle::UIStyle(std::string path){
    std::ifstream f(path, std::ios::in | std::ios::binary);

    const auto sz = std::filesystem::file_size(path);
    std::string source(sz, '\0');
    f.read(source.data(), sz);   

    // Regex for individual queries
    std::regex pattern(R"((#|\.|)([a-zA-Z_]+)\{([\s\S]*?)\})"); 
    // Remove all spaces
    source.erase(std::remove_if(source.begin(), source.end(), isspace), source.end());

    auto matches_begin = std::sregex_iterator(source.begin(), source.end(), pattern);
    auto matches_end = std::sregex_iterator();

    for (std::sregex_iterator i = matches_begin; i != matches_end; ++i) {
        std::smatch match = *i;
        
        parseQuery(match[1],match[2],match[3]);
    }
}

void UIStyle::applyTo(
    std::shared_ptr<UIFrame> element,
    std::string tag,
    std::string id,
    const std::vector<std::string>& classes
){
    if(tag_queries.count(tag) != 0) tag_queries[tag].applyTo(element);
    for(auto& classname: classes) if(class_queries.count(classname) != 0) class_queries[classname].applyTo(element);
    if(id_queries.count(id)) id_queries[id].applyTo(element);
}

void UIStyleQuery::applyTo(std::shared_ptr<UIFrame> element){
    for(auto& attr: attributes){
        attributeApplyFunctions[attr.name](element, attr.value);
    }    
}