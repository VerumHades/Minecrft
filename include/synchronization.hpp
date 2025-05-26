#pragma once

#include <mutex>
#include <atomic>
#include <vector>

#include <rendering/opengl/buffer.hpp>

/**
 * @brief A thread safe buffer that enables one side to write and the other side to read the data without interference
 * 
 * @tparam T 
 */
template <typename T>
class PassTroughBuffer{
    private:
        std::mutex write_mutex;
        std::mutex read_mutex;
        std::mutex pass_mutex;

        size_t write_cursor = 0;
        size_t _size = 0;

        std::vector<T> front;
        std::vector<T> back;

    public:
        PassTroughBuffer(){}
        void clear(){
            std::lock_guard<std::mutex> pass_lock(pass_mutex);
            std::lock_guard<std::mutex> write_lock(write_mutex);

            write_cursor = 0;
        }

        /**
         * @brief Appends data to the writable buffer
         * 
         * @param data 
         * @param size 
         */
        void append(T* data, size_t size){
            write(write_cursor, data, size);
            write_cursor += size;
        }

        /**
         * @brief Writes data to the writable buffer
         * 
         * @param at 
         * @param data 
         * @param size 
         */
        void write(size_t at, T* data, size_t size){
            std::lock_guard<std::mutex> lock(write_mutex);

            if(at + size > front.size()) front.resize(at + size);
            std::memcpy(front.data() + at, data, size * sizeof(T));
        }

        /**
         * @brief Passes the data to be readable
         * 
         */
        void pass(){
            std::lock_guard<std::mutex> write_lock(write_mutex);
            std::lock_guard<std::mutex> pass_lock(pass_mutex);
            std::lock_guard<std::mutex> read_lock(read_mutex);

            back.resize(front.size());
            std::memcpy(back.data(), front.data(), front.size() * sizeof(T));

            _size = back.size();
        }

        /**
         * @brief Uploads a data to an opengl buffer
         * 
         * @param buffer 
         */
        void upload(GLBuffer<T, GL_ARRAY_BUFFER>& buffer){
            std::lock_guard<std::mutex> read_lock(read_mutex);  
            buffer.insert(0, back.size(), back.data());
        }

        size_t size(){
            std::lock_guard<std::mutex> pass_lock(pass_mutex);
            return _size;
        }
};