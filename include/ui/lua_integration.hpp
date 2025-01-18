#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <string>
#include <iostream>
#include <functional>

class LuaEngine{
    private:
        sol::state lua;

    public:
        void run(const std::string& code);

        template <typename T>
        void addFunction(const std::string& name, T f){
            lua.set_function(name, f);
        }
};

