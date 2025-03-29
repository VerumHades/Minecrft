#pragma once

#include <structure/streams/stream.hpp>

class Buffer: public Stream{
    private:
        size_t cursor = 0;

    public:
        /*
            Read bytes into buffer 
        */
        virtual bool Read(size_t offset, size_t size, byte* buffer) = 0;
        /*
            Write bytes into buffer at offset
        */
        virtual bool Write(size_t offset, size_t size, byte* buffer) = 0;

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
        bool Write(size_t size, byte* buffer) override;
};