#pragma once

#include <glm/glm.hpp>

#include <bit>
#include <iostream>
#include <memory>

/**
 * @brief A class that implements a generic octree (https://en.wikipedia.org/wiki/Octree)
 * 
 * @tparam T 
 */
template <typename T> class Octree {
  private:
    class Node {
      public:
        std::array<std::unique_ptr<Node>, 8> sub_nodes{};
        std::array<std::unique_ptr<T>, 8> values{};
    };
    unsigned int top_level = 0; // What level is the root node;
    std::unique_ptr<Node> root_node = nullptr;

    void AddLevel() {
        auto node = std::move(root_node);

        root_node = std::make_unique<Node>();
        root_node->sub_nodes[0] = std::move(node);

        top_level++;
    }

    int GetIndex(const glm::uvec3& position, unsigned int level) {
        return ((position.x & 1 << (level - 1)) != 0) + ((position.y & 1 << (level - 1)) != 0) * 2 +
               ((position.z & 1 << (level - 1)) != 0) * 4;
    }

    /*void SnapPosition(glm::uvec3 &position, int level) {
        unsigned int mask = ~(1 << (level - 1));
        position.x &= mask;
        position.y &= mask;
        position.z &= mask;
    }*/

    std::tuple<Node*, int> InternalGet(const glm::uvec3& position, bool create_missing) {
        unsigned int lookup_top_level = 32 - std::countl_zero(position.x | position.y | position.z);

        if (lookup_top_level > top_level && !create_missing)
            return {nullptr, 0};
        else
            while (lookup_top_level > top_level)
                AddLevel();

        Node* current_node = root_node.get();

        unsigned int level = top_level;
        glm::uvec3 current_position = position;

        while (level >= 0) {
            int index = GetIndex(current_position, level);

            if (level == 0)
                return {current_node, index};

            // SnapPosition(current_position, level);
            auto& sub_node = current_node->sub_nodes[index];

            if (!sub_node) {
                if (create_missing)
                    sub_node = std::make_unique<Node>();
                else
                    return {nullptr, 0};
            }

            current_node = sub_node.get();

            level--;
        }
        return {nullptr, 0};
    }

    template <typename A> friend class OctreeSerializer;

  public:
    Octree() {
        root_node = std::make_unique<Node>();
    }
    /**
     * @brief Sets a value to be at a set position.
     * 
     * @param position 
     * @param value 
     */
    void Set(const glm::uvec3& position, std::unique_ptr<T> value) {
        auto [node_ptr, index] = InternalGet(position, true);
        node_ptr->values[index] = std::move(value);
    }

    /**
     * @brief Returns a value at a position or nullptr if it doensnt exist.
     * 
     * @param position 
     * @return T* 
     */
    T* Get(const glm::uvec3& position) {
        auto [node_ptr, index] = InternalGet(position, false);
        if(!node_ptr) return nullptr;
        return node_ptr->values[index].get();
    }
    
    /**
     * @brief Return a value at a position and destroy it (the node)
     * 
     * @param position 
     * @return std::unique_ptr<T> 
     */
    std::unique_ptr<T> Pop(const glm::uvec3& position) {
        auto [node_ptr, index] = InternalGet(position, false);
        if(!node_ptr) return nullptr;
        return std::move(node_ptr->values[index]);
    }
};
