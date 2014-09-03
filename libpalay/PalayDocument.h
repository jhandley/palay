#ifndef PALAYDOCUMENT_H
#define PALAYDOCUMENT_H

#include <QObject>
#include <QTextDocument>
 #include <QTextCharFormat>

class PalayDocument : public QObject
{
    Q_OBJECT
public:
    explicit PalayDocument(QObject *parent = 0);
    ~PalayDocument();

    void paragraph(const QString &text);
    void toPDF(const QString &path);

signals:

public slots:

private:
    QTextDocument *doc_;
    QTextCursor *cursor_;
};

#endif // PALAYDOCUMENT_H
