#include "PalayDocument.h"
#include <QTextCursor>
#include <QPrinter>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextTable>
#include <QTextTableCell>

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

PalayDocument::PalayDocument(QObject *parent) :
    QObject(parent),
    doc_(new QTextDocument(this))
{
    cursorStack_.push(QTextCursor(doc_));

    // Qt's default spacing of 2 causes screwy looking borders since there
    // is space between the borders of adjactent cells.
    tableFormat_.setCellSpacing(0);
    // Qt's default format of 2 is kinda cramped.
    tableFormat_.setCellPadding(4);
    tableFormat_.setBorderBrush(QBrush(Qt::black));
    tableFormat_.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);

}

PalayDocument::~PalayDocument()
{
}

int PalayDocument::paragraph(lua_State *L)
{
    cursorStack_.top().insertBlock();
    text(L);
    cursorStack_.top().insertBlock();
    return 0;
}

int PalayDocument::text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 2);
    cursorStack_.top().insertText(QString::fromUtf8(text));
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
        } else if (strcmp(key, "border_width") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) <= 0)
                luaL_error(L, "Invalid value for border_width. Must be a positive number.");
            tableFormat_.setBorder(lua_tonumber(L, -1));
        } else if (strcmp(key, "border_style") == 0) {
            if (!lua_isnumber(L, -1) || !setBorderStyle(tableFormat_, lua_tointeger(L, -1)))
                luaL_error(L, "Invalid value for border_style.");
        } else if (strcmp(key, "border_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for text_color.");
            tableFormat_.setBorderBrush(QBrush(color));
        } else if (strcmp(key, "text_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for text_color.");
            charFormat.setForeground(QBrush(color));
        } else if (strcmp(key, "background_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for background_color.");
            charFormat.setBackground(QBrush(color));
        } else {
            luaL_error(L, "Invalid key in style table: %s", key);
        }
        lua_pop(L, 1);
    }

    cursorStack_.top().mergeCharFormat(charFormat);

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

int PalayDocument::table(lua_State *L)
{
    int rows = luaL_checkinteger(L, 2);
    int cols = luaL_checkinteger(L, 3);
    if (rows < 1 || cols < 1)
        luaL_error(L, "Tables must have at least one column and at least one row.");

    if (rows < 1)
        luaL_error(L, "Number of table rows must be greater than zero.");
    if (cols < 1)
        luaL_error(L, "Number of table columns must be greater than zero.");

    // Save off position before inserting the table so that we can move past the end
    // of the table when endTable is called.
    cursorStack_.push(cursorStack_.top());

    cursorStack_.top().insertTable(rows, cols, tableFormat_);

    return 0;
}

int PalayDocument::cell(lua_State *L)
{
    int row = luaL_checkinteger(L, 2);
    int col = luaL_checkinteger(L, 3);
    int rowspan = 1;
    int colspan = 1;
    if (lua_gettop(L) >= 4) {
        rowspan = luaL_checkinteger(L, 4);
    }
    if (lua_gettop(L) >= 5) {
        colspan = luaL_checkinteger(L, 5);
    }

    QTextTable *table = cursorStack_.top().currentTable();
    if (!table)
        luaL_error(L, "cell called with no matching call to table()");

    if (row < 1 || row > table->rows())
        luaL_error(L, "Invalid row number %d: must be between 1 and %d", row, table->rows());
    if (col < 1 || col > table->columns())
        luaL_error(L, "Invalid column number %d: must be between 1 and %d", col, table->columns());

    if (rowspan > 1 or colspan > 1)
        table->mergeCells(row - 1, col - 1, rowspan, colspan);

    cursorStack_.top() = table->cellAt(row - 1, col - 1).firstCursorPosition();

    return 0;
}

int PalayDocument::endTable(lua_State *L)
{
    if (!cursorStack_.top().currentTable())
        luaL_error(L, "endTable called with no matching call to table()");

    cursorStack_.pop();
    // Saved cursor is in the parent frame of table so moving to last position
    // in that frame moves past end of table.
    cursorStack_.top() = cursorStack_.top().currentFrame()->lastCursorPosition();
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

bool PalayDocument::setBorderStyle(QTextTableFormat &format, int style)
{
    switch (style) {
    case None:
        format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
        break;
    case Dotted:
        format.setBorderStyle(QTextFrameFormat::BorderStyle_Dotted);
        break;
    case Dashed:
        format.setBorderStyle(QTextFrameFormat::BorderStyle_Dashed);
        break;
    case Solid:
        format.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
        break;
    default:
        return false;
    }

    return true;
}

QColor PalayDocument::getColor(lua_State *L, int index)
{
    index = index > 0 ? index : lua_gettop(L) + index + 1;
    if (lua_type(L, index) == LUA_TSTRING) {
        const char* colorName = lua_tostring(L, index);
        QColor color(colorName);
        if (!color.isValid())
            luaL_error(L, "\"%s\" is not a valid color.", colorName);
        return color;
    } else if (lua_type(L, index) == LUA_TNUMBER) {
        const int colorValue = lua_tointeger(L, index);
        QColor color(colorValue);
        if (!color.isValid())
            luaL_error(L, "%d is not a valid color value.", colorValue);
        return color;
    } else if (lua_type(L, index) == LUA_TTABLE) {
        luaL_checktype(L, index, LUA_TTABLE);
        size_t len = lua_rawlen(L, index);
        if (len < 3 || len > 4)
            luaL_error(L, "Invalid number of entries in color array. Must be an array of 3 (RGB) or 4 (RGBA) numbers.");
        int rgba[] = {0, 0, 0, 255};
        for (size_t i = 1; i <= len; ++i) {
            lua_pushnumber(L, i);
            lua_gettable(L, -2);
            if (!lua_isnumber(L, -1))
                luaL_error(L, "Invalid value in color array. Must be numeric.");
            rgba[i - 1] = lua_tonumber(L, -1);
            lua_pop(L,1);
        }
        return QColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    }  else {
        return luaL_error(L, "Invalid color. Must be a string, integer array or integer.");
    }
}
