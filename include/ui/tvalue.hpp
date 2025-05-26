#pragma once

#include <vector>

enum Units{
    AUTO, // Some elements support automatic resize 
    PIXELS,

    WINDOW_WIDTH, // Percentage of the window width
    WINDOW_HEIGHT, // Percentage of the window height
    
    MY_PERCENT, // Percentage of the size of the widget
    /*
        The percentage of parent.
        Subtracts elements own margin and border sizes from the width, 100% is a perfect fit even with a margin and border
    */
    PERCENT,
    FIT_CONTENT, // Size of the content transform

    OPERATION_PLUS    , // TValue + TValue (resolved to pixels)
    OPERATION_MINUS   , // TValue - TValue (resolved to pixels)
    OPERATION_MULTIPLY, // TValue * TValue (resolved to pixels)
    OPERATION_DIVIDE  , // TValue / TValue (resolved to pixels)
};

/**
 * @brief A value used for sizes in ui
 * 
 */
struct TValue{
    Units unit = AUTO;
    int value = 0; 

    std::vector<TValue> operands;

    TValue(Units unit, int value) : unit(unit), value(value){}
    //TValue(int value) : unit(PIXELS), value(value){}
    TValue(Units unit): unit(unit), value(10){}

    // Gets automatically resolved if the operands have the same types
    TValue(Units operation, TValue op1, TValue op2): unit(operation){
        /*if(op1.unit == op2.unit){
            unit = op1.unit;
            switch(operation){
                case OPERATION_PLUS    : value = op1.value + op2.value; return;
                case OPERATION_MINUS   : value = op1.value - op2.value; return;
                case OPERATION_MULTIPLY: value = op1.value * op2.value; return;
                case OPERATION_DIVIDE  : value = op1.value / op2.value; return;
                default:
                    unit = operation;
                break;
            }
        }*/
        
        operands.push_back(op1);
        operands.push_back(op2);
    }

    bool hasParentReference(){
        if(unit == OPERATION_PLUS || unit == OPERATION_MINUS){
            return operands[0].hasParentReference() || operands[1].hasParentReference();
        }
        return unit == PERCENT;
    }

    TValue operator-(const TValue& other) const {
        return {OPERATION_MINUS, *this, other};
    }
    TValue operator+(const TValue& other) const {
        return {OPERATION_PLUS, *this, other};
    }
    TValue operator*(const TValue& other) const {
        return {OPERATION_MULTIPLY, *this, other};
    }
    TValue operator/(const TValue& other) const {
        return {OPERATION_DIVIDE, *this, other};
    }

    static TValue Center(){
        return TValue{PERCENT,50} - TValue{MY_PERCENT,50};
    }
    static TValue Bottom(TValue offset){
        return (TValue{PERCENT,100} - TValue{MY_PERCENT,100}) - offset;
    }

    static TValue Pixels(int pixels){
        return {PIXELS, pixels};
    }
};

TValue operator"" _px(unsigned long long value);

const static TValue TNONE = {PIXELS, 0};
enum UIElementState{
    BASE,
    HOVER,
    FOCUS
};

struct UISideSizesT{
    TValue top{AUTO,0};
    TValue right{AUTO,0};
    TValue bottom{AUTO,0};
    TValue left{AUTO,0};

    UISideSizesT(
        TValue top,
        TValue right,
        TValue bottom,
        TValue left
    ): top(top), right(right), bottom(bottom), left(left) {}
    UISideSizesT(TValue size): top(size), right(size), bottom(size), left(size) {}
};