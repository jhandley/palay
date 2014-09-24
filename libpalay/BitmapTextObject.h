#ifndef BITMAPTEXTOBJECT_H
#define BITMAPTEXTOBJECT_H

#include <QObject>
#include <QTextObjectInterface>
#include <QImage>

class BitmapTextObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit BitmapTextObject(const QImage &image, float width = -1, float height = -1, QObject *parent = 0);

    bool isValid() const;

    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument,
                         const QTextFormat &format);

    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc,
                    int posInDocument, const QTextFormat &format);

private:
    QSizeF size_;
    QImage image_;
};


#endif // BITMAPTEXTOBJECT_H
