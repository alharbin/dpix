/*****************************************************************************\

CdaModelScene.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

libcda is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include <assert.h>
#include "CdaModelScene.h"
#include "CdaNode.h"
#include "CdaGeometry.h"

CdaModelScene::CdaModelScene( QObject* parent ) : QAbstractItemModel(parent), CdaScene()
{
    _dummy_model_root = NULL;
    clear();
}

CdaModelScene::~CdaModelScene()
{
    clear();
}

void CdaModelScene::clear()
{
    if (_dummy_model_root)
    {
        // Must detach the dummy before deleting it, otherwise it will interfere
        // with the CdaScene::clear call below.
        for (int i = 0; i < (int)_dummy_model_root->numChildren(); i++) {
            _dummy_model_root->child(i)->setParent(NULL);
        }
        _dummy_model_root->clearChildren();
        
        delete _dummy_model_root;
        _dummy_model_root = NULL;
    }
    CdaScene::clear();
    
}

bool CdaModelScene::load(const QString& filename)
{
    if (!CdaScene::load(filename))
        return false;

    postLoad();
    return true;
}

void CdaModelScene::addLibraryLight(CdaLight* light)
{
    (void)light;
    
    assert(0);
}

void CdaModelScene::addNodeInstance(const QString& parent_id, CdaNode* node)
{
    (void)parent_id;
    (void)node;
    
    assert(0);
}

QVariant CdaModelScene::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    CdaNode* node = getNode(index.internalId());

    return node->name();
}

Qt::ItemFlags CdaModelScene::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant CdaModelScene::headerData(int section, Qt::Orientation orientation, int role) const
{
    (void)section;
    
     if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
         return "Node";

     return QVariant();
 }

QModelIndex CdaModelScene::index(int row, int column, const QModelIndex& parent) const
{
    CdaNode* parent_node;

    if (!parent.isValid())
        parent_node = _dummy_model_root;
    else
        parent_node = getNode(parent.internalId());

    if (row >= 0 && row < (int)(parent_node->numChildren()))
    {
        CdaNode* child_node = parent_node->childSorted(row);
        return createIndex(row, column, child_node->uid());
    }
   
    return QModelIndex();
}

QModelIndex CdaModelScene::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    CdaNode *child_node = getNode(index.internalId());
    CdaNode *parent_node = child_node->parent();

    if (parent_node == _dummy_model_root)
        return QModelIndex();

    int row = 0;
    CdaNode* grandparent = parent_node->parent();
    if (grandparent)
    {
        for (int i = 0; i < grandparent->numChildren(); i++)
        {
            if (grandparent->childSorted(i) == parent_node)
            {
                row = i;
                break;
            }
        }
    }

    return createIndex(row, 0, parent_node->uid());
}
 
int CdaModelScene::rowCount(const QModelIndex& parent) const
{
    CdaNode* parent_node;

    if (!parent.isValid())
        parent_node = _dummy_model_root;
    else
        parent_node = getNode(parent.internalId());

    return parent_node->numChildren();
}

int CdaModelScene::columnCount(const QModelIndex& parent) const
{
    (void)parent;
    
    return 1;
}

void CdaModelScene::postLoad()
{
    _dummy_model_root = new CdaNode();
    _dummy_model_root->addChild(_root);
    QMap< QString, CdaNode* > sort_map;
    
    for (int i = 0; i < (int)_library_nodes.size(); i++) {
        sort_map.insert(_library_nodes[i]->name(), _library_nodes[i]);
    }
    
    const QList< CdaNode* > sorted_nodes = sort_map.values();
    
    for (int i = 0; i < sorted_nodes.size(); i++) {
        _dummy_model_root->addChild(sorted_nodes[i]);
    }
    
    for (int i = 0; i < (int)_dummy_model_root->numChildren(); i++) {
        _dummy_model_root->child(i)->setParent(_dummy_model_root);
    }

    return;
}
