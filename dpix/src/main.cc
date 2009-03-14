/******************************************************************************\
*                                                                            *
*  filename : main.cc                                                        *
*  authors  : Forrester Cole                                                 *
*                                                                            *
*  Main source file for dpix. Just opens a MainWindow widget.            *
*                                                                            *
\******************************************************************************/

#include "NPRUtility.h"
#include <QApplication>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QMessageBox>

#include "NPRRenderer.h"
#include "NPRSettings.h"
#include "GQShaderManager.h"

#include <stdio.h>
#include <stdlib.h>
#include "XForm.h"
#include "MainWindow.h"

void printUsage(const char *myname)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "\n Usage    : %s [infile] [options]\n", myname);

    fprintf(stderr, " options: -h | --help   : print this message\n");
    fprintf(stderr, "        : -saveandquit <filename> : save one screen shot and quit\n");

    fprintf(stderr, "\n        : DEBUGGING OPTIONS:\n");
    fprintf(stderr, "        : -nolines      : turn off line drawing\n");

    exit(1);
}

QDir findShadersDirectory( const QString& app_path )
{
    // look for the shaders/programs.xml file to find the working directory
    QStringList candidates;
    candidates << QDir::currentPath()
    << QDir::cleanPath(app_path)
    << QDir::cleanPath(app_path + "/Contents/MacOS/")
    << QDir::cleanPath(app_path + "/../../libnpr/")
    << QDir::cleanPath(app_path + "/../../../../libnpr/")
    << QDir::cleanPath(app_path + "/../../../../../libnpr/");

    for (int i = 0; i < candidates.size(); i++)
    {
        if (QFileInfo(candidates[i] + "/shaders/programs.xml").exists())
            return QDir(candidates[i]);
    }

    QString pathstring;
    for (int i = 0; i < candidates.size(); i++)
        pathstring = pathstring + candidates[i] + "/shaders/programs.xml\n";

    QMessageBox::critical(NULL, "Error", 
        QString("Could not find programs.xml (or the working directory). Tried: \n %1").arg(pathstring));

    exit(1);
}

// the main routine makes the window, and then runs an event loop
// until the window is closed
int main( int argc, char** argv )
{
    QString scene_name;
    QString save_and_quit_file;

    QApplication app(argc, argv);
    QDir shaders_dir = findShadersDirectory(app.applicationDirPath());
    QDir working_dir;
    // As a convenience, set the working directory to libnpr
    // when running out of the libnpr tree.
    if (shaders_dir.absolutePath().endsWith("libnpr"))
        working_dir = shaders_dir;
    else
        working_dir = QDir::home();

    shaders_dir.cd("shaders");
    GQShaderManager::setShaderDirectory(shaders_dir);

    QStringList arguments = app.arguments();

    NPRSettings::instance().loadDefaults();

    for (int i = 1; i < arguments.size(); i++)
    {
        const QString& arg = arguments[i];

        if (i == 1 && !arg.startsWith("-"))
        {
            scene_name = arg;
        }
        else if ( arg == "-saveandquit" )
        {
            if (++i >= arguments.size())
                printUsage(argv[0]);
            save_and_quit_file = arguments[i];
        }
        else if ( arg == "-nolines" )
        {
            NPRSettings::instance().set(NPR_ENABLE_LINES, false);
        }
        else
            printUsage(argv[0]);
    }


    MainWindow window;
    window.init( working_dir, scene_name );	
    window.show();

    if (!save_and_quit_file.isEmpty())
    {
        window.getGLViewer()->saveSnapshot(save_and_quit_file);
        return 0;
    }

    return app.exec();
}
