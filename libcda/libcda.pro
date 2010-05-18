CONFIG += debug_and_release

CONFIG(release, debug|release) {
	DBGNAME = release
}
else {
	DBGNAME = debug
}
DESTDIR = $${DBGNAME}

win32 {
	TEMPLATE = vclib
}
else {
	TEMPLATE = lib

	macx {
		DEFINES += DARWIN
        QMAKE_CXXFLAGS += -fopenmp
	}
	else {
		DEFINES += LINUX
	}
}

CONFIG += staticlib
QT += opengl xml


TARGET = cda

DEPENDPATH += include zlib
INCLUDEPATH += include zlib

#Input
HEADERS += include/Cda*.h
SOURCES += libsrc/Cda*.cc zlib/*.c

win32 {
	SOURCES += zlib/win32/*.c
}
