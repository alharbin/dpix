CONFIG += debug_and_release

CONFIG(release, debug|release) {
	DBGNAME = release
}
else {
	DBGNAME = debug
}
DESTDIR = $${DBGNAME}

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

        LIBS += -framework CoreFoundation
        QMAKE_CXXFLAGS += -fopenmp
    }
    else {
        DEFINES += LINUX
        UNAME = Linux
    }
}

QT += opengl xml
TARGET = dpix

PRE_TARGETDEPS += ../libnpr/$${DBGNAME}/libnpr.a
DEPENDPATH += ../libnpr/include
INCLUDEPATH += ../libnpr/include
LIBS += -L../libnpr/$${DBGNAME} -lnpr

PRE_TARGETDEPS += ../libgq/$${DBGNAME}/libgq.a
DEPENDPATH += ../libgq/include
INCLUDEPATH += ../libgq/include
LIBS += -L../libgq/$${DBGNAME} -lgq

PRE_TARGETDEPS += ../libcda/$${DBGNAME}/libcda.a
DEPENDPATH += ../libcda/include
INCLUDEPATH += ../libcda/include 
LIBS += -L../libcda/$${DBGNAME} -lcda

PRE_TARGETDEPS += ../qglviewer/$${DBGNAME}/libqglviewer.a
DEPENDPATH += ../qglviewer
INCLUDEPATH += ../qglviewer 
LIBS += -L../qglviewer/$${DBGNAME} -lqglviewer
DEFINES += QGLVIEWER_STATIC

# Input
HEADERS += src/*.h \

FORMS += src/ui/*.ui

SOURCES += src/*.cc \

