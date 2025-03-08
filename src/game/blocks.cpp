#include <game/blocks.hpp>

BlockRegistry& BlockRegistry::get(){
    static std::mutex singleton_mutex;
    std::lock_guard lock(singleton_mutex);
    
    static BlockRegistry registry{};
    return registry;
}

/*
    Adds a full block, where texture path is the texture name for all sides
*/
void BlockRegistry::addFullBlock(std::string name, std::string texture_name, bool transparent){
    BlockPrototype prototype{};

    prototype.id = blocks.size();
    prototype.name = name;
    prototype.colliders = {{0, 0, 0, 1.0f, 1.0f, 1.0f}};

    prototype.single_texture = true;
    prototype.transparent = transparent;

    prototype.textures = {getTextureIndex(texture_name)};
    prototype.render_type = FULL_BLOCK;

    prototype.texture_names = {texture_name};
    prototype.texture_paths = {getTextureByName(texture_name)->path};

    blocks.push_back(std::move(prototype));
}

/*
    Adds a full block, with the defined texture paths
*/
void BlockRegistry::addFullBlock(std::string name, std::array<std::string,6> texture_names, bool transparent){
    std::array<size_t, 6> textures{};
    std::array<std::string, 6> texture_paths{};

    int i = 0;
    for(auto& name: texture_names){
        auto* texture = getTextureByName(name);
        if(!texture) {
            std::cerr << "Invalid texture name: " << name << std::endl;
            return;
        }
        texture_paths[i] = texture->path;
        //std::cout << texture_paths[i] << " " << name << std::endl;

        textures[i++] = getTextureIndex(name);
    }

    BlockPrototype prototype{};

    prototype.id = blocks.size();
    prototype.name = name;
    prototype.colliders = {{0, 0, 0, 1.0f, 1.0f, 1.0f}};

    prototype.single_texture = false;
    prototype.transparent = transparent;

    prototype.textures = textures;
    prototype.render_type = FULL_BLOCK;

    prototype.texture_names = texture_names;
    prototype.texture_paths = texture_paths;
    
    blocks.push_back(std::move(prototype));
}

/*
    Adds a billboard block with the defined texture
*/
void BlockRegistry::addBillboardBlock(std::string name, std::string texture_name){
    BlockPrototype prototype{};

    prototype.id = blocks.size();
    prototype.name = name;
    prototype.colliders = {};

    prototype.single_texture = true;
    prototype.transparent = true;

    prototype.textures = {getTextureIndex(texture_name)};
    prototype.render_type = BILLBOARD;

    prototype.texture_names = {texture_name};
    prototype.texture_paths = {getTextureByName(texture_name)->path};
    
    blocks.push_back(std::move(prototype));
}

size_t BlockRegistry::getIndexByName(std::string name){
    int i = -1;
    for(auto& registered_block: blocks){
        i++;
        if(registered_block.name != name) continue;

        return i;
    }

    return 0;
}

BlockRegistry::BlockPrototype* BlockRegistry::getPrototype(size_t id){
    if(id >= blocks.size()) return nullptr;
    return &blocks[id];
}

BlockRegistry::BlockPrototype* BlockRegistry::getPrototype(const std::string& name){
    return getPrototype(getIndexByName(name));
}
void BlockRegistry::setPrototypeInterface(BlockID id, std::unique_ptr<BlockInterface> interface){
    auto prototype = getPrototype(id);
    if(!prototype) return;

    prototype->interface = std::move(interface);
}

using namespace tinyxml2;

#define for_each_child_as(of, child_name) for ( \
    XMLElement* child_name = of->FirstChildElement(); \
    child_name != nullptr; \
    child_name = child_name->NextSiblingElement()\
)

RectangularCollider BlockRegistry::parseCollider(tinyxml2::XMLElement* element){
    RectangularCollider collider = 
        XMLExtras::Load<RectangularCollider>(element, {
            {"x", offsetof(RectangularCollider,x), XType::FLOAT},
            {"y", offsetof(RectangularCollider,y), XType::FLOAT},
            {"z", offsetof(RectangularCollider,z), XType::FLOAT},
            {"width", offsetof(RectangularCollider,width), XType::FLOAT},
            {"height", offsetof(RectangularCollider,height), XType::FLOAT},
            {"depth", offsetof(RectangularCollider,depth), XType::FLOAT}
        });

    return collider;
}

void BlockRegistry::loadTexture(const char* name, int index, BlockPrototype& prototype){
    if(!name){
        std::cerr << "Missing texture name." << std::endl;
        return;
    }
    
    auto [texture_index, exists] = getTextureIndexChecked(name);
    if(!exists){
        std::cerr << "Texture: '" << name << "' doesnt exist." << std::endl;
        return;
    }

    prototype.textures[index] = texture_index;
    prototype.texture_names[index] = name;
    prototype.texture_paths[index] = getTextureByName(name)->path;
}

void BlockRegistry::processAttribute(tinyxml2::XMLElement* element, BlockPrototype& prototype, bool& found_colliders){
    std::string attribute_name = element->Name();

    if(attribute_name == "render_type"){
        auto* type_name_ptr = element->GetText();
        if(!type_name_ptr) return;
        std::string type_name = type_name_ptr;

        if      (type_name == "FULL_BLOCK") prototype.render_type = FULL_BLOCK;
        else if (type_name == "BILLBOARD") prototype.render_type = BILLBOARD;
        else {
            std::cerr << "Invalid block type: '" << type_name << "'.";
            return;
        }
    }
    else if(attribute_name == "colliders"){
        for_each_child_as(element, collider){
            if(std::string(collider->Name()) == "default_collider") prototype.colliders.push_back({0, 0, 0, 1.0f, 1.0f, 1.0f});
            else prototype.colliders.push_back(parseCollider(collider));
        }
        found_colliders = true;
    }
    else if(attribute_name == "transparent"){
        bool value;
        if(element->QueryBoolText(&value) == XML_SUCCESS) prototype.transparent = value;
        else std::cerr << "Invalid value for block transparency." << std::endl;
    }
    else if(attribute_name == "textures"){
        prototype.single_texture = false;

        int i = 0;
        for_each_child_as(element, texture){
            if(i >= 6) break;
            loadTexture(texture->GetText(), i, prototype);
            i++;
        }
    }
    else if(attribute_name == "texture"){
        prototype.single_texture = true;
        loadTexture(element->GetText(), 0, prototype);
    }
    else if(attribute_name == "hardness"){
        int value;

        if(element->QueryIntText(&value) == XML_SUCCESS) prototype.hardness = value;
        else std::cerr << "Invalid value for block hardness." << std::endl;
    }
    else std::cerr << "Invalid block defintion attribute: '" << attribute_name << "'." <<std::endl;
}

bool BlockRegistry::loadPrototypesFromFile(const std::string& path){
    XMLDocument doc;

    if (doc.LoadFile(path.c_str()) != XML_SUCCESS) {
        std::cerr << "Error loading block XML file." << std::endl;
        return false;
    }

    XMLElement* root = doc.FirstChildElement("blocks");
    if (!root) {
        std::cerr << "No root blocks element found." << std::endl;
        return false;
    }

    for_each_child_as(root, block_definition)
    {
        BlockPrototype prototype{};
        prototype.id = blocks.size();
        prototype.name = block_definition->Name();

        bool found_colliders = false;

        for_each_child_as(block_definition, block_attribute)
        {
            processAttribute(block_attribute, prototype, found_colliders);
        }

        if(!found_colliders) prototype.colliders.push_back({0, 0, 0, 1.0f, 1.0f, 1.0f});

        blocks.push_back(std::move(prototype));
    }

    return true;
}