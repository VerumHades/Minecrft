#include <game/colliders.hpp>

void RectangularCollider::serialize(ByteArray& array){
    array.append<float>(x); 
    array.append<float>(y); 
    array.append<float>(z);
    array.append<float>(width); 
    array.append<float>(height); 
    array.append<float>(depth);
}
RectangularCollider RectangularCollider::deserialize(ByteArray& array){
    RectangularCollider collider{};
    
    collider.x = array.read<float>();
    collider.y = array.read<float>();
    collider.z = array.read<float>();

    collider.width = array.read<float>();
    collider.height = array.read<float>();
    collider.depth = array.read<float>();
    
    return collider;
}