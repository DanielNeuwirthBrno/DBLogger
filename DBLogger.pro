#-------------------------------------------------
# Project: DBLogger
# Created: 2020-06-06
#-------------------------------------------------

QT += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += constants.h \
           database.h \
           mainwindow.h \
           query.h \
           session.h \
           shared.h \
           ui/ui_buttons.h \
           ui/ui_mainwindow.h

SOURCES += database.cpp \
           main.cpp \
           mainwindow.cpp \
           query.cpp \
           session.cpp

RESOURCES += resource.qrc

DISTFILES += notes.txt

TARGET = DBLogger
