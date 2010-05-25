/*****************************************************************************\

Console.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Console.h"
#include <stdio.h>
#include <QMessageBox>
#include <assert.h>

#ifdef WIN32
    #include <string.h>
    #include "windows.h"
#endif

Console* Console::_current_msg_console = 0;

Console::Console()
{
    connect( &_process, SIGNAL(readyReadStandardOutput()), this, 
        SLOT(getProcessStdout()) );
    _process.setReadChannelMode( QProcess::MergedChannels );
    _process.setReadChannel( QProcess::StandardOutput );
}

void Console::show()
{
    if (isHidden())
    {
        resize(500,300);
        setWindowTitle("dpix console");

        QTextEdit::show();
    }
}

void Console::print( const QString& str )
{
    insertPlainText( str );
    moveCursor( QTextCursor::End );
}

void Console::execute( const QString& cmd, const QStringList& args )
{
    _process.start(cmd, args);
    _process.waitForFinished();
}


void Console::getProcessStdout()
{
    QString out = _process.readAll();
    print( out );
}

void Console::installMsgHandler()
{
    assert(_current_msg_console == 0);
    qInstallMsgHandler( msgHandler );
    _current_msg_console = this;

}

void Console::removeMsgHandler()
{
    assert(_current_msg_console == this);
    qInstallMsgHandler( msgHandler );
    _current_msg_console = 0;
}

void Console::msgHandler( QtMsgType type, const char* msg )
{
    if (type == QtFatalMsg)
    {
        QMessageBox::critical( 0, "Error", QString(msg) );
        abort();
    }

    QString out = QString(msg) + QString("\n");

    switch (type) {
        case QtWarningMsg:  out = QString("Warning: ") + out; break;
        case QtCriticalMsg: out = QString("Critical: ") + out; break;
        case QtDebugMsg: out = QString("Debug: ") + out; break;
        case QtFatalMsg: break;
    }

    _current_msg_console->print( out );
    fprintf( stderr, qPrintable(out) );

#ifdef WIN32
    wchar_t wcharstr[1024];
    int length = out.trimmed().left(1023).toWCharArray(wcharstr);
    wcharstr[length] = 0;
    OutputDebugStringW(wcharstr);
    OutputDebugStringA("\r\n");
#endif
}
