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
    QAbstractTextDocumentLayout *layout = document_->documentLayout();
    painter->save();
    painter->translate(QPointF(position_.x(), position_.y()));
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.clip = painter->clipBoundingRect();
    layout->draw(painter, ctx);
    painter->restore();
}
