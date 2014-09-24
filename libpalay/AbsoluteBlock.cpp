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

#include "AbsoluteBlock.h"
#include <QAbstractTextDocumentLayout>
#include <QPainter>

AbsoluteBlock::AbsoluteBlock(Qt::Corner corner, const QPointF& pos, const QSizeF &parentSize, QObject *parent) :
    QObject(parent),
    document_(new QTextDocument(this)),
    corner_(corner),
    position_(pos)
{
    // Make text line wrapping work correctly
    switch (corner_) {
    case Qt::TopLeftCorner:
    case Qt::BottomLeftCorner:
        document_->setTextWidth(parentSize.width() - pos.x());
        break;
    case Qt::TopRightCorner:
    case Qt::BottomRightCorner:
        document_->setTextWidth(pos.x());
    default:
        break;
    }
}

QTextDocument *AbsoluteBlock::document()
{
    return document_;
}

QPointF AbsoluteBlock::absolutePosition()
{
    qreal width = document_->idealWidth();
    qreal height = document_->size().height();

    QPointF pos = position_;
    switch (corner_) {
    case Qt::TopLeftCorner:
        break;
    case Qt::TopRightCorner:
        pos.rx() -= width;
        break;
    case Qt::BottomLeftCorner:
        pos.ry() -= height;
        break;
    case Qt::BottomRightCorner:
        pos.rx() -= width;
        pos.ry() -= height;
        break;
    }
    return pos;
}

QRectF AbsoluteBlock::bounds()
{
    return QRectF(absolutePosition(), document_->size());
}

void AbsoluteBlock::draw(QPainter *painter)
{
    QAbstractTextDocumentLayout *layout = document_->documentLayout();
    painter->save();
    painter->translate(absolutePosition());
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.clip = painter->clipBoundingRect();
    layout->draw(painter, ctx);
    painter->restore();
}
