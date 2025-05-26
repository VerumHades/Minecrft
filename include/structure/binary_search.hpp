#pragma once

#include <type_traits>

/**
 * @brief In an array of T, look for a key K of arithmetic type, where K is a property of T
 * 
 * @tparam T element type
 * @tparam K key type
 * @param key_ptr Relative pointer to key in T
 * @param key The key to look for
 * @param array The array of T
 * @param size The size of the array of T
 * @return T* If found pointer to the element
 */
template <typename T, typename K> T* BinarySearch(K T::* key_ptr, K key, T* array, size_t size) {
    static_assert(std::is_arithmetic<K>::value, "Error: Type K is not arithmetic!");

    size_t segment_start = 0;
    size_t segment_size = size;

    while (segment_size > 1) {
        size_t lower_size = segment_size / 2;
        size_t index = segment_start + lower_size;

        K& lower_key = array[index - 1].*key_ptr;
        K& upper_key = array[index].*key_ptr;

        if (key == lower_key)
            return &array[index - 1];
        if (key == upper_key)
            return &array[index];

        if (key < lower_key) // Is in the lower portion
            segment_size = lower_size;
        else if (key > upper_key) { // Is in the upper portion
            segment_size = segment_size - lower_size;
            segment_start += lower_size;
        } else
            return nullptr; // Is between
    }

    return nullptr;
}

#define BINARY_SEARCH_ORDER_INVALID -2
/**
 * @brief In an array of T, look for the lowest index where K arithmetic can be placed and maintain the ascending order, where K is a property of T
 * 
 * @tparam T element type
 * @tparam K key type
 * @param key_ptr Relative pointer to key in T
 * @param key The key to look for
 * @param array The array of T
 * @param size The size of the array of T
 * @return int index of the element or BINARY_SEARCH_ORDER_INVALID
 */
template <typename T, typename K> int BinarySearchOrder(K T::* key_ptr, K key, T* array, size_t size) {
    size_t current_start = 0;
    size_t current_size = size;

    while (current_size >= 0) {
        size_t lower_size = current_size / 2;

        size_t lower_value_index = current_start + lower_size - 1;
        size_t upper_value_index = current_start + lower_size;

        if (lower_value_index < 0)
            return -1;
        if (upper_value_index >= size)
            return size;

        auto lower_value = array[lower_value_index].*key_ptr;
        auto upper_value = array[upper_value_index].*key_ptr;

        if (key > lower_value && key > upper_value) {
            current_size = current_size - lower_size;
            current_start += lower_size;
        } else if (key < lower_value && key < upper_value)
            current_size = lower_size;
        else if (key >= lower_value && key <= upper_value)
            return upper_value_index;
    }

    return BINARY_SEARCH_ORDER_INVALID;
}
