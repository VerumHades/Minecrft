#pragma once

#include <stdint.h>
#include <chrono>
#include <iostream>
using uint = unsigned int;

class ScopeTimer{
    private:
        std::chrono::high_resolution_clock::time_point relative_start;
        std::chrono::high_resolution_clock::time_point start;
        std::string message;

    public:
        ScopeTimer(std::string message): message(message){
            start = std::chrono::high_resolution_clock::now();
            relative_start = start;
        }
        void timestamp(std::string message){
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - relative_start).count();
            relative_start = end;
            std::cout << message << " in: " << duration << " microseconds" << std::endl;
        }
        ~ScopeTimer(){
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            std::cout << message << " in: " << duration << " microseconds" << std::endl;
        }
        
};