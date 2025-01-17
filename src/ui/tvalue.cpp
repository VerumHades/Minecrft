#include <ui/tvalue.hpp>

TValue operator"" _px(unsigned long long value){
    return {PIXELS, value};
}