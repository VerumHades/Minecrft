#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/items/item.hpp> 

SerializeFunction(Item) {
    auto* prototype = getPrototype();
    std::string name = prototype ? prototype->getName() : "NO_NAME";
    array.append(name);
    array.append<int>(quantity);
}
SerializeInstatiate(Item)

DeserializeFunction(Item){
    ResolvedOption(name, sread);
    ResolvedOption(quantity, read<int>);

    ItemRef item = Item::Create(name);

    if(!item) return NO_ITEM;
    this_ = *item;
    this_.setQuantity(quantity);
    return item;
}
DeserializeInstatiate(Item);