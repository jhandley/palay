// Copyright (c) 2011-2014 LKC Technologies, Inc.  All rights reserved.
// LKC Technologies, Inc. PROPRIETARY AND CONFIDENTIAL

#include "AbsoluteBlock.h"
#include <QAbstractTextDocumentLayout>
#include <QPainter>

AbsoluteBlock::AbsoluteBlock(const QPointF& pos, QObject *parent) :
    QObject(parent),
    document_(new QTextDocument(this)),
    position_(pos)
{
}

QTextDocument *AbsoluteBlock::document()
{
    return document_;
}

QRectF AbsoluteBlock::bounds()
{
    return QRectF(position_, document_->size());
}

void AbsoluteBlock::draw(QPainter *painter)
{
    document_->documentLayout()->setPaintDevice(painter->device()); // make sure we are on same DPI as printer
    painter->save();
    painter->translate(position_);
    document_->drawContents(painter);
    painter->restore();
}
