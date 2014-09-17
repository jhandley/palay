#include "SvgVectorTextObject.h"
#include <QSvgRenderer>

Q_GUI_EXPORT extern int qt_defaultDpiX();
Q_GUI_EXPORT extern int qt_defaultDpiY();

/*!
    \class SvgVectorTextObject
    \brief The SvgVectorTextObject class is used to insert an SVG vector image as a custom object in a QTextDocument without rendering it to bitmap.

    To use the SVGTextObject class construct a SVGTextObject passing the SVG text and the size (in points)
    that the plot should occupy in the document (it will be scaled to fit this size).

        SVGTextObject mySVGTextObject = new SVGTextObject(myPlot, QSizeF(72,72));

    Then register a handler for the custom object:

        const int myObjectType = QTextFormat::UserObject + 1;
        myDoc->documentLayout()->registerHandler(myObjectType, mySVGTextObject);

    where myObject type is an integer greater than QTextFormat::UserObject that is unique to this
    instance of SVGTextObject in myDoc.

    Next create a create QTextCharFormat to hold the image in the document and set it's object type
    to myObjectType.

        QTextCharFormat myFormat;
        myFormat.setObjectType(myObjectType);

    Finally insert the QTextCharFormat into the document:

        QTextCursor cursor(myDoc);
        cursor.insertText(QString(QChar::ObjectReplacementCharacter), myFormat);

    Note that this is great for printed documents since the SVG will be drawn as vectors
    and will look nice at high DPI, however for an on-screen document, this is will be horribly
    ineffecient since the SVG will constantly get redrawn.  For that it would be better cache
    the rendered SVG in a QImage (the second example below does this).

    For details on custom text objects in QTextDocument see:
        http://qt-project.org/faq/answer/how_can_i_add_a_non-resource_image_to_a_qtextdocument
        http://qt-project.org/doc/qt-4.8/richtext-textobject.html
 */

SvgVectorTextObject::SvgVectorTextObject(const QString &svg, float width, float height, QObject *parent) :
    QObject(parent),
    size_(width, height),
    renderer_(new QSvgRenderer(svg.toUtf8(), this))
{
    if (renderer_->isValid()) {
        if (width < 0 && height < 0) {
            // no height or width use default size
            size_ = renderer_->defaultSize();
        } else if (width < 0) {
            // use height and aspect ratio to compute width
            size_.rwidth() = height * renderer_->defaultSize().width()/renderer_->defaultSize().height();
        } else if (height < 0) {
            // use width and aspect ratio to compute height
            size_.rheight() = width * renderer_->defaultSize().height()/renderer_->defaultSize().width();
        }
    }
}

bool SvgVectorTextObject::isValid() const
{
    return renderer_->isValid();
}

QSizeF SvgVectorTextObject::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
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

void SvgVectorTextObject::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument);
    Q_UNUSED(doc);
    Q_UNUSED(format);

    renderer_->render(painter, rect);
}
