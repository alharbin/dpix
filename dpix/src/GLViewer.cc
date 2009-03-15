/*****************************************************************************\

GLViewer.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "GLViewer.h"
#include "MainWindow.h"
#include "NPRUtility.h"
#include "NPRRenderer.h"
#include "NPRScene.h"
#include "NPRLight.h"
#include "NPRStyle.h"
#include "GQStats.h"
#include "Session.h"
#include <XForm.h>
#include <assert.h>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QMouseEvent>

const QString GLViewer::lightPresetNames[] = { "Headlight",
    "North", "North-Northeast", "Northeast", "East-Northeast", "East", "East-Southeast", "Southeast", "South-Southeast",
    "South", "South-Southwest", "Southwest", "West-Southwest", "West", "West-Northwest", "Northwest", "North-Northwest" };

GLViewer::GLViewer(QWidget* parent) : QGLViewer( parent )
{ 
    _inited = false;
    _visible = false;
    _light_preset = NORTHEAST;
    _light_depth = 1.0;
    _focus_dragging = false;
    _display_timers = false;
    _reset_timers_each_frame = true;
    _focus_paths.resize(10, NULL);
    _npr_scene = NULL;
    _renderer = NULL;

    camera()->frame()->setWheelSensitivity(-1.0);
}

void GLViewer::resetView()
{
    vec center;
    float radius;

    if (_npr_scene)
    {
        _npr_scene->boundingSphere(center, radius);

        setupLighting();

        setSceneRadius( radius );
        setSceneCenter( qglviewer::Vec( center[0], center[1], center[2] ) );
        camera()->setFieldOfView(3.1415926f / 6.0f);
        camera()->setZNearCoefficient(0.01f);
        camera()->setOrientation(0,0);
        xform cam_xf = xform(camera()->frame()->matrix());
        _npr_scene->setCameraTransform(cam_xf);
        showEntireScene();
    }
}

void GLViewer::setNPR( NPRRenderer* renderer, NPRScene* npr_scene )
{ 
    _renderer = renderer; 
    _npr_scene = npr_scene;
    setFocalPoint(npr_scene->focalPoint());
    if (_inited && _renderer)
    {
        _renderer->resize( width(), height() );
    } 
}

void GLViewer::finishInit()
{
    resetView();
    forceFullRedraw();

    _inited = true;
}

void GLViewer::setupLighting()
{
    if (_npr_scene)
    {
        NPRLight* light = _npr_scene->light(0);

//        if (_light_preset != FOLLOWS_OBJECT)
//        {
//            qglviewer::Vec cam_dir = camera()->viewDirection();
//            GLdouble mv[16];
//            camera()->getModelViewMatrix(mv);
//            xform mv_xf( mv );
//            mv_xf = inv(rot_only( mv_xf ));

        switch(_light_preset)
        {
            case HEADLIGHT:
                _light_direction = vec(0.0f, 0.0f, 1.0f); break;
            case NORTH:
                _light_direction = vec(0.0f, 1.0f, _light_depth); break;
            case NORTHNORTHEAST:
                _light_direction = vec(0.374f, 1.0f, _light_depth); break;
            case NORTHEAST:
                _light_direction = vec(1.0f, 1.0f, _light_depth); break;
            case EASTNORTHEAST:
                _light_direction = vec(1.0f, 0.374f, _light_depth); break;
            case EAST:
                _light_direction = vec(1.0f, 0.0f, _light_depth); break;
            case EASTSOUTHEAST:
                _light_direction = vec(1.0f, -0.374f, _light_depth); break;
            case SOUTHEAST:
                _light_direction = vec(1.0f, -1.0f, _light_depth); break;
            case SOUTHSOUTHEAST:
                _light_direction = vec(0.374f, -1.0f, _light_depth); break;
            case SOUTH:
                _light_direction = vec(0.0f, -1.0f, _light_depth); break;
            case SOUTHSOUTHWEST:
                _light_direction = vec(-0.374f, -1.0f, _light_depth); break;
            case SOUTHWEST:
                _light_direction = vec(-1.0f, -1.0f, _light_depth); break;
            case WESTSOUTHWEST:
                _light_direction = vec(-1.0f, -0.374f, _light_depth); break;
            case WEST:
                _light_direction = vec(-1.0f, 0.0f, _light_depth); break;
            case WESTNORTHWEST:
                _light_direction = vec(-1.0f, 0.374f, _light_depth); break;
            case NORTHWEST:
                _light_direction = vec(-1.0f, 1.0f, _light_depth); break;
            case NORTHNORTHWEST:
                _light_direction = vec(-0.374f, 1.0f, _light_depth); break;
            default: break;
        }
        normalize(_light_direction);
//            _light_direction = mv_xf * _light_direction;
//        }

        light->setLightDir( _light_direction );
        light->applyToGL(0);
    }

    glShadeModel(GL_SMOOTH);
}

void GLViewer::setLightPreset( LightPreset pos )
{
    _light_preset = pos;
}

void GLViewer::resizeGL( int width, int height )
{
    if (width < 0 || height < 0) {
        _visible = false;
        return;
    }

    _visible = (width * height != 0);
    
    QGLViewer::resizeGL( width, height);
    if (_inited && _renderer)
    {
        _renderer->resize( width, height );
    }
}

static bool in_draw_function = false;

void GLViewer::draw()
{  
    if (in_draw_function)
        return; // recursive draw

    if (!(_visible && _inited && _renderer && _npr_scene))
        return;
    
    in_draw_function = true;

    GQStats& perf = GQStats::instance();
    if (_reset_timers_each_frame)
        perf.reset();

    setupLighting();

    xform cam_xf = xform(camera()->frame()->matrix());
    _npr_scene->setCameraTransform(cam_xf);

    vec fp;
    _focus_frame.getTranslation( fp[0], fp[1], fp[2] );
    _npr_scene->setFieldOfView(camera()->fieldOfView());
    _npr_scene->setFocalPoint( fp );

    _npr_scene->computePotentiallyVisibleSet();

    _renderer->drawScene(*_npr_scene);

    if (_display_timers)
    {
        perf.updateView();
    }

    in_draw_function = false;
}


void GLViewer::forceFullRedraw()
{
    forceExtractLines();
    if (_reset_timers_each_frame)
    {
        GQStats::instance().clearTimers();
        GQStats::instance().clearCounters();
    }
    updateGL();
}

void GLViewer::forceExtractLines()
{
    //_need_extract = true;
}

void GLViewer::setFocalPoint( const QPoint& point )
{
    vec fp;
    NPRFocusMode mode = (NPRFocusMode)(NPRSettings::instance().get(NPR_FOCUS_MODE));
    if (mode == NPR_FOCUS_WORLD)
    {
        _renderer->drawSceneDepth( *_npr_scene );
        bool found = false;
        qglviewer::Vec wp = camera()->pointUnderPixel( point, found );
        if (found)
        {
            vec wp2 = vec(wp.x, wp.y, wp.z );
            _focus_frame.setTranslation( wp );
            fp = wp2;
        }
    }
    else if (mode == NPR_FOCUS_SCREEN)
    {
        vec sp = vec(point.x(), height() - point.y(), 0);
        _focus_frame.setTranslation( sp[0], sp[1], sp[2] );
        fp = sp;
    }

    _npr_scene->setFocalPoint(fp);
}

void GLViewer::mousePressEvent( QMouseEvent* e )
{
    if (e->button() == Qt::LeftButton && e->modifiers() == Qt::ShiftModifier)
    {
        _focus_dragging = true;
        setFocalPoint( e->pos() );
        updateGL();
    }
    else if (e->button() == Qt::LeftButton && e->modifiers() == Qt::AltModifier)
    {
        // Normal camera rotation, but set the revolve point from the point the user clicked.
        camera()->setRevolveAroundPointFromPixel(e->pos());
        QGLViewer::mousePressEvent(e);
    }
    else
    {
        QGLViewer::mousePressEvent(e);
    }
}

void GLViewer::mouseReleaseEvent( QMouseEvent* e )
{
    if (e->button() == Qt::LeftButton && e->modifiers() == Qt::ShiftModifier)
    {
        _focus_dragging = false;
    }
    else
    {
        QGLViewer::mousePressEvent(e);
    }
}

void GLViewer::mouseMoveEvent( QMouseEvent* e )
{
    // Despite the name, this function is only called when a mouse button is
    // held down.
    if (_focus_dragging)
    {
        setFocalPoint( e->pos() );
        updateGL();
    }
    else
    {
        QGLViewer::mouseMoveEvent(e);
    }
}

void GLViewer::keyPressEvent( QKeyEvent* e )
{
    if (e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9 && e->modifiers() == Qt::AltModifier)
    {
        int path = e->key() - Qt::Key_1;
        if (_focus_paths[path] == NULL)
        {
            _focus_paths[path] = new qglviewer::KeyFrameInterpolator(&_focus_frame);
        }

        qglviewer::Frame frame;
        vec fp = _npr_scene->focalPoint();
        frame.setTranslation( fp[0], fp[1], fp[2] ); 
        _focus_paths[path]->addKeyFrame(frame);

        if (_focus_paths[path]->numberOfKeyFrames() == 1)
            displayMessage(QString("Focus point %1 saved").arg(path));
        else
            displayMessage(QString("Focus position %1 on path %2 saved").
            arg(_focus_paths[path]->numberOfKeyFrames()).arg(path));
    }
    else if (e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9)
    {
        int path = e->key() - Qt::Key_1;

        if (_focus_paths[path])
        {
            connect( _focus_paths[path], SIGNAL(interpolated()), SLOT(updateGL()) );
            _focus_paths[path]->startInterpolation();
        }
    }
    else
        QGLViewer::keyPressEvent(e);

}

void GLViewer::setAppropriateTextColor()
{
    const vec& bg_color = _npr_scene->globalStyle()->backgroundColor();
    float sum = bg_color[0] + bg_color[1] + bg_color[2];
    if (sum > 1.5f)
    {
        setForegroundColor( Qt::black );
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        setForegroundColor( Qt::white );
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void GLViewer::setFocalPoint(const vec& v)
{
    _focus_frame.setTranslation(v[0], v[1], v[2]);
}


QDomElement GLViewer::domElement(const QString& name, QDomDocument& document) const
{
    // Store the light preset and light depth values.
    QDomElement light = document.createElement("light");
    light.setAttribute("preset", _light_preset);
    light.setAttribute("depth", _light_depth);

    QDomElement res = QGLViewer::domElement(name, document);
    res.appendChild(light);
    return res;
}

void GLViewer::initFromDOMElement(const QDomElement& element)
{
    QGLViewer::initFromDOMElement(element);

    QDomElement light = element.firstChildElement("light");
    if (!light.isNull())
    {
        _light_preset = (LightPreset)(light.attribute("preset").toInt());
        _light_depth = light.attribute("depth").toFloat();
    }
}


