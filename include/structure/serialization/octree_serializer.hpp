#pragma once

#include <structure/bytearray.hpp>
#include <structure/octree.hpp>
#include <structure/serialization/serializer.hpp>

template <typename T> class OctreeSerializer {
  private:
    static size_t SerializeValueNode(std::unique_ptr<T>& value, ByteArray& array) {
        size_t location = array.GetCursor();
        Serializer::Serialize<T>(*value.get(), array);
        return location;
    }
    static std::unique_ptr<T> DeserializeValueNode(ByteArray& array) {
        std::unique_ptr<T> value = std::make_unique<T>();
        Serializer::Deserialize(*value, array);
        return value;
    }

    static size_t SerializeNode(typename Octree<T>::Node& node, ByteArray& array, unsigned int level = 0) {
        size_t location = array.GetCursor();
        array.Write<unsigned int>(location, level);

        size_t offset_total = sizeof(unsigned int) + 8 * sizeof(size_t);

        for (int i = 0; i < 8; i++) {
            array.SetCursor(location + offset_total);

            size_t node_start = 0;
            if (node.sub_nodes[i] != nullptr || (level == 0 && node.values[i] != nullptr)) {
                if (level == 0)
                    node_start = SerializeValueNode(node.values[i], array);
                else
                    node_start = SerializeNode(*node.sub_nodes[i], array);
            }

            array.Write<size_t>(location + sizeof(unsigned int) + i * sizeof(size_t), node_start);
            if (node_start == 0)
                continue;

            offset_total += node_start - location;
        }

        return location;
    }

    static std::unique_ptr<typename Octree<T>::Node> DeserializeNode(ByteArray& array) {
        auto node = std::make_unique<typename Octree<T>::Node>();

        size_t location = array.GetCursor();

        auto level_option = array.Read<unsigned int>();
        if (!level_option)
            return nullptr;

        auto level = level_option.value();

        for (int i = 0; i < 8; i++) {
            array.SetCursor(location + sizeof(unsigned int) + i * sizeof(size_t));
            auto pos_option = array.Read<size_t>();

            if (!pos_option)
                continue;

            auto position = pos_option.value();
            array.SetCursor(position);

            if (level == 0)
                node->values[i] = DeserializeValueNode(array);
            else
                node->sub_nodes[i] = DeserializeNode(array);
        }

        return node;
    }

  public:
    static void Serialize(const Octree<T>& tree, ByteArray& array) {
        SerializeNode(*tree.root_node, array, tree.top_level);
    }
    static void Deserialize(Octree<T>& tree, ByteArray& array) {
        size_t location = array.GetCursor();

        auto level_option = array.Read<unsigned int>();
        if (!level_option) return;

        tree.top_level = level_option.value();
        array.SetCursor(location);

        tree.root_node = DeserializeNode(array);
    }
};
