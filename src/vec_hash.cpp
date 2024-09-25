#include <vec_hash.hpp>

std::size_t Vec3Hash::operator()(const glm::vec3& v) const noexcept {
    std::size_t h1 = std::hash<float>{}(v.x);
    std::size_t h2 = std::hash<float>{}(v.y);
    std::size_t h3 = std::hash<float>{}(v.z);
    // Combine the hash values
    return h1 ^ (h2 << 1) ^ (h3 << 2);
}

bool Vec3Equal::operator()(const glm::vec3& lhs, const glm::vec3& rhs) const noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}