#ifndef VECHASH_H
#define VECHASH_H

#include <glm/glm.hpp>

/**
 * @brief Ai generated classes for custom hash functions
 * 
 */
struct Vec2Hash {
    size_t operator()(const glm::vec2& v) const noexcept;
};

struct Vec2Equal {
    bool operator()(const glm::vec2& lhs, const glm::vec2& rhs) const noexcept;
};

struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const noexcept;
};

struct IVec2Equal {
    bool operator()(const glm::ivec2& lhs, const glm::ivec2& rhs) const noexcept;
};

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