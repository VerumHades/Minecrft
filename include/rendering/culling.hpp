#pragma once

#include <glm/glm.hpp>
#include <functional>
#include <iostream>
#include <game/blocks.hpp>

// Some of these are a courtesy of https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling

/**
 * @brief Courtesy of https://learnopengl.com/Guest-Articles/2021/Scene/Frustum-Culling
 * 
 */
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

    bool isForwardOfPlane(const glm::vec3& point) const{
        return getSignedDistanceToPlane(point) >= 0; 
    }
};

/**
 * @brief Checks whether a cubiod is forward or backward to a plane
 * 
 * @param plane 
 * @param min 
 * @param max 
 * @return true 
 * @return false 
 */
inline bool isAABBOnOrForwardPlane(const Plane& plane, glm::vec3& min, glm::vec3& max){
    std::array<glm::vec3, 8> points = {
        min, // min.x,min.y,min.z
        {min.x,max.y,min.z},
        {min.x,max.y,max.z},
        {min.x,min.y,max.z},
        {max.x,min.y,min.z},
        {max.x,max.y,min.z},
        max,
        {max.x,min.y,max.z}
    };

    bool value = plane.isForwardOfPlane(points[0]);

    for(int i = 1;i < 8;i++){
        if(value != plane.isForwardOfPlane(points[i])) return true; // The AABB intersects the plane
    }

    return value;
}

/**
 * @brief A set of planes representing the cameras view space
 * 
 */
struct Frustum
{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;

    bool isAABBWithing(glm::vec3 min, glm::vec3 max){
        return  isAABBOnOrForwardPlane(leftFace  , min, max) &&
                isAABBOnOrForwardPlane(rightFace , min, max) &&
                isAABBOnOrForwardPlane(topFace   , min, max) &&
                isAABBOnOrForwardPlane(bottomFace, min, max) &&
                isAABBOnOrForwardPlane(nearFace  , min, max) &&
                isAABBOnOrForwardPlane(farFace   , min, max);
    }
};

using ChunkFoundCallback = std::function<void(glm::ivec3 position)>;
/*
    Find all chunks in a render distance that are inside the frustum. 
    Calls the 'chunkFound' function with its relative position to the center.
    Will break if renderDistance is not 2^X.
*/
void findVisibleChunks(Frustum& frustum, int renderDistance, ChunkFoundCallback chunkFound);