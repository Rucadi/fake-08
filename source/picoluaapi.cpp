
#include <string>

#include "picoluaapi.h"
#include "graphics.h"
#include "Input.h"

extern "C" {
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
}

Graphics* _graphicsForLuaApi;
Input* _inputForLuaApi;

void initPicoApi(Graphics* graphics, Input* input){
    _graphicsForLuaApi = graphics;
    _inputForLuaApi = input;
}

/*functions to expose to lua*/
//Graphics
int cls(lua_State *L){
    _graphicsForLuaApi->cls();

    return 0;
}
int pset(lua_State *L){
    double x = lua_tonumber(L,1);
    double y = lua_tonumber(L,2);
    double c = lua_tonumber(L,3);

    _graphicsForLuaApi->pset((short)x, (short)y, (uint8_t)c);

    return 0;
}
int pget(lua_State *L){
    double x = lua_tonumber(L,1);
    double y = lua_tonumber(L,2);

    uint8_t color = _graphicsForLuaApi->pget((short)x, (short)y);

    lua_pushinteger(L, color);

    return 1;
}
int color(lua_State *L){
    double c = lua_tonumber(L,1);

    _graphicsForLuaApi->color((uint8_t)c);

    return 0;
}
int line (lua_State *L){
    double x1 = lua_tonumber(L,1);
    double y1 = lua_tonumber(L,2);
    double x2 = lua_tonumber(L,3);
    double y2 = lua_tonumber(L,4);
    double c = lua_tonumber(L,5);

    _graphicsForLuaApi->line((short)x1, (short)y1, (short)x2, (short)y2, (uint8_t)c);

    return 0;
}
int circ(lua_State *L){
    double ox = lua_tonumber(L,1);
    double oy = lua_tonumber(L,2);
    double r = lua_tonumber(L,3);
    double c = lua_tonumber(L,4);

    _graphicsForLuaApi->circ((short)ox, (short)oy, (short)r, (uint8_t)c);

    return 0;
}
int circfill(lua_State *L){
    double ox = lua_tonumber(L,1);
    double oy = lua_tonumber(L,2);
    double r = lua_tonumber(L,3);
    double c = lua_tonumber(L,4);

    _graphicsForLuaApi->circfill((short)ox, (short)oy, (short)r, (uint8_t)c);

    return 0;
}
int rect(lua_State *L){
    double x1 = lua_tonumber(L,1);
    double y1 = lua_tonumber(L,2);
    double x2 = lua_tonumber(L,3);
    double y2 = lua_tonumber(L,4);
    double c = lua_tonumber(L,5);

    _graphicsForLuaApi->rect((short)x1, (short)y1, (short)x2, (short)y2, (uint8_t)c);

    return 0;
}
int rectfill(lua_State *L){
    double x1 = lua_tonumber(L,1);
    double y1 = lua_tonumber(L,2);
    double x2 = lua_tonumber(L,3);
    double y2 = lua_tonumber(L,4);
    double c = lua_tonumber(L,5);

    _graphicsForLuaApi->rectfill((short)x1, (short)y1, (short)x2, (short)y2, (uint8_t)c);

    return 0;
}
int print(lua_State *L){
    const char * str = "";

    //todo: handle other cases, maybe move this somewhere else
    //learned this from zepto8 https://github.com/samhocevar/zepto8/blob/27f83fe0626d4823fe2a33568d8310d8def84ae9/src/pico8/vm.cpp
    if (lua_isnil(L, 1)){
        str = "[nil]";
    }
    else if (lua_isstring(L, 1)){
        str = lua_tolstring(L, 1, nullptr);
    }
    else if (lua_isnumber(L, 1)){
        str = lua_tolstring(L, 1, nullptr);
    }
    else if (lua_isboolean(L, 1)){
        str = lua_toboolean(L, 1) ? "true" : "false";
    }

    double x = lua_tonumber(L,2);
    double y = lua_tonumber(L,3);
    double c = lua_tonumber(L,3);

    _graphicsForLuaApi->print(str, x, y, c);

    return 0;
}
int spr(lua_State *L) {
    double n = lua_tonumber(L,1);
    double x = lua_tonumber(L,2);
    double y = lua_tonumber(L,3);
    double w = lua_tonumber(L,4);
    double h = lua_tonumber(L,5);
    bool flip_x = lua_toboolean(L,6);
    bool flip_y = lua_toboolean(L,7);

    _graphicsForLuaApi->spr((short)n, (short)x, (short)y, (short)w, (short)h, flip_x, flip_y);

    return 0;
}

int sspr(lua_State *L) {
    double sx = lua_tonumber(L,1);
    double sy = lua_tonumber(L,2);
    double sw = lua_tonumber(L,3);
    double sh = lua_tonumber(L,4);
    double dx = lua_tonumber(L,5);
    double dy = lua_tonumber(L,6);
    double dw = lua_tonumber(L,7);
    double dh = lua_tonumber(L,8);
    bool flip_x = lua_toboolean(L,9);
    bool flip_y = lua_toboolean(L,10);

    _graphicsForLuaApi->sspr(
        (short)sx,
        (short)sy,
        (short)sw,
        (short)sh,
        (short)dx,
        (short)dy,
        (short)dw,
        (short)dh,
        flip_x,
        flip_y);

    return 0;
}

int fget(lua_State *L) {
    double n = lua_tonumber(L,1);

    if (lua_gettop(L) == 1) {
        uint8_t result = _graphicsForLuaApi->fget((uint8_t)n);
        lua_pushinteger(L, result);
    }
    else {
        double f = lua_tonumber(L,2);
        bool result = _graphicsForLuaApi->fget((uint8_t)n, (uint8_t)f);
        lua_pushboolean(L, result);
    }

    return 1;
}

int fset(lua_State *L) {
    double n = lua_tonumber(L,1);

    if (lua_gettop(L) > 2) {
        double f = lua_tonumber(L,2);
        double v = lua_toboolean(L,3);
        _graphicsForLuaApi->fset((uint8_t)n, (uint8_t)f, (bool)v);
    }
    else {
        double v = lua_tonumber(L,2);
        _graphicsForLuaApi->fset((uint8_t)n, (uint8_t)v);
    }

    return 0;
}

//Input

//input api
int btn(lua_State *L){
    double i = lua_tonumber(L,1);

    bool pressed = _inputForLuaApi->btn((short)i);

    lua_pushboolean(L, pressed);

    return 1;
}
int btnp(lua_State *L){
    double i = lua_tonumber(L,1);

    bool pressed = _inputForLuaApi->btnp((short)i);

    lua_pushboolean(L, pressed);

    return 1;
}