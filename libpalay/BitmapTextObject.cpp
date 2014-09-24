#include "BitmapTextObject.h"
#include <QPainter>

Q_GUI_EXPORT extern int qt_defaultDpiX();
Q_GUI_EXPORT extern int qt_defaultDpiY();

/*!
    \class BitmapTextObject
    \brief The BitmapTextObject class is used to insert a bitmap image as a custom object in a QTextDocument without converting to a QPixmap first.

    This class is only useful if you want to avoid enabling GUI code in the application.
    The default QTextDocument can handle bitmap images, but will convert them to QPixmaps
    which forces window manager interactions.
 */

BitmapTextObject::BitmapTextObject(const QImage &image, float width, float height, QObject *parent) :
    QObject(parent),
    size_(width, height),
    image_(image)
{
    if (width <= 0 && height <= 0) {
        // no height or width use default size
        size_.setWidth(image.width());
        size_.setHeight(image.height());
    } else if (width <= 0) {
        // use height and aspect ratio to compute width
        size_.setWidth(height * image.width() / image.height());
    } else if (height <= 0) {
        // use width and aspect ratio to compute height
        size_.setHeight(width * image.height() / image.width());
    }
}

bool BitmapTextObject::isValid() const
{
    return !image_.isNull();
}

QSizeF BitmapTextObject::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument);
    Q_UNUSED(format);

    int dpiX;
    int dpiY;
    QPaintDevice *device = doc->documentLayout()->paintDevice();
    if (device) {
        dpiX = device->logicalDpiX();
        dpiY = device->logicalDpiY();
    } else {
        dpiX = qt_defaultDpiX();
        dpiY = qt_defaultDpiY();
    }
    return QSizeF(dpiX * size_.width()/72.f, dpiY * size_.height()/72.f);
}

void BitmapTextObject::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument);
    Q_UNUSED(doc);
    Q_UNUSED(format);

    painter->drawImage(rect, image_);
}
