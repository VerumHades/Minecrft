#pragma once

#include <glm/glm.hpp>


struct RectangularCollider {
    float x, y, z;
    float width, height, depth;
    float bounding_sphere_radius;
    glm::vec3 center;

    RectangularCollider(): RectangularCollider(0,0,0,0,0,0){}
    RectangularCollider(float x, float y, float z, float width, float height, float depth):
    x(x),y(y),z(z),width(width),height(height),depth(depth)
    {
        center = {
            x + width / 2,
            y + height / 2,
            z + depth / 2
        };

        bounding_sphere_radius = glm::distance(glm::vec3{x,y,z},center);
    }

    bool collidesWith(const RectangularCollider* otherCollider, glm::vec3 offset, glm::vec3 other_offset) const {
        return  otherCollider->x + other_offset.x + otherCollider->width  >= x + offset.x && otherCollider->x + other_offset.x <= x + offset.x + width &&
                otherCollider->y + other_offset.y + otherCollider->height >= y + offset.y && otherCollider->y + other_offset.y <= y + offset.y + height &&
                otherCollider->z + other_offset.z + otherCollider->depth  >= z + offset.z && otherCollider->z + other_offset.z <= z + offset.z + depth;
    }
};
