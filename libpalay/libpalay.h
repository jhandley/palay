#ifndef LIBPALAY_H
#define LIBPALAY_H

#include "libpalay_global.h"

extern "C" {

    #include <lua.h>

    int LIBPALAYSHARED_EXPORT luaopen_libpalay(lua_State *L);
}

#endif // LIBPALAY_H
