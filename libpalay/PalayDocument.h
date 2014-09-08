#ifndef PALAYDOCUMENT_H
#define PALAYDOCUMENT_H

#include <QObject>
#include <QTextDocument>

struct lua_State;
class QTextCharFormat;

class PalayDocument : public QObject
{
    Q_OBJECT
public:
    explicit PalayDocument(QObject *parent = 0);
    ~PalayDocument();

    int paragraph(lua_State *L);
    int text(lua_State *L);
    int style(lua_State *L);
    int saveAs(lua_State *L);

    enum FontStyle {
        Normal = 0,
        Bold = 1,
        Italic = 2,
        Underline = 4
    };

signals:

public slots:

private:

    bool setFontStyle(QTextCharFormat &format, int style);

    QTextDocument *doc_;
    QTextCursor *cursor_;
};

#endif // PALAYDOCUMENT_H
