#ifndef PALAYDOCUMENT_H
#define PALAYDOCUMENT_H

#include <QObject>
#include <QTextDocument>

struct lua_State;

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

signals:

public slots:

private:
    QTextDocument *doc_;
    QTextCursor *cursor_;
};

#endif // PALAYDOCUMENT_H
