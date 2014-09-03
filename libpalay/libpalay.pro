#-------------------------------------------------
#
# Project created by QtCreator 2014-09-02T18:57:23
#
#-------------------------------------------------

QT       += core
QT       -= gui

greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport

TARGET = palay
TEMPLATE = lib

DEFINES += LIBPALAY_LIBRARY

SOURCES += Libpalay.cpp \
    PalayDocument.cpp

HEADERS += Libpalay.h\
        libpalay_global.h \
    PalayDocument.h

unix:cross_compile {
    LIBS += -llua -ldl
}
unix:!cross_compile {
    # On Ubuntu, but not buildroot, the LUA header files are
    # in lua5.2
    INCLUDEPATH += /usr/include/lua5.2
    LIBS += -llua5.2 -ldl
}

win32 {
    LIBS += -llua52
}


unix {
    target.path = /usr/lib
    INSTALLS += target
}
