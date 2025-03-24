#include <vec_hash.hpp>

std::size_t Vec2Hash::operator()(const glm::vec2& v) const noexcept {
    std::size_t h1 = std::hash<float>{}(v.x);
    std::size_t h2 = std::hash<float>{}(v.y);
    return h1 ^ (h2 << 1);
}

bool Vec2Equal::operator()(const glm::vec2& lhs, const glm::vec2& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

std::size_t IVec2Hash::operator()(const glm::ivec2& v) const noexcept {
    std::size_t h1 = std::hash<int>{}(v.x);
    std::size_t h2 = std::hash<int>{}(v.y);
    return h1 ^ (h2 << 1);
}

bool IVec2Equal::operator()(const glm::ivec2& lhs, const glm::ivec2& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

std::size_t Vec3Hash::operator()(const glm::vec3& v) const noexcept {
    std::size_t h1 = std::hash<float>{}(v.x);
    std::size_t h2 = std::hash<float>{}(v.y);
    std::size_t h3 = std::hash<float>{}(v.z);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

bool Vec3Equal::operator()(const glm::vec3& lhs, const glm::vec3& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

std::size_t IVec3Hash::operator()(const glm::ivec3& v) const noexcept {
    std::size_t h1 = std::hash<int>{}(v.x);
    std::size_t h2 = std::hash<int>{}(v.y);
    std::size_t h3 = std::hash<int>{}(v.z);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

bool IVec3Equal::operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}
