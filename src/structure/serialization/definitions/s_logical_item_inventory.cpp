#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/items/item.hpp>

SerializeFunction(LogicalItemInventory) {
    array.Append<int>(this_.slots_horizontaly);
    array.Append<int>(this_.slots_verticaly);

    size_t items_total = 0;
    for(int y = 0;y < this_.slots_verticaly;y++)
        for(int x = 0;x < this_.slots_horizontaly;x++){
            auto* slot = this_.getSlot(x,y);
            if(!slot->hasItem()) continue;
            items_total++;
        }

    array.Append<size_t>(items_total);

    for(int y = 0;y < this_.slots_verticaly;y++)
        for(int x = 0;x < this_.slots_horizontaly;x++){
            auto* slot = this_.getSlot(x,y);

            if(!slot->hasItem()) continue;

            array.Append<int>(x);
            array.Append<int>(y);
            Serializer::Serialize<Item>(*slot->getItem(), array);
        }

    return true;
}
SerializeInstatiate(LogicalItemInventory)

DeserializeFunction(LogicalItemInventory){
    ResolveOptionTo(this_.slots_horizontaly, wopt, Read<int>)
    ResolveOptionTo(this_.slots_verticaly, hopt, Read<int>)
    ResolvedOption(items_total, Read<size_t>);

    for(int i = 0;i < items_total;i++){
        ResolvedOption(x, Read<int>);
        ResolvedOption(y, Read<int>);

        auto* slot = this_.getSlot(x,y);
        if(!slot) continue;

        auto item = Item::Create(nullptr);
        Deserialize<Item>(*item, array);
        slot->setItem(item);
    }

    return true;
}
DeserializeInstatiate(LogicalItemInventory);
