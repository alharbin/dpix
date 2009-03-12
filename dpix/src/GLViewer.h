/******************************************************************************\
*                                                                            *
*  filename : GLViewer.h                                                     *
*  authors  : Forrester Cole                                                 *
*                                                                            *
*  OpenGL widget for drawing. Handles camera movement and mouse actions.     *
*                                                                            *
\******************************************************************************/

#ifndef GLVIEWER_H_
#define GLVIEWER_H_

#include "NPRUtility.h"
#include <qglviewer.h>
#include <QString>
#include <QTimer>
#include <vector>
using std::vector;

class NPRScene;
class NPRRenderer;
class NPRLight;
class Session;
class Console;

class GLViewer : public QGLViewer
{
    Q_OBJECT

public:
    typedef enum
    {
        HEADLIGHT,
        NORTH,
        NORTHNORTHEAST,
        NORTHEAST,
        EASTNORTHEAST,
        EAST,
        EASTSOUTHEAST,
        SOUTHEAST,
        SOUTHSOUTHEAST,
        SOUTH,
        SOUTHSOUTHWEST,
        SOUTHWEST,
        WESTSOUTHWEST,
        WEST,
        WESTNORTHWEST,
        NORTHWEST,
        NORTHNORTHWEST,
        NUM_LIGHT_PRESETS
    } LightPreset;
    
    static const QString lightPresetNames[];

public:
    GLViewer( QWidget* parent = 0 );

    void setNPR( NPRRenderer* renderer, NPRScene* npr_scene );
    void finishInit();
    void forceExtractLines();
    void resetView();

    void setLightPreset( LightPreset pos );
    LightPreset lightPreset() { return _light_preset; }
    void setLightDepth(float depth) { _light_depth = depth; }
    float lightDepth() { return _light_depth; }
    void setFocalPoint(const vec& v);

    void setTimersAreDisplayed( bool enable ) { _display_timers = enable; }
    void setResetTimersEachFrame( bool enable ) { _reset_timers_each_frame = enable; }

    void setAppropriateTextColor();

    virtual QDomElement domElement(const QString& name, QDomDocument& document) const;

public slots:
    void forceFullRedraw();
    virtual void initFromDOMElement(const QDomElement& element);

protected:
    virtual void draw();
    virtual void resizeGL( int width, int height );
    virtual void mousePressEvent( QMouseEvent* e );
    virtual void mouseMoveEvent( QMouseEvent* e );
    virtual void mouseReleaseEvent( QMouseEvent* e );
    virtual void keyPressEvent( QKeyEvent* e );

    void setFocalPoint(const QPoint& point);

private:
    void setupLighting();

private:
    NPRRenderer* _renderer;
    NPRScene*	 _npr_scene;
    bool         _inited;
    bool         _visible;
    bool		 _focus_dragging;
    bool		 _display_timers;
    bool		 _reset_timers_each_frame;

    LightPreset _light_preset;
    float       _light_depth;
    vec			_light_direction;

    vector<qglviewer::KeyFrameInterpolator*> _focus_paths;
    qglviewer::Frame _focus_frame;
};

#endif /*GLVIEWER_H_*/
