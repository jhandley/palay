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
