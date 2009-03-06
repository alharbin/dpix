include(../../npr_header.pri)

DESTDIR = ../lib.$${DBGNAME}

win32 {
	TEMPLATE = vclib
}
else {
	TEMPLATE = lib

macx {
	DEFINES += DARWIN
}
else {
	DEFINES += LINUX
}
}

CONFIG += staticlib
QT += opengl xml

TARGET = gq

DEPENDPATH += ../include
INCLUDEPATH += ../include
DEPENDPATH += ../../libcda/include
INCLUDEPATH += ../../libcda/include

#Input
HEADERS += ../include/GQ*.h
SOURCES += GQ*.cc
SOURCES += GLee.c
