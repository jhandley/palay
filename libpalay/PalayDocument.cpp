#include "PalayDocument.h"
#include <QTextCursor>
#include <QPrinter>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextImageFormat>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextDocumentFragment>
#include <QUrl>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <AbsoluteBlock.h>

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

Q_GUI_EXPORT extern int qt_defaultDpiX();
Q_GUI_EXPORT extern int qt_defaultDpiY();

namespace {

    // Convert from points (1/72 inch), the unit for
    // the palay API, to dots, the unit that the QTextDocument
    // measurements are in.
    float pointsToDotsX(float pts)
    {
        return pts * qt_defaultDpiX()/72.f;
    }

    float pointsToDotsY(float pts)
    {
        return pts * qt_defaultDpiY()/72.f;
    }

}
PalayDocument::PalayDocument(QObject *parent) :
    QObject(parent),
    doc_(new QTextDocument(this)),
    printer_(QPrinter::HighResolution)
{
    cursorStack_.push(QTextCursor(doc_));

    // Qt's default spacing of 2 causes screwy looking borders since there
    // is space between the borders of adjacent cells.
    tableFormat_.setCellSpacing(0);
    // Qt's default format of 2 is kinda cramped.
    tableFormat_.setCellPadding(4);
    tableFormat_.setBorderBrush(QBrush(Qt::black));
    tableFormat_.setBorder(1);
    tableFormat_.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);

    printer_.setOutputFormat(QPrinter::PdfFormat);
    printer_.setColorMode(QPrinter::Color);
    doc_->setDocumentMargin(0);

    // We set margins on the document root frame, not on
    // the printer. This way we can position absolute blocks
    // outside the margin for stuff like page numbers
    // and footnotes.
    printer_.setPageMargins(0,0,0,0,QPrinter::Millimeter);
    setPageMargins(pointsToDotsX(54),
                   pointsToDotsY(37),
                   pointsToDotsX(54),
                   pointsToDotsY(37));
    setPageSize(QPrinter::Letter);
}

PalayDocument::~PalayDocument()
{
}

int PalayDocument::paragraph(lua_State *L)
{
    cursorStack_.top().insertBlock(blockFormat_);
    text(L);
    return 0;
}

int PalayDocument::text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 2);
    cursorStack_.top().insertText(QString::fromUtf8(text), charFormat_);
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
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
        if (!lua_isstring(L, -2))
            luaL_error(L, "Invalid key in style table. All style keys must be strings.");
        const char *key = lua_tostring(L, -2);
        if (strcmp(key, "font_family") == 0) {
            if (!lua_isstring(L, -1))
                luaL_error(L, "Invalid value for font_family. Must be a string.");
            charFormat_.setFontFamily(lua_tostring(L, -1));
        } else if (strcmp(key, "font_size") == 0) {
            if (!lua_isnumber(L, -1) || lua_tointeger(L, -1) <= 0)
                luaL_error(L, "Invalid value for font_size. Must be a positive number.");
            // Note that QTextDocument takes font size in points, not dots
            // even though other measurements are in dots.
            charFormat_.setFontPointSize(lua_tointeger(L, -1));
        } else if (strcmp(key, "font_style") == 0) {
            if (!lua_isnumber(L, -1) || !setFontStyle(charFormat_, lua_tointeger(L, -1)))
                luaL_error(L, "Invalid value for font_style.");
        } else if (strcmp(key, "border_width") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) <= 0)
                luaL_error(L, "Invalid value for border_width. Must be a positive number.");
            tableFormat_.setBorder(pointsToDotsX(lua_tonumber(L, -1)));
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
            charFormat_.setForeground(QBrush(color));
        } else if (strcmp(key, "text_background_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for background_color.");
            charFormat_.setBackground(QBrush(color));
        } else if (strcmp(key, "background_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for background_color.");
            blockFormat_.setBackground(QBrush(color));
        } else if (strcmp(key, "alignment") == 0) {
            Qt::Alignment align = getAlignment(L, -1);
            blockFormat_.setAlignment(align);
            tableFormat_.setAlignment(align);
        } else if (strcmp(key, "width") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) <= 0)
                luaL_error(L, "Invalid value for width. Must be a positive number.");
            tableFormat_.setWidth(pointsToDotsX(lua_tonumber(L, -1)));
        } else if (strcmp(key, "height") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) <= 0)
                luaL_error(L, "Invalid value for height. Must be a positive number.");
            tableFormat_.setHeight(pointsToDotsY(lua_tonumber(L, -1)));
        } else {
            luaL_error(L, "Invalid key in style table: %s", key);
        }
        lua_pop(L, 1);
    }

    return 0;
}

int PalayDocument::saveAs(lua_State *L)
{
    const char *path = luaL_checkstring(L, 2);
    printer_.setOutputFileName(QString::fromUtf8(path));
    print();
    return 0;
}

int PalayDocument::startTable(lua_State *L)
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
        luaL_error(L, "endTable called with no matching call to startTable()");

    cursorStack_.pop();
    // Saved cursor is in the parent frame of table so moving to last position
    // in that frame moves past end of table.
    cursorStack_.top() = cursorStack_.top().currentFrame()->lastCursorPosition();
    return 0;
}

int PalayDocument::pageBreak(lua_State *L)
{
    Q_UNUSED(L);
    QTextBlockFormat breakBlock;
    breakBlock.setPageBreakPolicy(QTextBlockFormat::PageBreak_AlwaysAfter);
    cursorStack_.top().insertBlock(breakBlock);
    return 0;
}

int PalayDocument::image(lua_State *L)
{
    QString name = QString::fromUtf8(luaL_checkstring(L, 2));
    QImage im;

    // Check to see if this literal SVG or a filename
    QRegExp svgExp("\\s*(<\\?xml.*\\?>\\s*)?<svg.*<\\/svg>\\s*");
    if (svgExp.exactMatch(name)) {
        if (!im.loadFromData(name.toUtf8(), "svg"))
            luaL_error(L, "Error parsing SVG literal");
    } else if (!im.load(name))
        luaL_error(L, "Failed to load image from file %s", qPrintable(name));

    QString imageResourceName = QString::number(im.serialNumber());
    doc_->addResource(QTextDocument::ImageResource, QUrl(imageResourceName), im);

    QTextImageFormat imageFormat;
    imageFormat.setName(imageResourceName);
    if (lua_gettop(L) >= 3)
        imageFormat.setWidth(pointsToDotsX(luaL_checkinteger(L, 3)));
    if (lua_gettop(L) >= 4)
        imageFormat.setHeight(pointsToDotsY(luaL_checkinteger(L, 4)));

    cursorStack_.top().insertImage(imageFormat);

    return 0;
}

int PalayDocument::html(lua_State *L)
{
    QString htmlText = QString::fromUtf8(luaL_checkstring(L, 2));
    QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(htmlText);
    cursorStack_.top().insertFragment(fragment);
    return 0;
}

int PalayDocument::pageSize(lua_State *L)
{
    QString sizeString = QString(luaL_checkstring(L, 2)).toUpper();
    QPrinter::PaperSize size;
    if (sizeString == "A4")
        size = QPrinter::A4;
    else if (sizeString == "LETTER")
        size = QPrinter::Letter;
    else
        luaL_error(L, "\"%s\" is not a valid page size. Try \"Letter\" or \"A4\".", qPrintable(sizeString));

    setPageSize(size);

    return 0;
}

int PalayDocument::pageMargins(lua_State *L)
{
    setPageMargins(pointsToDotsX(luaL_checkinteger(L, 2)),
                   pointsToDotsY(luaL_checkinteger(L, 3)),
                   pointsToDotsY(luaL_checkinteger(L, 4)),
                   pointsToDotsY(luaL_checkinteger(L, 5)));
    return 0;
}

int PalayDocument::getPageWidth(lua_State *L)
{
    lua_pushnumber(L, printer_.pageRect(QPrinter::Point).width());
    return 1;
}

int PalayDocument::getPageHeight(lua_State *L)
{
    lua_pushnumber(L, printer_.pageRect(QPrinter::Point).height());
    return 1;
}

int PalayDocument::getPageCount(lua_State *L)
{
    lua_pushinteger(L, doc_->pageCount());
    return 1;
}

int PalayDocument::startBlock(lua_State *L)
{
    float x = pointsToDotsX(luaL_checkinteger(L, 2));
    float y = pointsToDotsY(luaL_checkinteger(L, 3));
    AbsoluteBlock *block = new AbsoluteBlock(QPointF(x,y), this);
    absoluteBlocks_ << block;

    // Make block content word wrap
    block->document()->setTextWidth(doc_->pageSize().width() - x);

    QTextCursor blockCursor(block->document());
    blockCursor.setBlockFormat(blockFormat_);
    blockCursor.setCharFormat(charFormat_);
    cursorStack_.push(blockCursor);

    return 0;
}

int PalayDocument::endBlock(lua_State *L)
{
    if (cursorStack_.top().document() == doc_)
        luaL_error(L, "endBlock called with no matching call to startBlock()");

    cursorStack_.pop();
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

Qt::Alignment PalayDocument::getAlignment(lua_State *L, int index)
{
    int alignment = luaL_checkinteger(L, index);
    Qt::Alignment result = 0;
    if (alignment & Left)
        result |= Qt::AlignLeft;
    if (alignment & Right)
        result |= Qt::AlignRight;
    if (alignment & HCenter)
        result |= Qt::AlignHCenter;
    if (alignment & Top)
        result |= Qt::AlignTop;
    if (alignment & Bottom)
        result |= Qt::AlignBottom;
    if (alignment & VCenter)
        result |= Qt::AlignVCenter;
    return result;
}

void PalayDocument::setPageSize(QPrinter::PaperSize size)
{
    printer_.setPaperSize(size);

    // Need to set document page size to match printer page size so that document
    // gets paginated and the pageCount() method will work correctly.
    doc_->setPageSize(QSizeF(printer_.pageRect(QPrinter::Inch).width() * qt_defaultDpiX(),
                             printer_.pageRect(QPrinter::Inch).height() * qt_defaultDpiY()));
}

void PalayDocument::setPageMargins(float left, float top, float right, float bottom)
{
    QTextFrameFormat rootFormat = doc_->rootFrame()->frameFormat();
    rootFormat.setLeftMargin(left);
    rootFormat.setTopMargin(top);
    rootFormat.setRightMargin(right);
    rootFormat.setBottomMargin(bottom);
    doc_->rootFrame()->setFrameFormat(rootFormat);

}

void PalayDocument::print()
{
    QPainter painter(&printer_);

    // Scale to printer dpi
    const qreal dpiScaleX = qreal(printer_.logicalDpiX()) / qt_defaultDpiX();
    const qreal dpiScaleY = qreal(printer_.logicalDpiY()) / qt_defaultDpiY();
    painter.scale(dpiScaleX, dpiScaleY);

    qreal pageWidth = doc_->pageSize().width();
    qreal pageHeight = doc_->pageSize().height();
    QAbstractTextDocumentLayout *layout = doc_->documentLayout();

    for (int pageNumber = 1; pageNumber <= doc_->pageCount(); ++pageNumber) {

        painter.save();
        QRect view(0, (pageNumber - 1) * pageHeight, pageWidth, pageHeight);
        painter.translate(0, -view.top());
        QAbstractTextDocumentLayout::PaintContext ctx;
        painter.setClipRect(view);
        ctx.clip = view;

        layout->draw(&painter, ctx);

        drawAbsoluteBlocks(&painter, view);

        painter.restore();
        if (pageNumber != doc_->pageCount())
            printer_.newPage();
    }
    painter.end();
}

void PalayDocument::drawAbsoluteBlocks(QPainter *painter, const QRectF &view)
{
    foreach (AbsoluteBlock *block, absoluteBlocks_) {
        QRectF blockBounds = block->bounds();
        if (view.intersects(blockBounds)) {
            block->draw(painter);
        }
    }
}
