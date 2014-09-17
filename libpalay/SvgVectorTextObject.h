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
