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
