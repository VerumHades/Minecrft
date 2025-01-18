#include <ui/lua_integration.hpp>

void LuaEngine::run(const std::string& code){
    lua.script(code);
}