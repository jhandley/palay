#-------------------------------------------------
#
# Project created by QtCreator 2014-09-04T09:25:28
#
#-------------------------------------------------

QT       += core gui

TARGET = palay
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

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
    target.path = /usr/bin
    INSTALLS += target
}
