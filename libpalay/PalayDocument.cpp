#include "PalayDocument.h"
#include <QTextCursor>
#include <QPrinter>
#include <QTextCharFormat>
#include <QTextBlockFormat>

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

PalayDocument::PalayDocument(QObject *parent) :
    QObject(parent),
    doc_(new QTextDocument(this)),
    cursor_(new QTextCursor(doc_))
{
}

PalayDocument::~PalayDocument()
{
    delete cursor_;
}

int PalayDocument::paragraph(lua_State *L)
{
    cursor_->insertBlock();
    text(L);
    cursor_->insertBlock();
    return 0;
}

int PalayDocument::text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 2);
    cursor_->insertText(QString::fromUtf8(text));
    return 0;
}


static void debugStackDump(lua_State *L)
{
    int top = lua_gettop(L);
    for (int i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {
        case LUA_TSTRING:  /* strings */
            qDebug("%d: `%s'", i, lua_tostring(L, i));
            break;

        case LUA_TBOOLEAN:  /* booleans */
            qDebug("%d: %s", i, lua_toboolean(L, i) ? "true" : "false");
            break;

        case LUA_TNUMBER:  /* numbers */
            qDebug("%d: %g", i, lua_tonumber(L, i));
            break;

        default:  /* other values */
            qDebug("%d: %s %p", i, lua_typename(L, t), lua_topointer(L, i));
            break;
        }
    }
}

int PalayDocument::style(lua_State *L)
{
    QTextCharFormat charFormat;

    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
        if (!lua_isstring(L, -2))
            luaL_error(L, "Invalid key in style table. All style keys must be strings.");
        const char *key = lua_tostring(L, -2);
        if (strcmp(key, "font_family") == 0) {
            if (!lua_isstring(L, -1))
                luaL_error(L, "Invalid value for font_family. Must be a string.");
            charFormat.setFontFamily(lua_tostring(L, -1));
        } else if (strcmp(key, "font_size") == 0) {
            if (!lua_isnumber(L, -1) || lua_tointeger(L, -1) <= 0)
                luaL_error(L, "Invalid value for font_size. Must be a positive number.");
            charFormat.setFontPointSize(lua_tointeger(L, -1));
        } else if (strcmp(key, "font_style") == 0) {
            if (!lua_isnumber(L, -1) || !setFontStyle(charFormat, lua_tointeger(L, -1)))
                luaL_error(L, "Invalid value for font_style.");
        } else {
            luaL_error(L, "Invalid key in style table: %s", key);
        }
        lua_pop(L, 1);
    }

    cursor_->mergeCharFormat(charFormat);

    return 0;
}

int PalayDocument::saveAs(lua_State *L)
{
    const char *path = luaL_checkstring(L, 2);
    QPrinter pdfPrinter(QPrinter::HighResolution);
    pdfPrinter.setOutputFileName(QString::fromUtf8(path));
    pdfPrinter.setOutputFormat(QPrinter::PdfFormat);
    pdfPrinter.setColorMode(QPrinter::Color);

    doc_->print(&pdfPrinter);
    return 0;
}

bool PalayDocument::setFontStyle(QTextCharFormat &format, int style)
{
    if (style < 0 || style > (Bold + Italic + Underline))
        return false;
    format.setFontWeight(style & Bold ? QFont::Bold : QFont::Normal);
    format.setFontItalic(style & Italic);
    format.setFontUnderline(style & Underline);
    return true;
}
