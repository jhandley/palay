#include "libpalay.h"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}
#include "PalayDocument.h"
#include <QApplication>
#include <QSharedPointer>

static int argc = 0;
static char *argv[] = {};
static QSharedPointer<QCoreApplication> app;

static const char* docMetatableName = "palay.document";

static int newDocument(lua_State *L)
{
    PalayDocument **ud = (PalayDocument**) lua_newuserdata(L, sizeof(PalayDocument*));
    *ud = new PalayDocument();

    luaL_getmetatable(L, docMetatableName);
    lua_setmetatable(L, -2);
    return 1;
}

static PalayDocument* checkDocument(lua_State *L, int index)
{
    PalayDocument **docPtr = (PalayDocument **) luaL_checkudata(L, index, docMetatableName);
    luaL_argcheck(L, docPtr != NULL, 1, "`PalayDocument' expected");
    return *docPtr;
}

static int paragraph(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->paragraph(L);
}

static int text(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->text(L);
}

static int style(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->style(L);
}

static int saveAs(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->saveAs(L);
}

static int gc(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    delete doc;
    return 0;
}

static const struct luaL_Reg palaylib_functions[] = {
    {"newDocument", newDocument},
    {NULL, NULL}
};

static const struct luaL_Reg palaydoc_methods[] = {
    {"paragraph", paragraph},
    {"text", text},
    {"style", style},
    {"saveAs", saveAs},
    {"__gc", gc},
    {NULL, NULL}
};

extern "C" {
    /* This function is called when the module is loaded from Lua
     * via require() e.g. require("libpalay").
     */
    int LIBPALAYSHARED_EXPORT luaopen_libpalay(lua_State *L) {

        // QTextDocument requires a QApplication.
        // If running as a library in a non-Qt application
        // need to create one but don't create two.
        if (!QCoreApplication::instance()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
            app = QSharedPointer<QCoreApplication>(new QGuiApplication(argc, argv));
#else
            app = QSharedPointer<QCoreApplication>(new QApplication(argc, argv, false));
#endif
        }
        luaL_newmetatable(L, docMetatableName);
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2);  // pushes the metatable
        lua_settable(L, -3);  // metatable.__index = metatable
        luaL_setfuncs(L, palaydoc_methods, 0);
        luaL_newlib(L, palaylib_functions);

        lua_pushinteger(L, (int) PalayDocument::Normal);
        lua_setfield(L, -2, "Normal");
        lua_pushinteger(L, (int) PalayDocument::Bold);
        lua_setfield(L, -2, "Bold");
        lua_pushinteger(L, (int) PalayDocument::Italic);
        lua_setfield(L, -2, "Italic");
        lua_pushinteger(L, (int) PalayDocument::Underline);
        lua_setfield(L, -2, "Underline");

        return 1;
    }
}