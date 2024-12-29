#pragma once

template <typename T>
class Pair{
    private:
        T value;
        Pair<T>* _partner = nullptr;
    public:
        Pair(){}

        Pair(const Pair& other) = delete;
        Pair& operator=(const Pair& other) = delete;

        // Move Constructor
        Pair(Pair&& other) noexcept {
            value = other.value;
            
            _partner = other._partner;
            if(_partner) _partner->_partner = this;

            other._partner = nullptr;
        }

        // Move Assignment Operator
        Pair& operator=(Pair&& other) noexcept {
            if (this != &other) {
                value = other.value;
            
                _partner = other._partner;
                if(_partner) _partner->_partner = this;

                other._partner = nullptr;
            }
            return *this;
        }

        // Destructor
        ~Pair() {
            if(_partner) _partner->_partner = nullptr;
        }

        Pair(Pair<T>* partner){
            partner->_partner = this;
        }

        Pair<T> create_partner(){
            return Pair<T>(this); 
        } 

        T& value() {return value;};
        Pair<T>* partner() {return _partner;};
};