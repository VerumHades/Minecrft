#include <game/items/item_renderer.hpp>


ItemTextureAtlas::StoredTexture ItemTextureAtlas::storeImage(const Image& image){
    int width = image.getWidth();
    int height = image.getHeight();
    auto* image_data = image.getData();
    
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

    textures_stored_total++;

    return {
        {
            glm::vec2{x,y} / static_cast<float>(atlas_size) ,
            glm::vec2{x + width, y + height} / static_cast<float>(atlas_size) ,
        },
        1
    };
}

ItemTextureAtlas::TextureSet* ItemTextureAtlas::getPrototypeTextureSet(ItemPrototype* prototype){
    if(!prototype) return nullptr;

    if(!stored_textures.contains(prototype)){
        this->texture_array->bind(0);

        TextureSet set = {};

        for(int i = 0;i < (prototype->display_type == ItemPrototype::SIMPLE ? 1 : 3);i++){
            set.textures[i] = storeImage(Image::LoadWithSize(prototype->texture_paths[i], single_texture_size, single_texture_size));
        }
        
        stored_textures[prototype] = set;
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

void LogicalItemSlot::decreaseQuantity(int number){
    if(!hasItem()) return;
    auto& item = getItem().value();

    if(item.getQuantity() <= number) clear();
    else item.setQuantity(item.getQuantity() - number);
}
void LogicalItemSlot::clear(){
    item_option.reset();
}

void ItemTextureAtlas::RenderItemIntoSlot(UIRenderBatch& batch, ItemPrototype* prototype, UITransform transform){
    auto* texture_info = getPrototypeTextureSet(prototype);
    if(!texture_info) return;
    
    if(prototype->display_type == ItemPrototype::SIMPLE)
        batch.Texture(transform, texture_info->textures[0].uvs);
    else{
        int x = transform.x;
        int y = transform.y;
        int center_x = transform.x + transform.width / 2;
        int center_y = transform.y + transform.height / 2;
        int right_x = transform.x + transform.width;
        int bottom_y = transform.y + transform.height;

        glm::vec2 center = {center_x, center_y};
        glm::vec2 quarter_right = {right_x , y + transform.height / 4};
        glm::vec2 quarter_left  = {x       , y + transform.height / 4};

        batch.TexturePolygon(
            {
                glm::vec2{center_x, y       },
                quarter_right,
                center,
                quarter_left
            },
            texture_info->textures[0].uvs
        );

        batch.TexturePolygon(
            {
                center,
                quarter_right,
                quarter_right + glm::vec2{0, transform.height / 2},
                glm::vec2{center_x, bottom_y}
            },
            texture_info->textures[1].uvs,
            {100,100,100}
        );

        batch.TexturePolygon(
            {
                quarter_left,
                center,
                glm::vec2{center_x, bottom_y},
                quarter_left + glm::vec2{0, transform.height / 2}
            },
            texture_info->textures[2].uvs,
            {150,150,150}
        );
    }
}

void ItemSlot::getRenderingInformation(UIRenderBatch& batch){
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

    textureAtlas.RenderItemIntoSlot(batch,prototype,transform);

    int slot_x = transform.x + slot_padding;
    int slot_y = transform.y + slot_padding;
    int slot_width  = slot_size - slot_padding * 2;
    int slot_height = slot_width;

    std::string quantity_number = std::to_string(item.getQuantity());
    UITextDimensions textDimensions = ui_core.getBackend().getTextDimensions(quantity_number, quantity_number_font_size);
    batch.Text(
        quantity_number, 
        slot_x + slot_width  - textDimensions.width,
        slot_y + slot_height - textDimensions.height,
        quantity_number_font_size,
        getAttribute(&UIFrame::Style::textColor)
    );
}

ItemInventory::ItemInventory(ItemTextureAtlas& textureAtlas, int slots_horizontaly, int slots_verticaly, std::shared_ptr<ItemSlot> held_item_ptr): 
    textureAtlas(textureAtlas), slots_horizontaly(slots_horizontaly),
    slots_verticaly(slots_verticaly), items(slots_horizontaly * slots_verticaly), held_item_ptr(held_item_ptr)
{
    dedicated_texture_array = textureAtlas.getTextureArray();

    setSize(slots_horizontaly * slot_size, slots_verticaly * slot_size);
    setAttribute(&UIFrame::Style::backgroundColor, {20,20,20,200});
    setAttribute(&UIFrame::Style::borderWidth, {3,3,3,3});
    setAttribute(&UIFrame::Style::borderColor, {UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}});

    onMouseEvent = [this](int button, int action, int mods){
        if(!this->hover) return;
        if(!this->held_item_ptr) return;

        auto mousePosition = ui_core.getMousePosition();
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

    LogicalItemSlot* first_empty_slot = nullptr;
    for(int x = 0;x < slots_horizontaly;x++){
        for(int y = 0;y < slots_verticaly;y++){
            auto& item_slot = items[getIndex(x,y)];

            if(!item_slot.hasItem()){
                if(first_empty_slot == nullptr) first_empty_slot = &item_slot;
                continue;
            }

            if(item_slot.addItem(item)) return true;
        }
    }

    if(first_empty_slot)
        return first_empty_slot->addItem(item);

    return false;
}
void ItemInventory::getRenderingInformation(UIRenderBatch& batch){
    int line_width = borderSizes.top;
    UIColor line_color = getAttribute(&UIFrame::Style::borderColor).top;

    batch.BorderedRectangle(
        transform.x - borderSizes.left + line_width / 2,
        transform.y - borderSizes.top  + line_width / 2,
        transform.width  - line_width / 2,
        transform.height - line_width / 2,
        getAttribute(&UIFrame::Style::backgroundColor),
        borderSizes,
        getAttribute(&UIFrame::Style::borderColor)
    );

    for(int y = 1;y < slots_verticaly; y++){
        batch.Rectangle(
            transform.x,
            transform.y + y * slot_size - line_width / 2,
            transform.width,
            line_width,
            line_color
        );
    }

    for(int x = 1;x < slots_horizontaly; x++){
        batch.Rectangle(
            transform.x + x * slot_size - line_width / 2,
            transform.y,
            line_width,
            transform.height - line_width,
            line_color
        );
    }

    for(int x = 0;x < slots_horizontaly;x++){
        for(int y = 0;y < slots_verticaly;y++){
            auto& item_slot = items[getIndex(x,y)];

            if(!item_slot.hasItem()) continue;
            auto& item = item_slot.getItem().value();
            auto* prototype = item.getPrototype();
            if(!prototype) continue;


            int slot_x = transform.x + x * slot_size + slot_padding;
            int slot_y = transform.y + y * slot_size + slot_padding;
            int slot_width  = slot_size - slot_padding * 2;
            int slot_height = slot_width;

            textureAtlas.RenderItemIntoSlot(batch,prototype,{
                slot_x,
                slot_y,
                slot_width,
                slot_height,
            });

            std::string quantity_number = std::to_string(item.getQuantity());
            UITextDimensions textDimensions = ui_core.getBackend().getTextDimensions(quantity_number, quantity_number_font_size);
            batch.Text( 
                quantity_number, 
                slot_x + slot_width  - textDimensions.width,
                slot_y + slot_height - textDimensions.height,
                quantity_number_font_size,
                getAttribute(&UIFrame::Style::textColor)
            );
        }
    }
}