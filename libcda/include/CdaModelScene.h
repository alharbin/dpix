/*****************************************************************************\

CdaModelScene.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

A descendant of CdaScene that implements QAbstractItemModel, which allows
the scenegraph to be easily viewed and edited in a Qt TreeView widget.

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef _CDA_MODEL_SCENE_H_
#define _CDA_MODEL_SCENE_H_

#include "CdaScene.h"
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class CdaModelScene : public QAbstractItemModel, public CdaScene
{
    Q_OBJECT

public:
    CdaModelScene(QObject* parent = 0);
    ~CdaModelScene();

    // overrided functions from CdaScene
    void clear();
    bool load(const QString& filename);

    // add library elements
    void                addLibraryLight(CdaLight* light);

    // add a node
    void                addNodeInstance(const QString& parent_id, CdaNode* node);

    // implementation of QAbstractItemModel
    QVariant        data(const QModelIndex& index, int role) const;
    Qt::ItemFlags   flags(const QModelIndex& index) const;
    QVariant        headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex     index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex     parent(const QModelIndex &index) const;
    int             rowCount(const QModelIndex& parent = QModelIndex()) const;
    int             columnCount(const QModelIndex& parent = QModelIndex()) const;

protected:
    void postLoad();

protected:
    CdaNode*    _dummy_model_root;

};

#endif


