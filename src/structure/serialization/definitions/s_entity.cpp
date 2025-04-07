
#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/entity.hpp>

SerializeFunction(Entity) {
    array.Append<glm::vec3>(this_.position);
    array.Append<glm::vec3>(this_.velocity);
    array.Append<glm::vec3>(this_.lastPosition);
    array.Append<RectangularCollider>(this_.collider);

    array.Append<size_t>(this_.tags.size());
    for(auto& tag: this_.tags) array.Append(tag);

    if(this_.data){
        array.Append(this_.data->type);
        this_.data->serialize(array);
    }
    else array.Append(EntityData::Type::NONE);

    return true;
}
SerializeInstatiate(Entity)

DeserializeFunction(Entity){
    ResolveOptionTo(this_.position, pos_opt, Read<glm::vec3>);
    ResolveOptionTo(this_.velocity, vel_opt, Read<glm::vec3>);
    ResolveOptionTo(this_.lastPosition, lpos_opt, Read<glm::vec3>);
    ResolveOptionTo(this_.collider, coll_opt, Read<RectangularCollider>);

    ResolvedOption(tag_count, Read<size_t>);
    for(size_t i = 0;i < tag_count;i++){
        ResolvedOption(tag, ReadString);
        this_.tags.emplace(tag);
    }

    this_.setData(this_.deserializeData(array));

    return true;
}
DeserializeInstatiate(Entity);
