#pragma once

#include <glm/glm.hpp>


struct RectangularCollider {
    float x, y, z;
    float width, height, depth;

    bool collidesWith(const RectangularCollider* otherCollider, glm::vec3 offset, glm::vec3 other_offset) const {
        return  otherCollider->x + other_offset.x + otherCollider->width  >= x + offset.x && otherCollider->x + other_offset.x <= x + offset.x + width &&
                otherCollider->y + other_offset.y + otherCollider->height >= y + offset.y && otherCollider->y + other_offset.y <= y + offset.y + height &&
                otherCollider->z + other_offset.z + otherCollider->depth  >= z + offset.z && otherCollider->z + other_offset.z <= z + offset.z + depth;
    }
};
