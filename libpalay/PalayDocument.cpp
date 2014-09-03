#include "PalayDocument.h"
#include <QTextCursor>
#include <QPrinter>

PalayDocument::PalayDocument(QObject *parent) :
    QObject(parent),
    doc_(new QTextDocument(this)),
    cursor_(new QTextCursor(doc_))
{
}

PalayDocument::~PalayDocument()
{
    delete cursor_;
}

void PalayDocument::paragraph(const QString &text)
{
    cursor_->insertText(text);
}

void PalayDocument::toPDF(const QString &path)
{
    QPrinter pdfPrinter(QPrinter::HighResolution);
    pdfPrinter.setOutputFileName(path);
    pdfPrinter.setOutputFormat(QPrinter::PdfFormat);
    pdfPrinter.setColorMode(QPrinter::Color);

    doc_->print(&pdfPrinter);
}
