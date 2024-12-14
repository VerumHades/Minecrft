#include <parsing/tokenizer.hpp>


static const std::string digits = "0123456789";
static const std::string tokenCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";
static const std::string operationCharacters = "+-*/=!";
static const std::string openerCharacters = "[({";
static const std::string closerCharacters = "])}";

static inline bool isCharacterOf(char character, const std::string& source){
    return source.find(character) != std::string::npos;
}

std::string getTokenTypeName(Token::Type type){
    switch (type)
    {
        case Token::Type::EXPRESSION: return "EXPRESSION";
        case Token::Type::SCOPE: return "SCOPE";
        case Token::Type::INDEXER: return "INDEXER";
        case Token::Type::ID: return "ID";
        case Token::Type::SEPARATOR: return "SEPARATOR";
        case Token::Type::OPERATOR: return "OPERATOR";
        case Token::Type::EMPTY: return "EMPTY";
        case Token::Type::MACRO: return "MACRO";
        case Token::Type::UNDEFINED: return "UNDEFINED";
        case Token::Type::COMMENT: return "COMMENT";
        case Token::Type::NUMBER: return "NUMBER";
        case Token::Type::FLOAT: return "FLOAT";
        default: return "UNDEFINED";
    }
}

Token Token::from(std::string& source, size_t start){
    char current_character = source[start];
    
    if(start + 1 < source.size() && current_character == '/' && source[start + 1] == '/'){
        int i = start;
        while(i < source.size() && source[i] != '\n') i++;

        return {COMMENT, start, i, &source};
    }

    if(isCharacterOf(current_character, digits)){
        int i = start;
        
        while(i < source.size() && isCharacterOf(source[i], digits)) i++;
        if(i < source.size() && source[i] == '.'){
            i++;
            while(i < source.size() && isCharacterOf(source[i], digits)) i++;
            if(i < source.size() && source[i] == 'f') i++;

            return {FLOAT, start, i, &source};
        }

        return {NUMBER, start, i, &source};
    }

    if(isCharacterOf(current_character, tokenCharacters)){
        int i = start;
        
        while(i < source.size() && isCharacterOf(source[i], tokenCharacters)) i++;

        return {ID, start, i, &source};
    }
    
    if(isCharacterOf(current_character, operationCharacters))
        return {OPERATOR, start, start + 1, &source};
    
    size_t opener_index = openerCharacters.find(current_character);
    if(opener_index != std::string::npos){
        std::stack<char> toBeClosed = {};
        toBeClosed.push(closerCharacters[opener_index]);
        char source_char = openerCharacters[opener_index];

        int i = start + 1;
        while(i < source.size() && toBeClosed.size() > 0){
            char checked_character = source[i];

            size_t checked_opener_index = openerCharacters.find(checked_character);
            if(checked_opener_index != std::string::npos){
                toBeClosed.push(closerCharacters[checked_opener_index]);
            }

            if(checked_character == toBeClosed.top()){
                toBeClosed.pop();
            }

            i++;
        }

        Token::Type type = EXPRESSION;

        switch (source_char)
        {
            case '[': type = INDEXER; break;
            case '{': type = SCOPE; break;
            default: break;
        }

        return {type, start, i, &source};
    }

    if(current_character == ' '){
        int i = start;
        while(i < source.size() && (source[i] == ' ' || source[i] == '\t')) i++;

        return {EMPTY, start, i, &source};
    }

    if(current_character == '#'){
        int i = start;
        while(i < source.size() && source[i] != '\n') i++;

        return {MACRO, start, i, &source};
    }

    if(current_character == ';') return {SEPARATOR, start, start + 1, &source};

    return {UNDEFINED, start, start + 1, &source};
} 

bool Token::hasNext(){
    return this->_end + 1 < source->size();
}

Token Token::next(){
    return Token::from(*source, this->_end);
}
Token Token::subtoken(size_t relative_start){
    return Token::from(*source, this->_start + relative_start);
}