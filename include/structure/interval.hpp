#pragma once


template <typename T>
class Interval{
    private:
        T min;
        T max;
        bool include_min;
        bool include_max;
    public:
        Interval(const T& min, const T& max, bool include_min = true, bool include_max = true):
        min(min), max(max), include_min(include_min), include_max(include_max){}
        bool IsWithin(const T& value){
            return (include_min ? value >= min  : value > min) && (include_max ? value <= max : value < max);
        }
};
