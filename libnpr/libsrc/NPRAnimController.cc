/*****************************************************************************\

NPRAnimController.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "NPRAnimController.h"
#include "GQInclude.h"
#include <QDebug>

NPRAnimController::NPRAnimController(const NPRGeometry *geom, vec &center):
    _center(center),
    _frame(0)
{
    _paths = new NPRFixedPathSet(geom);
    _cur_path = _paths->at(0);

    _total_length = 0.0f;
    for (int i = 0; i < _cur_path->size() - 1; i++) {
        const vec &va = _cur_path->vert(i);
        const vec &vb = _cur_path->vert(i + 1);
        _total_length += dist(va, vb);
    }

    _origin_translation = -center;
    
    setSpeed(0.0f);
}

NPRAnimController::~NPRAnimController()
{
    delete _paths;
}

void NPRAnimController::setSpeed(float speed)
{
    _speed = speed;
    _num_frames = _speed > 0.0f ? (unsigned int)(_total_length / _speed) + 1 : 0;
    
    reset();
}

void NPRAnimController::reset()
{
    _current_vert = 0;
    _traversed = 0.0;
    calcCurrentLength();
    _initial_direction = _current_direction;
    _rotation_axis = vec3(0.0f, 0.0f, 1.0f);
    calcRotation();
//    qDebug("Initial dir: (%f, %f, %f), current dir: (%f, %f, %f)", _initial_direction[0], _initial_direction[1], _initial_direction[2], _current_direction[0], _current_direction[1], _current_direction[2]);
//    qDebug("Axis: (%f, %f, %f), Amount: %f", _rotation_axis[0], _rotation_axis[1], _rotation_axis[2], _rotation_amount);
}

void NPRAnimController::setFrame(unsigned int frame)
{
    if (_num_frames > 0) {
        int new_frame = frame % _num_frames;
        if (new_frame < _frame)
            reset();
        _frame = frame % _num_frames;
        updateTransform();
    }
}

void NPRAnimController::calcCurrentLength()
{
    const vec &va = _cur_path->vert(_current_vert);
    const vec &vb = _cur_path->vert((_current_vert + 1) % _cur_path->size());
    _current_length = dist(va, vb);
    _current_direction = (vb - va) / _current_length;
}

void NPRAnimController::calcRotation()
{
    vec new_rotation_axis = _initial_direction CROSS _current_direction;
    if (len2(new_rotation_axis) > 0.001)
        _rotation_axis = new_rotation_axis;
    
    _rotation_amount = std::acos(_initial_direction DOT _current_direction);
}

void NPRAnimController::updateTransform()
{
    float current_pos = _frame * _speed;
    float distance = current_pos - _traversed;
    
    while (distance > _current_length) {
        // advance to next pair
        _current_vert = (_current_vert+1) % _cur_path->size();
        
        distance -= _current_length;
        _traversed += _current_length;
        
        calcCurrentLength();
        calcRotation();
//        qDebug("Initial dir: (%f, %f, %f), current dir: (%f, %f, %f)", _initial_direction[0], _initial_direction[1], _initial_direction[2], _current_direction[0], _current_direction[1], _current_direction[2]);
//        qDebug("Axis: (%f, %f, %f), Amount: %f", _rotation_axis[0], _rotation_axis[1], _rotation_axis[2], _rotation_amount);
    }

    // within this segment
    const vec &v0 = _cur_path->vert(0);
    const vec &va = _cur_path->vert(_current_vert);

    vec translation = _current_direction * distance + va - v0;

    _transform = xform::trans(translation) * xform::trans(_center) *xform::rot(_rotation_amount, _rotation_axis) *  xform::trans(_origin_translation);
}
