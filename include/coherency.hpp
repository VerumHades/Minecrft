#pragma once

/*
    List that remains cache friendly
*/
template <typename T>
class CoherentList{
    private:
        const int page_size = 4096;

        struct Page{
            std::array<T, page_size> 
        };


};