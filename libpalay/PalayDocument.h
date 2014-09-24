/*
 * Copyright 2014 LKC Technologies, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PALAYDOCUMENT_H
#define PALAYDOCUMENT_H

#include <QObject>
#include <QTextDocument>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextTableFormat>
#include <QTextTableCellFormat>
#include <QStack>
#include <QPrinter>

struct lua_State;
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
    int pushStyle(lua_State *L);
    int popStyle(lua_State *L);
    int saveAs(lua_State *L);

    int startTable(lua_State *L);
    int cell(lua_State *L);
    int endTable(lua_State *L);

    int pageBreak(lua_State *L);
    int image(lua_State *L);
    int svg(lua_State *L);
    int html(lua_State *L);

    int pageSize(lua_State *L);
    int pageMargins(lua_State *L);
    int getPageWidth(lua_State *L);
    int getPageHeight(lua_State *L);
    int getPageMargins(lua_State *L);
    int getPageCount(lua_State *L);

    int startBlock(lua_State *L);
    int endBlock(lua_State *L);

private:
    void setFontStyle(lua_State *L, QTextCharFormat &format, int index);
    QTextFrameFormat::BorderStyle getBorderStyle(lua_State *L, int index);
    QColor getColor(lua_State *L, int index);
    Qt::Alignment getAlignment(lua_State *L, int index);
    Qt::Corner getCorner(lua_State *L, int index);
    void setPageSize(QPrinter::PaperSize size);
    void setPageMargins(float left, float top, float right, float bottom);
    void insertBitmapImage(lua_State *L, const QString &filename, float widthPts, float heightPts);
    void insertSvgImage(lua_State *L, const QString &svg, float widthPts, float heightPts);
    void print();
    void drawAbsoluteBlocks(QPainter *painter, const QRectF &view);

    void dump();

    struct Formats {
        QTextTableFormat table_;
        QTextBlockFormat block_;
        QTextCharFormat char_;
        QTextFrameFormat frame_;
        QTextTableCellFormat cell_;
    };

    QTextDocument *doc_;
    QStack<QTextCursor> cursorStack_;
    QPrinter printer_;
    QList<AbsoluteBlock*> absoluteBlocks_;
    QStack<Formats> formatStack_;
    int nextCustomObjectType_;
};

#endif // PALAYDOCUMENT_H
