/*****************************************************************************\

NPRAnimController.h
Author: Forrester Cole (fcole@cs.princeton.edu)
        Michael Burns (mburns@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A controller class for simple path animations.

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _NPR_ANIM_CONTROLLER_H
#define _NPR_ANIM_CONTROLLER_H

#include <NPRFixedPathSet.h>
#include <XForm.h>

class NPRAnimController
{
public:
    NPRAnimController(const NPRGeometry *geom, vec &center);
    ~NPRAnimController();

    const xform& transform() const { return _transform; }
    void setSpeed(float speed);
    void setFrame(unsigned int frame);
    void reset();
    
private:    
    void calcCurrentLength();
    void calcRotation();
    void updateTransform();
    
    NPRFixedPathSet* _paths;
    NPRFixedPath*    _cur_path;
    vec _center;
    vec _origin_translation;
    float _speed;
    int _frame;
    float _traversed;
    int _current_vert;
    float _current_length;
    float _total_length;
    int _num_frames;
    vec _initial_direction;
    vec _current_direction;
    vec _rotation_axis;
    float _rotation_amount;
    xform _transform;
};

#endif 
