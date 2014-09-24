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

#ifndef SVGTEXTOBJECT_H
#define SVGTEXTOBJECT_H

#include <QObject>
#include <QTextObjectInterface>

class QSvgRenderer;

class SvgVectorTextObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit SvgVectorTextObject(const QString &svg, float width = -1, float height = -1, QObject *parent = 0);

    bool isValid() const;

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument,
                         const QTextFormat &format);

    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc,
                    int posInDocument, const QTextFormat &format);

signals:

public slots:

private:
    QSizeF size_;
    QSvgRenderer *renderer_;
};

#endif // SVGTEXTOBJECT_H
