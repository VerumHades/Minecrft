#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/entity.hpp>

SerializeFunction(Entity) {
    array.append<glm::vec3>(this_.position);
    array.append<glm::vec3>(this_.velocity);
    array.append<glm::vec3>(this_.lastPosition);
    Serializer::Serialize(this_.collider, array);

    array.append<size_t>(this_.tags.size());
    for(auto& tag: this_.tags) array.append(tag);

    if(this_.data){
        array.append(this_.data->type);
        this_.data->serialize(array);
    }
    else array.append(EntityData::Type::NONE);
}
SerializeInstatiate(Entity)

DeserializeFunction(Entity){
    ResolveOptionTo(this_.position, pos_opt, read<glm::vec3>);
    ResolveOptionTo(this_.velocity, vel_opt, read<glm::vec3>);
    ResolveOptionTo(this_.lastPosition, lpos_opt, read<glm::vec3>);

    Serializer::Deserialize(this_.collider, array);

    ResolvedOption(tag_count, read<size_t>);
    for(size_t i = 0;i < tag_count;i++){
        ResolvedOption(tag, sread);
        this_.tags.emplace(tag);
    }

    this_.setData(this_.deserializeData(array));
}
DeserializeInstatiate(Entity);