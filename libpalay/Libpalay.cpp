#include "Libpalay.h"

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

static int paragraph(lua_State *L)
{
    PalayDocument *doc = *((PalayDocument **) luaL_checkudata(L, 1, docMetatableName));
    luaL_argcheck(L, doc != NULL, 1, "`PalayDocument' expected");
    const char *text = luaL_checkstring(L, 2);
    doc->paragraph(QString::fromUtf8(text));
    return 0;
}

static int saveAs(lua_State *L)
{
    PalayDocument *doc = *((PalayDocument **) luaL_checkudata(L, 1, docMetatableName));
    luaL_argcheck(L, doc != NULL, 1, "`PalayDocument' expected");
    const char *path = luaL_checkstring(L, 2);
    doc->toPDF(QString::fromUtf8(path));
    return 0;
}

static int gc(lua_State *L)
{
    PalayDocument *doc = *((PalayDocument **) luaL_checkudata(L, 1, docMetatableName));
    delete doc;
    return 0;
}

static const struct luaL_Reg palaylib_functions[] = {
    {"newDocument", newDocument},
    {NULL, NULL}
};

static const struct luaL_Reg palaydoc_methods[] = {
    {"paragraph", paragraph},
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
        return 1;
    }
}
