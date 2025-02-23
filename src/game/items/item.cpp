#include <game/items/item.hpp>

ItemRegistry& ItemRegistry::get(){
    static ItemRegistry registry{};
    return registry;
}

ItemPrototype* ItemRegistry::addPrototype(ItemPrototype prototype){
    prototypes[prototype.name] = std::make_unique<ItemPrototype>(prototype);
    return prototypes.at(prototype.name).get();
}

ItemPrototype* ItemRegistry::getPrototype(std::string name){
    if(!prototypes.contains(name)) return nullptr;
    return prototypes.at(name).get();
}

ItemPrototype::ItemPrototype(std::string name, const BlockRegistry::BlockPrototype* prototype): name(name){
    if(prototype->render_type != BlockRegistry::BILLBOARD) display_type = BLOCK;
    is_block = true;

    if(prototype->single_texture){
        auto texture_path = prototype->texture_paths[0];
        texture_paths = {texture_path, texture_path, texture_path};
    }
    else if(prototype->render_type == BlockRegistry::FULL_BLOCK){
        texture_paths = {
            prototype->texture_paths[0],
            prototype->texture_paths[2],
            prototype->texture_paths[4]
        };
    }

    if(prototype->render_type == BlockRegistry::BILLBOARD) model = std::make_shared<SpriteModel>(prototype->texture_paths[0]);
    else model = std::make_shared<BlockModel>(prototype);
    this->block_id = prototype->id;
}
ItemPrototype::ItemPrototype(std::string name, std::string texture_path): name(name){
    display_type = SIMPLE;
    texture_paths[0] = texture_path;
    model = std::make_shared<SpriteModel>(texture_path);
}


void Item::serialize(ByteArray& to){
    auto* prototype = getPrototype();
    std::string name = prototype ? prototype->getName() : "NO_NAME";
    to.append(name);
    to.append<int>(quantity);
}
ItemID Item::deserialize(ByteArray& from){
    std::string name = from.sread();
    int quantity = from.read<int>();

    ItemID id = ItemRegistry::get().createItem(name);
    std::cout << "Creating item: "  << name <<  " id:" << id << std::endl;

    if(id == NO_ITEM) return id;
    ItemRegistry::get().getItem(id)->setQuantity(quantity);
    return id;
}

ItemPrototype* ItemRegistry::createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype){
    std::string prototype_name = "block_" + prototype->name;

    auto* existing_prototype = getPrototype(prototype_name);
    if(existing_prototype) return existing_prototype;

    return addPrototype({prototype_name, prototype});
}

bool ItemRegistry::prototypeExists(std::string name){
    return prototypes.contains(name);
}

static size_t itemID = NO_ITEM;
ItemID ItemRegistry::createItem(ItemPrototype* prototype){
    size_t id = itemID;
    if(id == NO_ITEM){
        itemID = 1;
        id = 1;
    }
    itemID++;

    items.try_emplace(id,prototype);
    return id;
}  
ItemID ItemRegistry::createItem(std::string prototype_name){
    auto prototype = getPrototype(prototype_name);
    if(!prototype) return NO_ITEM;
    return createItem(getPrototype(prototype_name));
}

Item* ItemRegistry::getItem(ItemID id){
    if(id == NO_ITEM) return nullptr;
    if(!items.contains(id)) return nullptr;
    return &items.at(id);
}
void ItemRegistry::deleteItem(ItemID id){
    if(items.contains(id)) items.erase(id);
}

bool LogicalItemSlot::takeItemFrom(LogicalItemSlot& source, int quantity){
    bool source_has_item = source.hasItem();
    bool destination_has_item = hasItem();

    if(!source_has_item) return false;
    
    Item& source_item = *source.getItem();
    auto* source_item_prototype = source_item.getPrototype();
    if(!source_item_prototype) return false;

    int source_quantity = source_item.getQuantity();
    if(quantity == -1) quantity = source_quantity;

    if(destination_has_item){
        Item& destination_item = *getItem();
        auto* destination_item_prototype = destination_item.getPrototype();
        
        if(source_item_prototype != destination_item_prototype) return false;

        if(source_quantity <= quantity){
            source.clear();
            quantity = source_quantity;
        }
        else source_item.setQuantity(source_quantity - quantity);

        destination_item.setQuantity(destination_item.getQuantity() + quantity);
        return true;
    }

    item = source.getPortion(quantity);
    return true;
}
bool LogicalItemSlot::moveItemTo(LogicalItemSlot& destination, int quantity){
    return destination.takeItemFrom(*this, quantity);
}

ItemID LogicalItemSlot::getPortion(int quantity){
    if(!hasItem()) return NO_ITEM;

    Item& source_item = *getItem();
    auto* source_item_prototype = source_item.getPrototype();
    if(!source_item_prototype) return NO_ITEM;
    
    int source_quantity = source_item.getQuantity();
    if(quantity == -1) quantity = source_quantity;

    ItemID output_id = NO_ITEM;

    if(source_quantity <= quantity){
        output_id = item;
        item = NO_ITEM;
    }
    else{
        source_item.setQuantity(source_quantity - quantity);

        output_id = ItemRegistry::get().createItem(source_item.getPrototype());
        ItemRegistry::get().getItem(output_id)->setQuantity(quantity);
    }
    return output_id;
}

int LogicalItemSlot::decreaseQuantity(int number){
    if(!hasItem()) return 0;
    auto& source_item = *getItem();
    int quantity = source_item.getQuantity();

    if(quantity <= number){
        clear();
        return quantity; 
    }
    else{
        source_item.setQuantity(quantity - number);
        return number;
    }
}
void LogicalItemSlot::clear(){
    ItemRegistry::get().deleteItem(item);
    item = NO_ITEM;
}

bool LogicalItemSlot::swap(LogicalItemSlot& slot){
    ItemID other = slot.item;
    slot.item = item;
    item = other;
    return true;
}

LogicalItemInventory::LogicalItemInventory(int slots_horizontaly, int slots_verticaly): slots(slots_horizontaly * slots_verticaly),
slots_horizontaly(slots_horizontaly), slots_verticaly(slots_verticaly){}

bool LogicalItemInventory::addItem(ItemID item_id){
    auto* item = ItemRegistry::get().getItem(item_id);
    if(!item) return false;
    auto* prototype = item->getPrototype();
    if(!prototype) return false;
    //std::cout << "Adding item " << prototype << std::endl;

    LogicalItemSlot* first_empty_slot = nullptr;

    for(int y = 0;y < slots_verticaly;y++)
    for(int x = 0;x < slots_horizontaly;x++){
        auto* slot = getSlot(x,y);

        if(!slot->hasItem()){
            if(first_empty_slot == nullptr) first_empty_slot = slot;
            continue;
        }

        if(slot->addItem(item_id)) return true;
    }

    if(first_empty_slot)
        return first_empty_slot->addItem(item_id );

    return false;
}


void LogicalItemInventory::serialize(ByteArray& to){
    to.append<int>(slots_horizontaly);
    to.append<int>(slots_verticaly);

    size_t items_total = 0;
    for(int y = 0;y < slots_verticaly;y++)
    for(int x = 0;x < slots_horizontaly;x++){
        auto* slot = getSlot(x,y);
        if(!slot->hasItem()) continue;
        items_total++;
    }

    to.append<size_t>(items_total);

    for(int y = 0;y < slots_verticaly;y++)
    for(int x = 0;x < slots_horizontaly;x++){
        auto* slot = getSlot(x,y);

        if(!slot->hasItem()) continue;

        to.append<int>(x);
        to.append<int>(y);
        slot->getItem()->serialize(to);
    }
}
LogicalItemInventory LogicalItemInventory::deserialize(ByteArray& from){
    LogicalItemInventory output{from.read<int>(),from.read<int>()};
    size_t items_total = from.read<size_t>();

    for(int i = 0;i < items_total;i++){
        int x = from.read<int>();
        int y = from.read<int>();

        auto* slot = output.getSlot(x,y);
        if(!slot) continue;
        slot->setItem(Item::deserialize(from));
    }

    return output;
}

void DroppedItem::serialize(ByteArray& array){
    Item* item = ItemRegistry::get().getItem(this->item);
    if(!item) return;
    item->serialize(array);
}

void DroppedItem::setup(Entity* entity){
    Item* item = ItemRegistry::get().getItem(this->item);
    if(!item) return;

    entity->setModel(item->getPrototype()->getModel());
    entity->onCollision = [](Entity* self, Entity* entity) {
        if(!entity->hasTag("player")) return;
        self->destroy = true;
    };
    entity->setSolid(false);
}

std::shared_ptr<EntityData> DroppedItem::deserializeData(ByteArray& array){
    ItemID item = Item::deserialize(array);
    return std::make_shared<DroppedItem>(item);
}

void DroppedItem::update(GameState* state){
    
}