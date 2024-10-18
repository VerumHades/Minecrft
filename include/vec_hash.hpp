#ifndef VECHASH_H
#define VECHASH_H

#include <glm/glm.hpp>

struct Vec3Hash {
    size_t operator()(const glm::vec3& v) const noexcept;
};

struct Vec3Equal {
    bool operator()(const glm::vec3& lhs, const glm::vec3& rhs) const noexcept;
};

struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const noexcept;
};

struct IVec3Equal {
    bool operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const noexcept;
};

#endif