#pragma once

#include <glm/glm.hpp>
#include <functional>
#include <game/chunk_masks.hpp>

struct Plane
{
	glm::vec3 normal = { 0.f, 1.f, 0.f }; // unit vector
	float     distance = 0.f;        // Distance with origin

	Plane() = default;

	Plane(const glm::vec3& p1, const glm::vec3& norm)
		: normal(glm::normalize(norm)),
		distance(glm::dot(normal, p1))
	{}

	float getSignedDistanceToPlane(const glm::vec3& point) const
	{
		return glm::dot(normal, point) - distance;
	}
};

inline bool isAABBOnOrForwardPlane(const Plane& plane, glm::vec3 center, int extent){
    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n

    const float r = 
        std::abs(plane.normal.x) * (extent) +
        std::abs(plane.normal.y) * (extent) +
        std::abs(plane.normal.z) * (extent);

    return -r <= plane.getSignedDistanceToPlane(center);
}

struct Frustum
{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;

    bool isAABBWithing(glm::vec3 position, int extent){
        //Get global scale thanks to our transform
        return  isAABBOnOrForwardPlane(leftFace  , position, extent) &&
                isAABBOnOrForwardPlane(rightFace , position, extent) &&
                isAABBOnOrForwardPlane(topFace   , position, extent) &&
                isAABBOnOrForwardPlane(bottomFace, position, extent) &&
                isAABBOnOrForwardPlane(nearFace  , position, extent) &&
                isAABBOnOrForwardPlane(farFace   , position, extent);
    }
};

using ChunkFoundCallback = std::function<void(glm::ivec3 position)>;
/*
    Find all chunks in a render distances that are inside the frustum. 
    Calls the 'chunkFound' function with its relative position to the center.
    Will break if renderDistance is not even.
*/
void findVisibleChunks(Frustum& frustum, int renderDistance, ChunkFoundCallback chunkFound);