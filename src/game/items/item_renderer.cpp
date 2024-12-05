#include <game/items/item_renderer.hpp>


ItemTextureAtlas::StoredTexture* ItemTextureAtlas::getPrototypeTexture(ItemPrototype* prototype){
    if(!prototype) return nullptr;

    if(!stored_textures.contains(prototype)){
        this->texture_array->bind(0);

        int width, height, original_channels;
        unsigned char* image_data;
        // Load the image with 4 channels (RGBA)
        image_data = stbi_load(prototype->texture_path.c_str(), &width, &height, &original_channels, 4);

        if (!image_data) {
            std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
            return nullptr;
        }


        if(width != single_texture_size || height != single_texture_size){
            std::cerr << "Item prototype texture has invalid size: " << width << "x" << height << " and not " << single_texture_size << "x" << single_texture_size << std::endl;
            stbi_image_free(image_data);
            return nullptr;
        }

        const int layerIndex = 0;
        int textures_per_row = atlas_size / single_texture_size;

        int x = (textures_stored_total % textures_per_row) * single_texture_size;
        int y = (textures_stored_total / textures_per_row) * single_texture_size;
        // Upload the image data to the specified layer
        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY, // Target
            0,                  // Mipmap level
            x, y, layerIndex,   // x, y, and z (layer) offsets
            width, height, 1,   // Width, height, depth (only one layer here)
            GL_RGBA,            // Format of the pixel data
            GL_UNSIGNED_BYTE,   // Data type of the pixel data
            image_data          // Pointer to the image data
        );

        stored_textures[prototype] = {
            glm::vec2{x,y} / static_cast<float>(atlas_size) ,
            glm::vec2{x + width, y + height} / static_cast<float>(atlas_size) ,
            1
        };

        textures_stored_total++;

        //CHECK_GL_ERROR();;

        stbi_image_free(image_data);
    }

    return &stored_textures[prototype];
}

bool LogicalItemSlot::takeItemFrom(LogicalItemSlot& source, int quantity){
    bool source_has_item = source.item_option.has_value();
    bool destination_has_item = item_option.has_value();

    if(!source_has_item) return false;
    
    Item& source_item = source.item_option.value();
    auto* source_item_prototype = source_item.getPrototype();
    if(!source_item_prototype) return false;

    int source_quantity = source_item.getQuantity();
    if(quantity == -1) quantity = source_quantity;

    if(destination_has_item){
        Item& destination_item = item_option.value();
        auto* destination_item_prototype = destination_item.getPrototype();
        
        if(source_item_prototype != destination_item_prototype) return false;

        if(source_quantity <= quantity){
            source.item_option.reset();
            quantity = source_quantity;
        }
        else source_item.setQuantity(source_quantity - quantity);

        destination_item.setQuantity(destination_item.getQuantity() + quantity);
        return true;
    }
    
    if(source_quantity <= quantity){
        source.item_option.reset();
        quantity = source_quantity;
    }
    else source_item.setQuantity(source_quantity - quantity);

    item_option.emplace(source_item);
    item_option.value().setQuantity(quantity);
    return true;
}
bool LogicalItemSlot::moveItemTo(LogicalItemSlot& destination, int quantity){
    return destination.takeItemFrom(*this, quantity);
}

void LogicalItemSlot::clear(){
    item_option.reset();
}


void ItemSlot::getRenderingInformation(RenderYeetFunction& yeet){
   /*yeet(
        UIRenderInfo::Rectangle(
            transform,
            getAttribute(&UIFrame::Style::backgroundColor)
        ),
        clipRegion
    );*/

    if(!item_slot.hasItem()) return;
    auto& item = item_slot.getItem().value();
    auto* prototype = item.getPrototype();
    if(!prototype) return;

    auto* texture_info = textureAtlas.getPrototypeTexture(prototype);

    yeet(
        UIRenderInfo::Texture(
            transform.x,transform.y,transform.width,transform.height,
            {
                {texture_info->uv_min.x, texture_info->uv_min.y},
                {texture_info->uv_max.x, texture_info->uv_min.y},
                {texture_info->uv_max.x, texture_info->uv_max.y},
                {texture_info->uv_min.x, texture_info->uv_max.y}
            },
            texture_info->index
        ),
        clipRegion
    );

    int slot_x = transform.x + slot_padding;
    int slot_y = transform.y + slot_padding;
    int slot_width  = slot_size - slot_padding * 2;
    int slot_height = slot_width;

    std::string quantity_number = std::to_string(item.getQuantity());
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(quantity_number, quantity_number_font_size);
    manager.buildTextRenderingInformation(yeet, clipRegion, 
        quantity_number, 
        slot_x + slot_width  - textDimensions.x,
        slot_y + slot_height - textDimensions.y,
        quantity_number_font_size,
        getAttribute(&UIFrame::Style::textColor)
    );
}

ItemInventory::ItemInventory(ItemTextureAtlas& textureAtlas, UIManager& manager, int slots_horizontaly, int slots_verticaly, std::shared_ptr<ItemSlot> held_item_ptr): 
    UIFrame(manager), textureAtlas(textureAtlas), slots_horizontaly(slots_horizontaly),
    slots_verticaly(slots_verticaly), items(slots_horizontaly * slots_verticaly), held_item_ptr(held_item_ptr)
{
    dedicated_texture_array = textureAtlas.getTextureArray();

    this->setSize(slots_horizontaly * slot_size, slots_verticaly * slot_size);

    onMouseEvent = [this](UIManager& manager, int button, int action, int mods){
        if(!this->hover) return;
        if(!this->held_item_ptr) return;

        auto mousePosition = manager.getMousePosition();
        int relative_x = mousePosition.x - transform.x;
        int relative_y = mousePosition.y - transform.y;

        int slot_x = relative_x / this->slot_size;
        int slot_y = relative_y / this->slot_size;

        if(slot_x < 0 || slot_x >= this->slots_horizontaly || slot_y < 0 || slot_y >= this->slots_verticaly) return;

        auto& item_slot = items[getIndex(slot_x,slot_y)];
        auto& hand_slot = this->held_item_ptr->getSlot();

        if(button == GLFW_MOUSE_BUTTON_LEFT){
            if(action == GLFW_PRESS && hand_slot.hasItem()){
                hand_slot.moveItemTo(item_slot);
            }
            else if(action == GLFW_PRESS && !hand_slot.hasItem()){
                hand_slot.takeItemFrom(item_slot);
            }
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT){
            if(action == GLFW_PRESS && hand_slot.hasItem()){
                hand_slot.moveItemTo(item_slot, 1);
            }
            else if(action == GLFW_PRESS && !hand_slot.hasItem()){
                hand_slot.takeItemFrom(item_slot, 1);
            }
        }
        
        this->held_item_ptr->update();
        update();
    };
}
    

bool ItemInventory::addItem(Item item){
    auto* prototype = item.getPrototype();

    for(int x = 0;x < slots_horizontaly;x++){
        for(int y = 0;y < slots_verticaly;y++){
            auto& item_slot = items[getIndex(x,y)];

        
            if(item_slot.addItem(item)) return true;
        }
    }

    return false;
}
void ItemInventory::getRenderingInformation(RenderYeetFunction& yeet){
    UIFrame::getRenderingInformation(yeet);

    int line_width = borderSizes.top;
    UIColor line_color = getAttribute(&UIFrame::Style::borderColor)[0];

    for(int y = 1;y < slots_verticaly; y++){
        yeet(
            UIRenderInfo::Rectangle(
                transform.x,
                transform.y + y * slot_size - line_width / 2,
                transform.width,
                line_width,
                line_color
            ),
            clipRegion
        );
    }

    for(int x = 1;x < slots_horizontaly; x++){
        yeet(
            UIRenderInfo::Rectangle(
                transform.x + x * slot_size - line_width / 2,
                transform.y,
                line_width,
                transform.height,
                line_color
            ),
            clipRegion
        );
    }

    for(int x = 0;x < slots_horizontaly;x++){
        for(int y = 0;y < slots_verticaly;y++){
            auto& item_slot = items[getIndex(x,y)];

            if(!item_slot.hasItem()) continue;
            auto& item = item_slot.getItem().value();
            auto* prototype = item.getPrototype();
            if(!prototype) continue;

            auto* texture_info = textureAtlas.getPrototypeTexture(prototype);

            int slot_x = transform.x + x * slot_size + slot_padding;
            int slot_y = transform.y + y * slot_size + slot_padding;
            int slot_width  = slot_size - slot_padding * 2;
            int slot_height = slot_width;

            yeet(
                UIRenderInfo::Texture(
                    slot_x,
                    slot_y,
                    slot_width,
                    slot_height,
                    {
                        {texture_info->uv_min.x, texture_info->uv_min.y},
                        {texture_info->uv_max.x, texture_info->uv_min.y},
                        {texture_info->uv_max.x, texture_info->uv_max.y},
                        {texture_info->uv_min.x, texture_info->uv_max.y}
                    },
                    texture_info->index
                ),
                clipRegion
            );
            std::string quantity_number = std::to_string(item.getQuantity());
            glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(quantity_number, quantity_number_font_size);
            manager.buildTextRenderingInformation(yeet, clipRegion, 
                quantity_number, 
                slot_x + slot_width  - textDimensions.x,
                slot_y + slot_height - textDimensions.y,
                quantity_number_font_size,
                getAttribute(&UIFrame::Style::textColor)
            );
        }
    }
}