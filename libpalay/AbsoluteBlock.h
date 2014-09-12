// Copyright (c) 2011-2014 LKC Technologies, Inc.  All rights reserved.
// LKC Technologies, Inc. PROPRIETARY AND CONFIDENTIAL

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
    explicit AbsoluteBlock(QObject *parent = 0);

    QRectF bounds();

    void draw(QPainter *painter);

signals:

public slots:

private:

    QTextDocument *content_;
    QPointF position_;
    AbsoluteBlock *parent_;

};

#endif // ABSOLUTEBLOCK_H
