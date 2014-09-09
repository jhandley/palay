// Copyright (c) 2011-2014 LKC Technologies, Inc.  All rights reserved.
// LKC Technologies, Inc. PROPRIETARY AND CONFIDENTIAL

#ifndef LIBPALAY_H
#define LIBPALAY_H

#include "libpalay_global.h"

extern "C" {

    #include <lua.h>

    int LIBPALAYSHARED_EXPORT luaopen_libpalay(lua_State *L);
}

#endif // LIBPALAY_H
