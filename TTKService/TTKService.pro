#-------------------------------------------------
#
# Project created by QtCreator 2014-08-08T23:19:41
#
# =================================================
# * This file is part of the TTK Music Player project
# * Copyright (c) 2014 - 2016 Greedysky Studio
# * All rights reserved!
# * Redistribution and use of the source code or any derivative
# * works are strictly forbiden.
# =================================================

QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TTKMusicPlayer = 2.3.2.0
unix:VERSION += $$TTKMusicPlayer

win32{
  TARGET = ../../bin/TTKMusicPlayer
  LIBS += -L../bin/$$TTKMusicPlayer -lMusicUi
}
unix{
  TARGET = ../lib/TTKMusicPlayer
  LIBS += -L./lib/$$TTKMusicPlayer -lMusicUi
}

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += ../ \
    ../TTKCore/musicCore

SOURCES += \
    musicservicemain.cpp \
    musicserviceapplication.cpp \
    musicserviceobject.cpp

HEADERS += \
    musicserviceglobaldefine.h \
    musicserviceobject.h \
    musicserviceapplication.h

win32{
    RC_FILE = TTKService.rc
}