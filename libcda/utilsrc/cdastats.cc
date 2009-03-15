/*****************************************************************************\

CdaGeometry.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A simple program to write out some statistics of a COLLADA scene file.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <CdaScene.h>
#include <CdaNode.h>
#include <CdaGeometry.h>

QString usage = "usage: cdastats <filename>\n";

int instanced_triangles = 0;
int instanced_lines = 0;
int instanced_contours = 0;

void traverseAndCountGeometry(const CdaScene* scene, const CdaNode* root)
{
    if (!root)
        return;

    const CdaGeometry* geom = root->geometry();
    if (geom)
    {
        const CdaPrimitiveList& triangles = geom->primList(CDA_TRIANGLES);
        const CdaPrimitiveList& lines = geom->primList(CDA_LINES);
        const CdaPrimitiveList& contours = geom->primList(CDA_CONTOURS);

        for (int i = 0; i < triangles.size(); i++)
        {
            instanced_triangles += triangles[i].count();
        }
        for (int i = 0; i < lines.size(); i++)
        {
            instanced_lines += lines[i].count();
        }
        for (int i = 0; i < contours.size(); i++)
        {
            instanced_contours += contours[i].count();
        }
    }
    if (!root->nodeId().isEmpty())
    {
        const CdaNode* inst_node = scene->findLibraryNode(root->nodeId());
        traverseAndCountGeometry(scene, inst_node);
    }
    for (int i = 0; i < root->numChildren(); i++)
    {
        traverseAndCountGeometry(scene, root->child(i));
    }
}

void myMessageOutput(QtMsgType type, const char *msg)
 {
     switch (type) {
     case QtDebugMsg:
         break;
     case QtWarningMsg:
         fprintf(stderr, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         abort();
     }
 }


int main(int argc, char* argv[])
{
    qInstallMsgHandler(myMessageOutput);

    QCoreApplication application(argc, argv);
    QStringList arguments = application.arguments();

    if (arguments.size() < 2)
    {
        printf(qPrintable(usage));
        return 0;
    }

    CdaScene scene;
    bool ret = scene.load(arguments.at(1));
    if (!ret)
    {
        printf("failed to load %s (not a Collada file?)\n", 
            qPrintable(arguments.at(1)));
        return 0;
    }

    traverseAndCountGeometry(&scene, scene.root());

    printf("%s\n", qPrintable(arguments.at(1)));
    printf("Total triangles: %d\n", instanced_triangles);
    printf("Total lines: %d\n", instanced_lines);
    printf("Total contours: %d\n", instanced_contours);

    return 0;
}
