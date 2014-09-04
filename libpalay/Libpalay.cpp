#include "Libpalay.h"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}
#include "PalayDocument.h"
#include <QGuiApplication>

static int argc = 0;
static char *argv[] = {};
static QGuiApplication app(argc, argv);

static const char* docMetatableName = "palay.document";

static int newDocument(lua_State *L)
{
    printf("Creating document");
    void *ud = lua_newuserdata(L, sizeof(PalayDocument));
    new(ud) PalayDocument();

    luaL_getmetatable(L, docMetatableName);
    lua_setmetatable(L, -2);
    return 1;
}

static int paragraph(lua_State *L)
{
    printf("Add paragraph");
    PalayDocument *doc = (PalayDocument *) luaL_checkudata(L, 1, docMetatableName);;
    luaL_argcheck(L, doc != NULL, 1, "`PalayDocument' expected");
    const char *text = luaL_checkstring(L, 2);
    doc->paragraph(QString::fromUtf8(text));
    return 0;
}

static int saveAs(lua_State *L)
{
    printf("saveAs");
    PalayDocument *doc = (PalayDocument *) luaL_checkudata(L, 1, docMetatableName);
    luaL_argcheck(L, doc != NULL, 1, "`PalayDocument' expected");
    const char *path = luaL_checkstring(L, 2);
    doc->toPDF(QString::fromUtf8(path));
    return 0;
}

static const struct luaL_Reg palaylib_functions[] = {
    {"newDocument", newDocument},
    {NULL, NULL}
};

static const struct luaL_Reg palaydoc_methods[] = {
    {"paragraph", paragraph},
    {"saveAs", saveAs},
    {NULL, NULL}
};

extern "C" {
    /* This function is called when the module is loaded from Lua
     * via require() e.g. require("palay").
     */
    int LIBPALAYSHARED_EXPORT luaopen_libpalay(lua_State *L) {

        luaL_newmetatable(L, docMetatableName);
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2);  // pushes the metatable
        lua_settable(L, -3);  // metatable.__index = metatable
        luaL_setfuncs(L, palaydoc_methods, 0);
        luaL_newlib(L, palaylib_functions);
        return 1;
    }
}
