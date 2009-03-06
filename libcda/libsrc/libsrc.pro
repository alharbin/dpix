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


TARGET = cda

DEPENDPATH += ../include ../zlib
INCLUDEPATH += ../include ../zlib

#Input
HEADERS += ../include/Cda*.h
SOURCES += Cda*.cc ../zlib/*.c

win32 {
	SOURCES += ../zlib/win32/*.c
}
