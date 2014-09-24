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

#ifndef ABSOLUTEBLOCK_H
#define ABSOLUTEBLOCK_H

#include <QObject>
#include <QTextDocument>
#include <QPointF>

class QPainter;

class AbsoluteBlock : public QObject
{
    Q_OBJECT
public:

    explicit AbsoluteBlock(Qt::Corner corner, const QPointF& pos, const QSizeF &parentSize, QObject *parent = 0);

    QTextDocument *document();

    QPointF absolutePosition();

    QRectF bounds();

    void draw(QPainter *painter);

signals:

public slots:

private:

    QTextDocument *document_;
    Qt::Corner corner_;
    QPointF position_;
    AbsoluteBlock *parent_;
};

#endif // ABSOLUTEBLOCK_H
