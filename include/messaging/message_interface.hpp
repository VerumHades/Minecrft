#pragma once

#include <structure/definitions.hpp>
#include <cstdlib>
#include <vector>

struct Message {
    unsigned int type;
    size_t length;
};

class MessageInterface {
    public:
        /*
            A function called to send data
        */
        virtual void Send(const byte* data, size_t size) = 0;
        /*
            A callback called on reception of data
        */
        virtual void Recieve(const byte* data, size_t size) = 0;

        template <typename T>
        void Send(const T& message){
            static_assert(std::is_base_of<Message, T>::value, "T must derive from Base");
        }
};
