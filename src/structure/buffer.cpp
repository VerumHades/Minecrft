#include <structure/streams/buffer.hpp>

bool Buffer::Read(size_t size, byte* buffer) {
    bool result = Read(cursor, size, buffer);
    cursor += size;
    return result;
}

bool Buffer::Write(size_t size, const byte* buffer) {
    bool result = Write(cursor, size, buffer);
    cursor += size;
    return result;
}

// Moves cursor to set position
void Buffer::SetCursor(size_t position){
    cursor = position;
}
// Moves cursor by offset position
void Buffer::MoveCursor(int offset){
    cursor += offset;
}
