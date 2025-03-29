#pragma once

#include <structure/octree.hpp>
#include <structure/serialization/serializer.hpp>

template <typename T>
class OctreeSerializer {

    public:
        static bool Serialize(Octree<T>& object, ByteArray& output){

        }

        static bool Deserialize(Octree<T>& object, ByteArray& input){

        }
}
