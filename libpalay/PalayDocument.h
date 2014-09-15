#ifndef PALAYDOCUMENT_H
#define PALAYDOCUMENT_H

#include <QObject>
#include <QTextDocument>
#include <QTextTableFormat>
#include <QStack>
#include <QPrinter>

struct lua_State;
class QTextCharFormat;
class AbsoluteBlock;

class PalayDocument : public QObject
{
    Q_OBJECT
public:
    explicit PalayDocument(QObject *parent = 0);
    ~PalayDocument();

    int paragraph(lua_State *L);
    int text(lua_State *L);
    int style(lua_State *L);
    int saveAs(lua_State *L);

    int startTable(lua_State *L);
    int cell(lua_State *L);
    int endTable(lua_State *L);

    int pageBreak(lua_State *L);
    int image(lua_State *L);
    int html(lua_State *L);

    int pageSize(lua_State *L);
    int getPageWidth(lua_State *L);
    int getPageHeight(lua_State *L);

    int startBlock(lua_State *L);
    int endBlock(lua_State *L);

    enum FontStyle {
        Normal = 0,
        Bold = 1,
        Italic = 2,
        Underline = 4
    };

    enum BorderStyle {
        None = 0,
        Dotted = 1,
        Dashed = 2,
        Solid = 3
    };

    enum Alignment {
        Left = 1,
        Right = 2,
        HCenter = 4,
        Justify = 8,
        Top = 16,
        Bottom = 32,
        VCenter = 64,
        Center = 128
    };

signals:

public slots:

private:

    bool setFontStyle(QTextCharFormat &format, int style);
    bool setBorderStyle(QTextTableFormat &format, int style);
    QColor getColor(lua_State *L, int index);
    Qt::Alignment getAlignment(lua_State *L, int index);
    void setPageSize(QPrinter::PaperSize size);

    void print();
    void drawAbsoluteBlocks(QPainter *painter, int pageNumber);

    QTextDocument *doc_;
    QStack<QTextCursor> cursorStack_;
    QTextTableFormat tableFormat_;
    QTextBlockFormat blockFormat_;
    QTextCharFormat charFormat_;
    QPrinter printer_;
    QList<AbsoluteBlock*> absoluteBlocks_;
};

#endif // PALAYDOCUMENT_H
