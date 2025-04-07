#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/items/item.hpp>

SerializeFunction(Item) {
    auto* prototype = this_.getPrototype();
    std::string name = prototype ? prototype->getName() : "NO_NAME";
    array.Append(name);
    array.Append<int>(this_.quantity);

    return true;
}
SerializeInstatiate(Item)

DeserializeFunction(Item){
    ResolvedOption(name, ReadString);
    ResolvedOption(quantity, Read<int>);

    this_.prototype = ItemRegistry::get().getPrototype(name);
    this_.setQuantity(quantity);

    return true;
}
DeserializeInstatiate(Item);
