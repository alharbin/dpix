include(../../npr_header.pri)

DESTDIR = ../bin.$${DBGNAME}

win32 {
    TEMPLATE = vcapp
    CONFIG(debug, debug|release) {
        DPIX = dpixd
    } else {
        DPIX = dpix
    }
    
    UNAME = Win32
}
else {
    TEMPLATE = app
    DPIX = dpix

    macx {
        DEFINES += DARWIN
        UNAME = Darwin
        CONFIG(debug, debug|release) {
            CONFIG -= app_bundle
        }
    }
    else {
        DEFINES += LINUX
        UNAME = Linux
    }
}

QT += opengl xml
TARGET = dpix

PRE_TARGETDEPS += ../../libnpr/lib.$${DBGNAME}/libnpr.a
DEPENDPATH += ../../libnpr/include
INCLUDEPATH += ../../libnpr/include
LIBS += -L../../libnpr/lib.$${DBGNAME} -lnpr

PRE_TARGETDEPS += ../../libgq/lib.$${DBGNAME}/libgq.a
DEPENDPATH += ../../libgq/include
INCLUDEPATH += ../../libgq/include
LIBS += -L../../libgq/lib.$${DBGNAME} -lgq

PRE_TARGETDEPS += ../../libcda/lib.$${DBGNAME}/libcda.a
DEPENDPATH += ../../libcda/include
INCLUDEPATH += ../../libcda/include 
LIBS += -L../../libcda/lib.$${DBGNAME} -lcda

PRE_TARGETDEPS += ../lib.$${DBGNAME}/libqglviewer.a
DEPENDPATH += ../qglviewer
INCLUDEPATH += ../qglviewer 
LIBS += -L../lib.$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC

# Input
HEADERS += *.h \

FORMS += ui/*.ui

SOURCES += *.cc \

