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

TARGET = npr

DEPENDPATH += ../include
INCLUDEPATH += ../include
DEPENDPATH += ../../libcda/include
INCLUDEPATH += ../../libcda/include
DEPENDPATH += ../../libgq/include
INCLUDEPATH += ../../libgq/include

#Input
HEADERS += ../include/*.h
SOURCES += *.cc
