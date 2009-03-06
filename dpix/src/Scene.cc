#include "Scene.h"

#include "NPRScene.h"
#include "NPRSettings.h"
#include "Session.h"
#include "GLViewer.h"

#include <QFileInfo>

const int CURRENT_VERSION = 1;

Scene::Scene()
{
    _npr_scene = 0;
    _npr_settings = 0;
    _session = 0;
}

void Scene::clear()
{
    delete _npr_scene;
    delete _npr_settings;
    delete _session;
    _npr_scene = 0;
    _npr_settings = 0;
    _session = 0;
}


bool Scene::load( const QString& filename )
{
    if (filename.endsWith("dps"))
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning("Could not open %s", qPrintable(filename));
            return false;
        }

        QDomDocument doc("dpixscene");
        QString parse_errors;
        if (!doc.setContent(&file, &parse_errors))
        {
            qWarning("Parse errors: %s", qPrintable(parse_errors));
            return false;
        }

        file.close();

        QDomElement root = doc.documentElement();
        QDir path = QFileInfo(filename).absoluteDir();

        return load(root, path); 
    }
    else
    {
        _npr_scene = new NPRScene();
        bool ret = _npr_scene->load(filename);
        if (!ret)
        {
            clear();
            return false;
        }
        _npr_settings = new NPRSettings();
        _session = 0;
        _viewer_state.clear();

        return true;
    }
}

bool Scene::load( const QDomElement& root, const QDir& path )
{
    int version = root.attribute("version").toInt();
    if (version != CURRENT_VERSION)
    {
        qWarning("Scene::load: file version out of date (%d, current is %d)", 
            version, CURRENT_VERSION);
        return false;
    }

	QDomElement nprsettings = root.firstChildElement("nprsettings");
    if (nprsettings.isNull())
    {
        qWarning("Scene::load: no nprsettings node found.\n");
        clear();
        return false;
    }

    _npr_settings = new NPRSettings();
    bool ret = _npr_settings->load(nprsettings);
    if (!ret)
    {
        clear();
        return false;
    }

	QDomElement nprscene = root.firstChildElement("nprscene");
    if (nprscene.isNull())
    {
        qWarning("Scene::load: no nprscene node found.\n");
        return false;
    }

    _npr_scene = new NPRScene();
    ret = _npr_scene->load(nprscene, path);
    if (!ret)
    {
        clear();
        return false;
    }

	QDomElement session = root.firstChildElement("session");
    if (!session.isNull())
    {
        _session = new Session();
        ret = _session->load(session);
        if (!ret)
        {
            clear();
            return false;
        }
    }

    _viewer_state = root.firstChildElement("viewerstate");
    if (_viewer_state.isNull())
    {
        qWarning("Scene::load: no viewerstate node found.\n");
        clear();
        return false;
    }

    return true;
}

bool Scene::save( const QString& filename, const GLViewer* viewer )
{
    QDomDocument doc("dpixscene");
    QDomElement root = doc.createElement("dpixscene");
    doc.appendChild(root);

    _viewer_state = viewer->domElement("viewerstate", doc);

    QDir path = QFileInfo(filename).absoluteDir();

    bool ret = save(doc, root, path);
    if (!ret)
        return false;

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning("Scene::save - Could not save %s", qPrintable(filename));
        return false;
    }

    file.write(doc.toByteArray());

    file.close();

    return true;
}

bool Scene::save( QDomDocument& doc, QDomElement& root, const QDir& path )
{
    root.setAttribute("version", CURRENT_VERSION);

	QDomElement nprscene = doc.createElement("nprscene");
    _npr_scene->save(doc, nprscene, path);
    root.appendChild(nprscene);

	QDomElement nprsettings = doc.createElement("nprsettings");
    _npr_settings->save(doc, nprsettings);
    root.appendChild(nprsettings);

    if (_session)
    {
    	QDomElement session = doc.createElement("session");
        _session->save(doc, session);
        root.appendChild(session);
    }

    root.appendChild(_viewer_state);

    return true;
}