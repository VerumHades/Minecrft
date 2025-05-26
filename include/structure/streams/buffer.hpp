#pragma once

#include <structure/streams/stream.hpp>

/**
 * @brief A generic definition of a buffer
 * 
 */
class Buffer: public Stream{
    private:
        size_t cursor = 0;

    public:
        /**
         * @brief Read bytes into buffer
         * 
         * @param offset 
         * @param size 
         * @param buffer 
         * @return true 
         * @return false 
         */
        virtual bool Read(size_t offset, size_t size, byte* buffer) = 0;
        /**
         * @brief Write bytes into buffer at offset
         * 
         * @param offset 
         * @param size 
         * @param buffer 
         * @return true 
         * @return false 
         */
        virtual bool Write(size_t offset, size_t size, const byte* buffer) = 0;

        // Returns the buffers size
        virtual size_t Size() = 0;

        /*
            Quality of life functions
        */
        // Moves cursor to set position
        void SetCursor(size_t position);
        // Moves cursor by offset position
        void MoveCursor(int offset);
        // Read bytes into buffer at cursor
        bool Read(size_t size, byte* buffer) override;
        // Write bytes into buffer at cursor
        bool Write(size_t size, const byte* buffer) override;

        template <typename T>
        bool Write(size_t offset, const T& value){
            return Write(offset, sizeof(T), reinterpret_cast<const byte*>(&value));
        }

        template <typename T>
        std::optional<T> Read(size_t offset){
            T object{};
            if(!Read(offset, sizeof(T), reinterpret_cast<byte*>(&object))) return std::nullopt;
            return object;
        }
};
