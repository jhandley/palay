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

static int startTable(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->startTable(L);
}

static int cell(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->cell(L);
}

static int endTable(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->endTable(L);
}

static int image(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->image(L);
}

static int html(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->html(L);
}

static int saveAs(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->saveAs(L);
}

static int pageSize(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->pageSize(L);
}

static int pageMargins(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->pageMargins(L);
}

static int getPageWidth(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->getPageWidth(L);
}

static int getPageHeight(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->getPageHeight(L);
}

static int getPageCount(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->getPageCount(L);
}

static int pageBreak(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->pageBreak(L);
}

static int startBlock(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->startBlock(L);
}

static int endBlock(lua_State *L)
{
    PalayDocument *doc = checkDocument(L, 1);
    return doc->endBlock(L);
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
    {"startTable", startTable},
    {"cell", cell},
    {"endTable", endTable},
    {"image", image},
    {"html", html},
    {"saveAs", saveAs},
    {"getPageWidth", getPageWidth},
    {"getPageHeight", getPageHeight},
    {"getPageCount", getPageCount},
    {"pageSize", pageSize},
    {"pageMargins", pageMargins},
    {"pageBreak", pageBreak},
    {"startBlock", startBlock},
    {"endBlock", endBlock},
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
            app = QSharedPointer<QCoreApplication>(new QApplication(argc, argv, true));
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

        lua_pushinteger(L, (int) PalayDocument::None);
        lua_setfield(L, -2, "None");
        lua_pushinteger(L, (int) PalayDocument::Dashed);
        lua_setfield(L, -2, "Dashed");
        lua_pushinteger(L, (int) PalayDocument::Dotted);
        lua_setfield(L, -2, "Dotted");
        lua_pushinteger(L, (int) PalayDocument::Solid);
        lua_setfield(L, -2, "Solid");

        lua_pushinteger(L, (int) PalayDocument::Left);
        lua_setfield(L, -2, "Left");
        lua_pushinteger(L, (int) PalayDocument::Right);
        lua_setfield(L, -2, "Right");
        lua_pushinteger(L, (int) PalayDocument::HCenter);
        lua_setfield(L, -2, "HCenter");
        lua_pushinteger(L, (int) PalayDocument::Top);
        lua_setfield(L, -2, "Top");
        lua_pushinteger(L, (int) PalayDocument::Bottom);
        lua_setfield(L, -2, "Bottom");
        lua_pushinteger(L, (int) PalayDocument::VCenter);
        lua_setfield(L, -2, "VCenter");
        lua_pushinteger(L, (int) PalayDocument::Center);
        lua_setfield(L, -2, "Center");

        return 1;
    }
}
