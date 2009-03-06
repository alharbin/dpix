/******************************************************************************\
 *                                                                            *
 *  filename : Scene.h                                                        *
 *  authors  : Forrester Cole                                                 *
 *                                                                            *
 *  Class for storing all the dpix state.                                     *
 *                                                                            *
\******************************************************************************/

#ifndef SCENE_H_
#define SCENE_H_

#include <QDomElement>
#include <QDir>

class NPRScene;
class NPRSettings;
class Session;
class GLViewer;

class Scene
{
public:
    Scene();

    void clear();

	bool load( const QString& filename );
    bool load( const QDomElement& root, const QDir& path );
	bool save( const QString& filename, const GLViewer* viewer );
    bool save( QDomDocument& doc, QDomElement& root, const QDir& path );

    NPRScene* nprScene() { return _npr_scene; }
    NPRSettings* nprSettings() { return _npr_settings; }
    Session* session() { return _session; }
    const QDomElement& viewerState() { return _viewer_state; }

    void setNprScene( NPRScene* scene ) { _npr_scene = scene; }
    void setNprSettings( NPRSettings* settings ) { _npr_settings = settings; }
    void setSession( Session* session ) { _session = session; }
    
protected:
    NPRScene* _npr_scene;
    NPRSettings* _npr_settings;
    Session* _session;
    QDomElement _viewer_state;
};

#endif // SCENE_H_