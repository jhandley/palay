#include "PalayDocument.h"
#include <QTextCursor>
#include <QPrinter>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextImageFormat>
#include <QTextFrameFormat>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextDocumentFragment>
#include <QUrl>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <AbsoluteBlock.h>
#include "SvgVectorTextObject.h"

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

    float dotsToPointsX(float dots)
    {
        return dots * 72.f/qt_defaultDpiX();
    }

    float dotsToPointsY(float dots)
    {
        return dots * 72.f/qt_defaultDpiY();
    }

    QRegExp whitespaceOrComma("(\\s*,\\s*)|\\s+");

}
PalayDocument::PalayDocument(QObject *parent) :
    QObject(parent),
    doc_(new QTextDocument(this)),
    printer_(QPrinter::HighResolution),
    nextCustomObjectType_(QTextFormat::UserObject + 1)
{
    Formats defaultFormat;

    defaultFormat.char_.setFontFamily("DejaVuSans");
    defaultFormat.char_.setFontPointSize(12);
    defaultFormat.char_.setFontWeight(QFont::Normal);
    defaultFormat.char_.setFontItalic(false);
    defaultFormat.char_.setFontUnderline(false);
    defaultFormat.char_.setForeground(QBrush(Qt::black));

    defaultFormat.block_.setAlignment(Qt::AlignLeft);

    // Qt's default spacing of 2 causes screwy looking borders since there
    // is space between the borders of adjacent cells.
    defaultFormat.table_.setCellSpacing(0);
    // We will set padding on individual cells
    defaultFormat.table_.setCellPadding(0);
    defaultFormat.table_.setBorderBrush(QBrush(Qt::black));
    defaultFormat.table_.setBorder(pointsToDotsX(1));
    defaultFormat.table_.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    defaultFormat.table_.setPadding(0);
    defaultFormat.table_.setMargin(0);
    defaultFormat.frame_.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    defaultFormat.frame_.setBorder(0);
    defaultFormat.frame_.setPadding(0);
    defaultFormat.frame_.setMargin(0);
    defaultFormat.block_.setBottomMargin(0);
    defaultFormat.block_.setTopMargin(0);
    defaultFormat.block_.setLeftMargin(0);
    defaultFormat.block_.setRightMargin(0);
    defaultFormat.block_.setBottomMargin(0);
    defaultFormat.cell_.setPadding(pointsToDotsX(4));
    formatStack_.push(defaultFormat);

    QTextCursor initialCursor(doc_);
    initialCursor.setBlockFormat(defaultFormat.block_);
    initialCursor.setBlockCharFormat(defaultFormat.char_);
    initialCursor.setCharFormat(defaultFormat.char_);
    cursorStack_.push(initialCursor);

    printer_.setOutputFormat(QPrinter::PdfFormat);
    printer_.setColorMode(QPrinter::Color);
    doc_->setDocumentMargin(0);

    // We set margins on the document root frame, not on
    // the printer. This way we can position absolute blocks
    // outside the margin for stuff like page numbers
    // and footnotes.
    setPageSize(QPrinter::Letter);
    printer_.setFullPage(true);
    printer_.setPageMargins(0,0,0,0,QPrinter::Millimeter);
    setPageMargins(pointsToDotsX(54),
                   pointsToDotsY(37),
                   pointsToDotsX(54),
                   pointsToDotsY(37));
}

PalayDocument::~PalayDocument()
{
}

int PalayDocument::paragraph(lua_State *L)
{
    QTextBlock currentBlock = cursorStack_.top().block();
    if (currentBlock.begin().atEnd()) {
        // If the current block is empty then don't add a new one, just reuse the existing one.
        // This will be the case for the first block in the document or first block in
        // a table cell or frame.
        QTextCursor tc(currentBlock);
        tc.mergeBlockFormat(formatStack_.top().block_); // merge in case the block format has page break
        tc.setBlockCharFormat(formatStack_.top().char_);
    } else {
        // Create a new block.
        cursorStack_.top().insertBlock(formatStack_.top().block_, formatStack_.top().char_);
    }

    text(L);

    return 0;
}

int PalayDocument::text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 2);
    cursorStack_.top().insertText(QString::fromUtf8(text), formatStack_.top().char_);
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
        if (qstricmp(key, "font_family") == 0) {
            if (!lua_isstring(L, -1))
                luaL_error(L, "Invalid value for font_family. Must be a string.");
            formatStack_.top().char_.setFontFamily(lua_tostring(L, -1));
        } else if (qstricmp(key, "font_size") == 0) {
            if (!lua_isnumber(L, -1) || lua_tointeger(L, -1) <= 0)
                luaL_error(L, "Invalid value for font_size. Must be a positive number.");
            // Note that QTextDocument takes font size in points, not dots
            // even though other measurements are in dots.
            formatStack_.top().char_.setFontPointSize(lua_tointeger(L, -1));
        } else if (qstricmp(key, "font_style") == 0) {
            setFontStyle(L, formatStack_.top().char_, -1);
        } else if (qstricmp(key, "border_width") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
                luaL_error(L, "Invalid value for border_width. Must be a positive number.");
            qreal border = pointsToDotsX(lua_tonumber(L, -1));
            formatStack_.top().table_.setBorder(border);
            formatStack_.top().frame_.setBorder(border);
        } else if (qstricmp(key, "cell_padding") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
                luaL_error(L, "Invalid value for cell_padding. Must be a positive number.");
            qreal padding = pointsToDotsX(lua_tonumber(L, -1));
            formatStack_.top().cell_.setPadding(padding);
        } else if (qstricmp(key, "cell_left_padding") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
                luaL_error(L, "Invalid value for cell_left_padding. Must be a positive number.");
            qreal padding = pointsToDotsX(lua_tonumber(L, -1));
            formatStack_.top().cell_.setLeftPadding(padding);
        } else if (qstricmp(key, "cell_right_padding") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
                luaL_error(L, "Invalid value for cell_right_padding. Must be a positive number.");
            qreal padding = pointsToDotsX(lua_tonumber(L, -1));
            formatStack_.top().cell_.setRightPadding(padding);
        } else if (qstricmp(key, "cell_top_padding") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
                luaL_error(L, "Invalid value for cell_top_padding. Must be a positive number.");
            qreal padding = pointsToDotsX(lua_tonumber(L, -1));
            formatStack_.top().cell_.setTopPadding(padding);
        } else if (qstricmp(key, "cell_bottom_padding") == 0) {
            if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
                luaL_error(L, "Invalid value for cell_bottom_padding. Must be a positive number.");
            qreal padding = pointsToDotsX(lua_tonumber(L, -1));
            formatStack_.top().cell_.setBottomPadding(padding);
        } else if (qstricmp(key, "border_style") == 0) {
            QTextFrameFormat::BorderStyle borderStyle = getBorderStyle(L, -1);
            formatStack_.top().table_.setBorderStyle(borderStyle);
            formatStack_.top().frame_.setBorderStyle(borderStyle);
        } else if (qstricmp(key, "border_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for text_color.");
            formatStack_.top().table_.setBorderBrush(QBrush(color));
            formatStack_.top().frame_.setBorderBrush(QBrush(color));
        } else if (qstricmp(key, "text_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for text_color.");
            formatStack_.top().char_.setForeground(QBrush(color));
        } else if (qstricmp(key, "text_background_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for background_color.");
            formatStack_.top().char_.setBackground(QBrush(color));
        } else if (qstricmp(key, "background_color") == 0) {
            QColor color = getColor(L, -1);
            if (!color.isValid())
                luaL_error(L, "Invalid color for background_color.");
            formatStack_.top().block_.setBackground(QBrush(color));
        } else if (qstricmp(key, "alignment") == 0) {
            Qt::Alignment align = getAlignment(L, -1);
            formatStack_.top().block_.setAlignment(align);
            formatStack_.top().table_.setAlignment(align);
        } else if (qstricmp(key, "width") == 0) {
            if (lua_isnumber(L, -1) && lua_tonumber(L, -1) >= 0) {
                qreal widthDots = pointsToDotsX(lua_tonumber(L, -1));
                formatStack_.top().table_.setWidth(widthDots);
                formatStack_.top().frame_.setWidth(widthDots);
            } else if (lua_isstring(L, -1) && (qstricmp(lua_tostring(L, -1), "variable") == 0)) {
                formatStack_.top().table_.setWidth(QTextLength());
                formatStack_.top().frame_.setWidth(QTextLength());
            } else if (lua_isstring(L, -1) && (qstricmp(lua_tostring(L, -1), "page") == 0)) {
                qreal widthDots = doc_->pageSize().width();
                formatStack_.top().table_.setWidth(widthDots);
                formatStack_.top().frame_.setWidth(widthDots);
            } else if (lua_isstring(L, -1) && (qstricmp(lua_tostring(L, -1), "inside_page") == 0)) {
                QTextFrameFormat rootFormat = doc_->rootFrame()->frameFormat();
                qreal widthDots = doc_->pageSize().width() - rootFormat.leftMargin() - rootFormat.rightMargin();
                formatStack_.top().table_.setWidth(widthDots);
                formatStack_.top().frame_.setWidth(widthDots);
            } else {
                luaL_error(L, "Invalid value for width. Must be a positive number, \"page\", \"inside_page\", or \"variable\".");
            }
        } else if (qstricmp(key, "height") == 0) {
            if (lua_isnumber(L, -1) && lua_tonumber(L, -1) >= 0) {
                qreal heightDots = pointsToDotsY(lua_tonumber(L, -1));
                formatStack_.top().table_.setHeight(heightDots);
                formatStack_.top().frame_.setHeight(heightDots);
            } else if (lua_isstring(L, -1) && (qstricmp(lua_tostring(L, -1), "variable") == 0)) {
                formatStack_.top().table_.setHeight(QTextLength());
                formatStack_.top().frame_.setHeight(QTextLength());
            } else if (lua_isstring(L, -1) && (qstricmp(lua_tostring(L, -1), "page") == 0)) {
                qreal heightDots = doc_->pageSize().height();
                formatStack_.top().table_.setHeight(heightDots);
                formatStack_.top().frame_.setHeight(heightDots);
            } else if (lua_isstring(L, -1) && (qstricmp(lua_tostring(L, -1), "inside_page") == 0)) {
                QTextFrameFormat rootFormat = doc_->rootFrame()->frameFormat();
                qreal heightDots = doc_->pageSize().height() - rootFormat.topMargin() - rootFormat.bottomMargin();
                formatStack_.top().table_.setHeight(heightDots);
                formatStack_.top().frame_.setHeight(heightDots);
            } else {
                luaL_error(L, "Invalid value for height. Must be a positive number, \"page\", \"inside_page\", or \"variable\".");
            }
        } else if (qstricmp(key, "indent") == 0) {
           if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
               luaL_error(L, "Invalid value for indent. Must be a positive number.");
           formatStack_.top().block_.setIndent(lua_tointeger(L, -1));
        } else if (qstricmp(key, "left_margin") == 0) {
           if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
               luaL_error(L, "Invalid value for left_margin. Must be a positive number.");
           formatStack_.top().block_.setLeftMargin(pointsToDotsX(lua_tonumber(L, -1)));
        } else if (qstricmp(key, "right_margin") == 0) {
           if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
               luaL_error(L, "Invalid value for right_margin. Must be a positive number.");
           formatStack_.top().block_.setRightMargin(pointsToDotsX(lua_tonumber(L, -1)));
        } else if (qstricmp(key, "top_margin") == 0) {
           if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
               luaL_error(L, "Invalid value for top_margin. Must be a positive number.");
           formatStack_.top().block_.setTopMargin(pointsToDotsY(lua_tonumber(L, -1)));
        } else if (qstricmp(key, "bottom_margin") == 0) {
           if (!lua_isnumber(L, -1) || lua_tonumber(L, -1) < 0)
               luaL_error(L, "Invalid value for bottom_margin. Must be a positive number.");
           formatStack_.top().block_.setBottomMargin(pointsToDotsY(lua_tonumber(L, -1)));
        } else {
            luaL_error(L, "Invalid key in style table: %s", key);
        }
        lua_pop(L, 1);
    }

    return 0;
}

int PalayDocument::pushStyle(lua_State *L)
{
    formatStack_.push(formatStack_.top());
    style(L);
    return 0;
}

int PalayDocument::popStyle(lua_State *L)
{
    if (formatStack_.size() < 2)
        luaL_error(L, "popStyle called with no matching pushStyle");
    formatStack_.pop();
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

    // Propogate all current formats to the cells of the table.
    // The call to cell() will do the same but we may not get a call
    // for each cell if some are left empty but we still want them to
    // have the right padding.
    QTextTable *table = cursorStack_.top().insertTable(rows, cols, formatStack_.top().table_);
    for (int i = 0; i < table->rows(); ++i) {
        for (int j = 0; j < table->columns(); ++j) {
            QTextTableCell cell = table->cellAt(i, j);
            QTextCursor cellCursor = cell.firstCursorPosition();
            cell.setFormat(formatStack_.top().cell_);
            cellCursor.setBlockFormat(formatStack_.top().block_);
            cellCursor.setCharFormat(formatStack_.top().char_);
        }
    }
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

    QTextTableCell tableCell = table->cellAt(row - 1, col - 1);
    QTextCursor cellCursor = tableCell.firstCursorPosition();

    // Propogate formats to cell (in case any have changed since
    // table was created).
    tableCell.setFormat(formatStack_.top().cell_);
    cellCursor.setBlockFormat(formatStack_.top().block_);
    cellCursor.setCharFormat(formatStack_.top().char_);

    // Put current cursor at start of cell.
    cursorStack_.top() = cellCursor;

    return 0;
}

int PalayDocument::endTable(lua_State *L)
{
    if (!cursorStack_.top().currentTable())
        luaL_error(L, "endTable called with no matching call to startTable()");

    cursorStack_.pop();

    // Saved cursor is in the parent frame of table so moving to last position
    // in that frame moves past end of table.
    if (cursorStack_.top().currentTable()) {
        // But if we are in a table cell (i.e. nested table) then move to the end
        // of the cell not the entire table.
        QTextTableCell currentCell = cursorStack_.top().currentTable()->cellAt(cursorStack_.top());
        cursorStack_.top() = currentCell.lastCursorPosition();
    } else {
        cursorStack_.top() = cursorStack_.top().currentFrame()->lastCursorPosition();
    }
    return 0;
}

int PalayDocument::pageBreak(lua_State *L)
{
    Q_UNUSED(L);
    QTextBlockFormat breakBlock(formatStack_.top().block_);
    breakBlock.setPageBreakPolicy(QTextBlockFormat::PageBreak_AlwaysBefore);
    cursorStack_.top().insertBlock(breakBlock);
    return 0;
}

int PalayDocument::image(lua_State *L)
{
    QString name = QString::fromUtf8(luaL_checkstring(L, 2));
    float widthPts = -1;
    float heightPts = -1;

    if (lua_gettop(L) >= 3)
        widthPts = luaL_checkinteger(L, 3);
    if (lua_gettop(L) >= 4)
        heightPts = luaL_checkinteger(L, 4);

    // Check to see if this literal SVG
    QRegExp svgExp("\\s*(<\\?xml.*\\?>\\s*)?<svg.*<\\/svg>\\s*");
    if (svgExp.exactMatch(name)) {
        insertSvgImage(L, name, widthPts, heightPts);
    } else if (name.endsWith(".svg", Qt::CaseInsensitive)) {
        QFile svgFile(name);
        if (!svgFile.open(QFile::ReadOnly))
            luaL_error(L, "Failed to open SVG file %s", qPrintable(name));
        QString svg = QString::fromUtf8(svgFile.readAll());
        insertSvgImage(L, svg, widthPts, heightPts);
    } else {
        insertBitmapImage(L, name, widthPts, heightPts);
    }

    return 0;
}

int PalayDocument::html(lua_State *L)
{
    QString htmlText = QString::fromUtf8(luaL_checkstring(L, 2));
    QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(htmlText);
    cursorStack_.top().insertFragment(fragment);
    return 0;
}

struct PageSizeLookup {
    const char *name;
    QPrinter::PageSize value;
};

#define NUM_ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))
static const PageSizeLookup nameToPageSize[] =
{
    #define STR(x)   #x
    #define XSTR(x)  STR(x)
    #define ENTRY(name) { XSTR(name), QPrinter::name }

    ENTRY(A4),
    ENTRY(B5),
    ENTRY(Letter),
    ENTRY(Legal),
    ENTRY(Executive),
    ENTRY(A0),
    ENTRY(A1),
    ENTRY(A2),
    ENTRY(A3),
    ENTRY(A5),
    ENTRY(A6),
    ENTRY(A7),
    ENTRY(A8),
    ENTRY(A9),
    ENTRY(B0),
    ENTRY(B1),
    ENTRY(B10),
    ENTRY(B2),
    ENTRY(B3),
    ENTRY(B4),
    ENTRY(B6),
    ENTRY(B7),
    ENTRY(B8),
    ENTRY(B9),
    ENTRY(C5E),
    ENTRY(Comm10E),
    ENTRY(DLE),
    ENTRY(Folio),
    ENTRY(Ledger),
    ENTRY(Tabloid),

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    // New values derived from PPD standard
    ENTRY(A10),
    ENTRY(A3Extra),
    ENTRY(A4Extra),
    ENTRY(A4Plus),
    ENTRY(A4Small),
    ENTRY(A5Extra),
    ENTRY(B5Extra),

    ENTRY(JisB0),
    ENTRY(JisB1),
    ENTRY(JisB2),
    ENTRY(JisB3),
    ENTRY(JisB4),
    ENTRY(JisB5),
    ENTRY(JisB6),
    ENTRY(JisB7),
    ENTRY(JisB8),
    ENTRY(JisB9),
    ENTRY(JisB10),

    ENTRY(AnsiA),
    ENTRY(AnsiB),
    ENTRY(AnsiC),
    ENTRY(AnsiD),
    ENTRY(AnsiE),
    ENTRY(LegalExtra),
    ENTRY(LetterExtra),
    ENTRY(LetterPlus),
    ENTRY(LetterSmall),
    ENTRY(TabloidExtra),

    ENTRY(ArchA),
    ENTRY(ArchB),
    ENTRY(ArchC),
    ENTRY(ArchD),
    ENTRY(ArchE),

    ENTRY(Imperial7x9),
    ENTRY(Imperial8x10),
    ENTRY(Imperial9x11),
    ENTRY(Imperial9x12),
    ENTRY(Imperial10x11),
    ENTRY(Imperial10x13),
    ENTRY(Imperial10x14),
    ENTRY(Imperial12x11),
    ENTRY(Imperial15x11),

    ENTRY(ExecutiveStandard),
    ENTRY(Note),
    ENTRY(Quarto),
    ENTRY(Statement),
    ENTRY(SuperA),
    ENTRY(SuperB),
    ENTRY(Postcard),
    ENTRY(DoublePostcard),
    ENTRY(Prc16K),
    ENTRY(Prc32K),
    ENTRY(Prc32KBig),

    ENTRY(FanFoldUS),
    ENTRY(FanFoldGerman),
    ENTRY(FanFoldGermanLegal),

    ENTRY(EnvelopeB4),
    ENTRY(EnvelopeB5),
    ENTRY(EnvelopeB6),
    ENTRY(EnvelopeC0),
    ENTRY(EnvelopeC1),
    ENTRY(EnvelopeC2),
    ENTRY(EnvelopeC3),
    ENTRY(EnvelopeC4),
    ENTRY(EnvelopeC5),
    ENTRY(EnvelopeC6),
    ENTRY(EnvelopeC65),
    ENTRY(EnvelopeC7),
    ENTRY(EnvelopeDL),

    ENTRY(Envelope9),
    ENTRY(Envelope10),
    ENTRY(Envelope11),
    ENTRY(Envelope12),
    ENTRY(Envelope14),
    ENTRY(EnvelopeMonarch),
    ENTRY(EnvelopePersonal),

    ENTRY(EnvelopeChou3),
    ENTRY(EnvelopeChou4),
    ENTRY(EnvelopeInvite),
    ENTRY(EnvelopeItalian),
    ENTRY(EnvelopeKaku2),
    ENTRY(EnvelopeKaku3),
    ENTRY(EnvelopePrc1),
    ENTRY(EnvelopePrc2),
    ENTRY(EnvelopePrc3),
    ENTRY(EnvelopePrc4),
    ENTRY(EnvelopePrc5),
    ENTRY(EnvelopePrc6),
    ENTRY(EnvelopePrc7),
    ENTRY(EnvelopePrc8),
    ENTRY(EnvelopePrc9),
    ENTRY(EnvelopePrc10),
    ENTRY(EnvelopeYou4)
#endif
};

int PalayDocument::pageSize(lua_State *L)
{
    const char * sizeString = luaL_checkstring(L, 2);
    QPrinter::PaperSize size = (QPrinter::PaperSize) (QPrinter::NPageSize + 1);
    for (size_t i = 0; i < NUM_ELEMENTS(nameToPageSize); i++) {
        if (qstricmp(sizeString, nameToPageSize[i].name) == 0) {
            size = nameToPageSize[i].value;
            break;
        }
    }
    if (size > QPrinter::NPageSize)
        luaL_error(L, "\"%s\" is not a valid page size. Try \"Letter\" or \"A4\".", qPrintable(sizeString));

    setPageSize(size);

    return 0;
}

int PalayDocument::pageMargins(lua_State *L)
{
    setPageMargins(pointsToDotsX(luaL_checkinteger(L, 2)),
                   pointsToDotsY(luaL_checkinteger(L, 3)),
                   pointsToDotsX(luaL_checkinteger(L, 4)),
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

int PalayDocument::getPageMargins(lua_State *L)
{
    QTextFrameFormat rootFormat = doc_->rootFrame()->frameFormat();
    lua_pushnumber(L, dotsToPointsX(rootFormat.leftMargin()));
    lua_pushnumber(L, dotsToPointsY(rootFormat.topMargin()));
    lua_pushnumber(L, dotsToPointsX(rootFormat.rightMargin()));
    lua_pushnumber(L, dotsToPointsY(rootFormat.bottomMargin()));
    return 4;
}

int PalayDocument::getPageCount(lua_State *L)
{
    lua_pushinteger(L, doc_->pageCount());
    return 1;
}

int PalayDocument::startBlock(lua_State *L)
{
    Qt::Corner corner = getCorner(L, 2);
    float x = pointsToDotsX(luaL_checkinteger(L, 3));
    float y = pointsToDotsY(luaL_checkinteger(L, 4));
    AbsoluteBlock *block = new AbsoluteBlock(corner, QPointF(x,y), doc_->pageSize(), this);
    absoluteBlocks_ << block;

    block->document()->rootFrame()->setFrameFormat(formatStack_.top().frame_);
    QTextCursor blockCursor(block->document());
    blockCursor.setBlockFormat(formatStack_.top().block_);
    blockCursor.setCharFormat(formatStack_.top().char_);
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

void PalayDocument::setFontStyle(lua_State *L, QTextCharFormat &format, int index)
{
    QString styleString = luaL_checkstring(L, index);
    QStringList styles = QString(styleString).split(whitespaceOrComma);
    if (styles.isEmpty())
        luaL_error(L, "\"%s\" is not a valid font style. Try a comma or space seperated list of values like: \"Bold,Italic\" or \"Bold Underline\"", qPrintable(styleString));
    format.setFontWeight(QFont::Normal);
    format.setFontItalic(false);
    format.setFontUnderline(false);
    foreach (QString s, styles) {
        if (s.compare("Bold", Qt::CaseInsensitive) == 0)
            format.setFontWeight(QFont::Bold);
        else if (s.compare("Italic", Qt::CaseInsensitive) == 0)
            format.setFontItalic(true);
        else if (s.compare("Underline", Qt::CaseInsensitive) == 0)
            format.setFontUnderline(true);
        else if (s.compare("Normal", Qt::CaseInsensitive) != 0) {
            luaL_error(L, "\"%s\" is not a valid font style. Try \"Normal\", \"Bold\", \"Italic\", \"Underline\" or a combination thereof.", qPrintable(s));
        }
    }
}

QTextFrameFormat::BorderStyle PalayDocument::getBorderStyle(lua_State *L, int index)
{
    const char* style = luaL_checkstring(L, index);
    if (qstricmp(style, "None") == 0)
        return QTextFrameFormat::BorderStyle_None;
    else if (qstricmp(style, "Dotted") == 0)
        return QTextFrameFormat::BorderStyle_Dotted;
    else if (qstricmp(style, "Dashed") == 0)
        return QTextFrameFormat::BorderStyle_Dashed;
    else if (qstricmp(style, "Solid") == 0)
        return QTextFrameFormat::BorderStyle_Solid;
    else
        return (QTextFrameFormat::BorderStyle) luaL_error(L, "Invalid value for border_style.");
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
    QString alignment = luaL_checkstring(L, index);
    QStringList alignments = QString(alignment).split(whitespaceOrComma);
    if (alignment.isEmpty())
        luaL_error(L, "\"%s\" is not a valid alignment. Try a comma or space seperated list of values like: \"Top Left\" or \"Top,HCenter\"", qPrintable(alignment));
    Qt::Alignment result = 0;
    foreach (QString a, alignments) {
        if (a.compare("Left", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignLeft;
        else if (a.compare("Right", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignRight;
        else if (a.compare("HCenter", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignHCenter;
        else if (a.compare("Top", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignTop;
        else if (a.compare("Bottom", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignBottom;
        else if (a.compare("VCenter", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignVCenter;
        else if (a.compare("Right", Qt::CaseInsensitive) == 0)
            result |= Qt::AlignRight;
        else
            luaL_error(L, "\"%s\" is not a value for alignment. Try \"Left\", \"Right\", \"HCenter\", \"Top\", \"Bottom\" or ", qPrintable(a));
    }

    return result;
}

Qt::Corner PalayDocument::getCorner(lua_State *L, int index)
{
    const char *cornerString = luaL_checkstring(L, index);
    if (qstricmp(cornerString, "TopLeft") == 0)
        return Qt::TopLeftCorner;
    else if (qstricmp(cornerString, "TopRight") == 0)
        return Qt::TopRightCorner;
    else if (qstricmp(cornerString, "BottomLeft") == 0)
        return Qt::BottomLeftCorner;
    else if (qstricmp(cornerString, "BottomRight") == 0)
        return Qt::BottomRightCorner;
    else
        return (Qt::Corner) luaL_error(L, "%s is not a valid corner. Try \"TopLeft\", \"TopRight\", \"BottomLeft\" or \"BottomRight\"", qPrintable(cornerString));
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

void PalayDocument::insertBitmapImage(lua_State *L, const QString &filename, float widthPts, float heightPts)
{
    // Filename - load as a bitmap
    QImage im;
    if (!im.load(filename))
        luaL_error(L, "Failed to load image from file %s", qPrintable(filename));

    QString imageResourceName = QString::number(im.cacheKey());
    cursorStack_.top().document()->addResource(QTextDocument::ImageResource, QUrl(imageResourceName), im);

    QTextImageFormat imageFormat;
    imageFormat.setName(imageResourceName);
    if (widthPts >= 0)
        imageFormat.setWidth(pointsToDotsX(widthPts));
    if (heightPts >= 0)
        imageFormat.setHeight(pointsToDotsX(heightPts));

    cursorStack_.top().insertImage(imageFormat);
}

void PalayDocument::insertSvgImage(lua_State *L, const QString &svg, float widthPts, float heightPts)
{
    SvgVectorTextObject *svgTextFormatInterface = new SvgVectorTextObject(svg, widthPts, heightPts, this);
    if (!svgTextFormatInterface->isValid())
        luaL_error(L, "Error parsing SVG");

    int objectType = nextCustomObjectType_++;
    cursorStack_.top().document()->documentLayout()->registerHandler(objectType, svgTextFormatInterface);
    QTextCharFormat format;
    format.setObjectType(objectType);
    cursorStack_.top().insertText(QString(QChar::ObjectReplacementCharacter), format);
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

void PalayDocument::dump()
{
    QTextBlock currentBlock = doc_->begin();

    while (currentBlock.isValid()) {
        qDebug("Block %d:", currentBlock.blockNumber());
        QTextBlock::iterator it;
        for (it = currentBlock.begin(); !(it.atEnd()); ++it) {
            QTextFragment currentFragment = it.fragment();
            if (currentFragment.isValid()) {
                qDebug("\tFragment: \"%s\" (%s)", qPrintable(currentFragment.text()),
                                                  qPrintable(currentFragment.charFormat().font().toString()));
            } else {
                qDebug("\tInvalid fragment");
            }
        }
        currentBlock = currentBlock.next();
    }
}
