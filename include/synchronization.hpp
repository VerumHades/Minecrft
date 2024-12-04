#pragma once

#include <mutex>
#include <atomic>
#include <vector>

/*
    A thread safe buffer that enables one side to write and the other side to read the data without interfierence
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
        void clear(){
            std::lock_guard<std::mutex> pass_lock(pass_mutex);
            std::lock_guard<std::mutex> write_lock(write_mutex);

            write_cursor = 0;
            _size = 0;
        }

        void append(T* data, size_t size){
            write(write_cursor, data, size);
            write_cursor += size;
        }

        void write(size_t at, T* data, size_t size){
            std::lock_guard<std::mutex> lock(write_mutex);

            if(at + size > front.size()) front.resize(at + size);
            std::memcpy(front.data() + at, data, size * sizeof(T));
        }

        /*
            Passes the data to be readable
        */
        void pass(){
            std::lock_guard<std::mutex> write_lock(write_mutex);
            std::lock_guard<std::mutex> pass_lock(pass_mutex);
            std::lock_guard<std::mutex> read_lock(read_mutex);

            back.resize(front.size());
            std::memcpy(back.data(), front.data(), front.size() * sizeof(T));

            _size = back.size();
        }

        T* read(){
            std::lock_guard<std::mutex> read_lock(read_mutex);
            return back.data();
        }

        size_t size(){
            std::lock_guard<std::mutex> pass_lock(pass_mutex);
            return _size;
        }
};