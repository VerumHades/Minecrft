#pragma once

#include <string>
#include <stack>
#include <iostream>

class Token{
    public:
        enum Type{
            EXPRESSION,
            SCOPE,
            INDEXER,
            ID,
            SEPARATOR,
            OPERATOR,
            EMPTY,
            MACRO,
            UNDEFINED,
            COMMENT,
            NUMBER,
            FLOAT
        };

    private:
        Type _type;
        size_t _start;
        size_t _end;
        std::string* source;

        Token(Type type, size_t start, size_t end, std::string* source): _type(type),_start(start),_end(end),source(source) { }
    
    public:
        Token(){}
        static Token from(std::string& source, size_t start);

        Token next();
        bool hasNext();

        Token subtoken(size_t relative_start);

        size_t length() {return _end - _start;}
        size_t start() {return _start;}
        size_t end() {return _end;}
        Type type() {return _type;}
        std::string content() {return source->substr(_start, length());}
};