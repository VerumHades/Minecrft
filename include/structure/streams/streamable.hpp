#pragma once

#include <structure/streams/stream.hpp>

/**
 * @brief A generic definition for a streamable object
 * 
 */
class Streamable{
    public:
        virtual bool WriteToStream(Stream& stream) = 0;
        virtual bool LoadFromStream(Stream& stream) = 0;

        template <typename T>
        bool WriteToStream(T& stream){
            return WriteToStream(reinterpret_cast<Stream&>(stream));
        }

        template <typename T>
        bool LoadFromStream(T& stream){
            return LoadFromStream(reinterpret_cast<Stream&>(stream));
        }
};