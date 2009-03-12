#include <QtGui>
#include <QFileDialog>
#include <QRegExp>

#include "GLViewer.h"
#include "MainWindow.h"
#include "NPRSettings.h"
#include "NPRRendererStandard.h"
#include "NPRScene.h"
#include "NPRGLDraw.h"
#include "NPRLight.h"
#include "NPRStyle.h"
#include "GQShaderManager.h"

#include "NPRDrawable.h"
#include "GQStats.h"

#include "Session.h"
#include "Console.h"

#include <XForm.h>
#include <assert.h>

const int CURRENT_INTERFACE_VERSION = 3;

const int MAX_RECENT_SCENES = 4;

MainWindow::MainWindow( )
{
    _current_renderer = NULL;
    _standard_renderer = NULL;
    _current_session = NULL;
    _console = NULL;
    _npr_scene = NULL;
    _glViewer = NULL;
    _dpix_scene = NULL;
    _recent_scenes_actions.resize(MAX_RECENT_SCENES);
    
    _ui.setupUi(this);

    for (int i = 0; i < (int)(GLViewer::NUM_LIGHT_PRESETS); i++) {
        _ui.lightingPositionComboBox->addItem(GLViewer::lightPresetNames[i]);
    }
}

MainWindow::~MainWindow()
{
    clearRenderers();
    delete _npr_scene;
}

void MainWindow::init( const QDir& working_dir, const QString& scenename )
{
    _working_dir = working_dir;

    QGLFormat format;
    format.setAlpha(true);
    // enabling multisampling caused weird buffer corruption bugs 
    // (back buffer sometimes is visible, back buffer sometimes is not cleared properly)
    //format.setSampleBuffers(true);
    format.setDoubleBuffer(true);
    QGLFormat::setDefaultFormat(format);

    _glViewer = _ui.viewer;
    _glViewer->makeCurrent();
    _ui.lightingPositionComboBox->setCurrentIndex(_glViewer->lightPreset());
   
    updateUiFromSettings();

    changeSessionState(SESSION_NONE);

    _console = new Console();
    _console->installMsgHandler();
    Session::setConsole(_console);

    // read back qsettings and update window state and recent file list
    QSettings settings("dpix", "dpix");
    settings.beginGroup("mainwindow");
    QByteArray windowstate = settings.value("windowstate").toByteArray();
    bool restored = restoreState(windowstate, CURRENT_INTERFACE_VERSION);
    if (!restored)
    {
        hideAllDockWidgets();
    }
    updateWindowMenu();

    settings.endGroup();
    settings.beginGroup("recent_scenes");
    for (int i = 0; i < MAX_RECENT_SCENES; i++)
    {
        QString scene_name = settings.value(QString("scene_%1").arg(i)).toString();

        if (!scene_name.isEmpty())
        {
            _recent_scenes.append(scene_name);
        }
    }
    updateRecentScenesMenu();

    _currentTransferIndex = _ui.transferComboBox->currentIndex();

    _last_scene_dir = working_dir.absolutePath() + "/models";
    _last_style_dir = working_dir.absolutePath() + "/styles";
    _last_texture_dir = working_dir.absolutePath() + "/textures";
    _last_export_dir = working_dir.absolutePath();
    _last_camera_dir = working_dir.absolutePath();

    _ui.statsTreeView->setModel(&(GQStats::instance()));

    resize(1000,800);

    if (!scenename.isEmpty())
    {
        if (openScene( scenename ))
        {
            _scenename = QDir::fromNativeSeparators(scenename);
        }
    }

    makeWindowTitle();

    _glViewer->finishInit();
}

void MainWindow::closeEvent( QCloseEvent* event )
{
    delete _console;

    QSettings settings("dpix", "dpix");
    settings.beginGroup("mainwindow");
    settings.setValue("windowstate", saveState(CURRENT_INTERFACE_VERSION));
    settings.endGroup();

    addCurrentSceneToRecentList();

    settings.beginGroup("recent_scenes");
    for (int i = 0; i < _recent_scenes.size(); i++)
    {
        if (!_recent_scenes[i].isEmpty())
        {
            settings.setValue(QString("scene_%1").arg(i), _recent_scenes[i]);
        }
    }

    settings.sync();

    event->accept();
}

bool MainWindow::openScene( const QString& filename )
{
    // Try to do all the file loading before we change the mainwindow state at all.

    QString absfilename = QDir::fromNativeSeparators(_working_dir.absoluteFilePath(filename));
    _scenename = absfilename;
    QFileInfo fileinfo(absfilename);
    
    if (!fileinfo.exists())
    {
        QMessageBox::critical(this, "File Not Found", QString("\"%1\" does not exist.").arg(absfilename));
        return false;
    }

    Scene* new_scene = new Scene();
    
    if (!new_scene->load(absfilename))
    {
        QMessageBox::critical(this, "Open Failed", QString("Failed to load \"%1\". Check console.").arg(absfilename));
        delete new_scene;
        return false;
    }

    // Success: file has been loaded, now change state

    clearRenderers();
    allocRenderers();

    if (_dpix_scene)
        delete _dpix_scene;

    _dpix_scene = new_scene;

    NPRSettings::instance().copyPersistent(*(_dpix_scene->nprSettings()));

    _npr_scene = _dpix_scene->nprScene();
    GQStats::instance().clear();
    _npr_scene->recordStats(GQStats::instance());
    _npr_scene->updateVBOs();

    _current_session = _dpix_scene->session();
    if (_current_session)
        changeSessionState(SESSION_LOADED);
    else
        changeSessionState(SESSION_NONE);

    // Set a zero renderer to stop the viewer from redrawing itself during initialization.
    _glViewer->setNPR(0, _npr_scene);
    if (!_dpix_scene->viewerState().isNull())
        _glViewer->initFromDOMElement(_dpix_scene->viewerState());
    else
        _glViewer->resetView();

    addCurrentSceneToRecentList();

    makeWindowTitle();
    
    updateUiFromScene();
    updateUiFromStyle();
    updateUiFromViewer();
    updateUiFromSettings();

    // Now set the proper renderer.
    _glViewer->setNPR(_current_renderer, _npr_scene);
    
    return true;
}

bool MainWindow::saveScene( const QString& filename )
{
    _dpix_scene->nprSettings()->copyPersistent(NPRSettings::instance());
    _dpix_scene->setSession(_current_session);
    return _dpix_scene->save(filename, _glViewer);
}

bool MainWindow::openRecentScene(int which)
{
    bool ret = false;

    for (int i = 0; i < _recent_scenes.size(); i++)
        qDebug("%s\n", qPrintable(_recent_scenes[i]));

    if (_recent_scenes.size() > which && 
        !_recent_scenes[which].isEmpty())
    {
        QFileInfo fileinfo(_recent_scenes[which]);
        QString fullpath = fileinfo.absoluteFilePath();

        if (openScene( _recent_scenes[which] ))
        {
            _glViewer->forceFullRedraw();
        }
    }

    return ret;
}

void MainWindow::addCurrentSceneToRecentList()
{
    if (!_scenename.isEmpty() && !_recent_scenes.contains(_scenename))
    {
        QFileInfo fileinfo(_scenename);

        _recent_scenes.push_front(_scenename);
        if (_recent_scenes.size() > MAX_RECENT_SCENES)
        {
            _recent_scenes.pop_back();
        }

        updateRecentScenesMenu();
    }
}

void MainWindow::clearRenderers()
{
    if (_standard_renderer)
    {
        delete _standard_renderer;
        _standard_renderer = 0;
    }
    _current_renderer = 0;
}

void MainWindow::allocRenderers()
{
    if (_standard_renderer == 0)
        _standard_renderer = new NPRRendererStandard();

    _current_renderer = _standard_renderer;
}

void MainWindow::on_actionShow_FPS_toggled( bool checked )
{
    _glViewer->setAppropriateTextColor();             
    _glViewer->setFPSIsDisplayed( checked );
    _glViewer->forceFullRedraw();
}
    
void MainWindow::on_actionOpen_Style_triggered()
{
    if (_npr_scene)
    {
        QString filename = 
            myFileDialog(QFileDialog::AcceptOpen, "Open Style", "Style Files (*.sty)", _last_style_dir );

        if (!filename.isNull())
        {
            NPRStyle* style = new NPRStyle();
            if (!style->load(qPrintable(filename)))
            {
                delete style;
                return;
            }
            QString rel_name = _working_dir.relativeFilePath(filename);
            _npr_scene->setStyle(style);
            _stylename = rel_name;
            _ui.penStyleTypeBox->setCurrentIndex(0);
            makeWindowTitle();
            updateUiFromStyle();
        }
    }
}

void MainWindow::on_actionSave_Style_triggered()
{
    saveStyle( _stylename );
}

void MainWindow::on_actionSave_Style_As_triggered()
{
    QString filename = 
        myFileDialog(QFileDialog::AcceptSave, "Save Style", "Style Files (*.sty)", _last_style_dir );

    if (!filename.isNull())
    {
        QString rel_name = _working_dir.relativeFilePath(filename);

        QFileInfo info(rel_name);
        if (info.suffix() != "sty")
        {
            rel_name += ".sty";
        }

        if (saveStyle( rel_name ))
        {
            _stylename = rel_name;
        }
    }
}

void MainWindow::on_actionOpen_Scene_triggered()
{
    QString filename = 
        myFileDialog(QFileDialog::AcceptOpen, "Open Scene", 
        "Scenes (*.dps *.kmz *.dae)", _last_scene_dir );

    if (!filename.isNull())
    {
        if (openScene( filename ))
        {
            _glViewer->forceFullRedraw();
        }
    }
}

void MainWindow::on_actionSave_Scene_triggered()
{
    if (_scenename.endsWith("dps"))
        saveScene( _scenename );
    else
        on_actionSave_Scene_As_triggered();
}

void MainWindow::on_actionSave_Scene_As_triggered()
{
    QString filename = 
        myFileDialog(QFileDialog::AcceptSave, "Save Scene", "Scene Files (*.dps)", _last_scene_dir );

    if (!filename.isNull())
    {
        QFileInfo info(filename);
        if (info.suffix() != "dps")
        {
            filename += ".dps";
        }

        if (saveScene( filename ))
        {
            _scenename = filename;
            makeWindowTitle();
        }
    }
}

void MainWindow::on_actionReload_Shaders_triggered()
{
    GQShaderManager::reload();
    _glViewer->updateGL();
}

void MainWindow::on_actionLighting_Lambertian_triggered()
{
    NPRLight *light = getCurrentLight();
    if (light)    
        light->setMode(NPR_LIGHT_LAMBERTIAN);
}
void MainWindow::on_actionLighting_Front_and_Back_triggered()
{
    NPRLight *light = getCurrentLight();
    if (light)
        light->setMode(NPR_LIGHT_FRONT_BACK);
}
void MainWindow::on_actionLighting_Warm_and_Cool_triggered()
{
    NPRLight *light = getCurrentLight();
    if (light)
        light->setMode(NPR_LIGHT_WARM_COOL);
}

void MainWindow::on_actionFlat_Shading_toggled( bool checked )
{
    Q_UNUSED(checked);
    //_npr_scene->setFlatShading( checked );
}

bool MainWindow::saveStyle( const QString& filename )
{
    if (!filename.isNull() && getCurrentStyle()->save(_working_dir.absoluteFilePath(filename)))
    {
        printf("Wrote style file: %s\n", qPrintable(filename));
        return true;
    }
    QMessageBox::critical(this, "Save Failed", QString("Could not save style: \"%1\"").arg(filename));
    return false;
}

void MainWindow::makeWindowTitle()
{
    QFileInfo fileinfo(_scenename);
    QString title = QString("dpix - %1").arg( fileinfo.fileName() );
    setWindowTitle( title );
}

void MainWindow::setFocusMode( int mode )
{
    NPRSettings::instance().set(NPR_FOCUS_MODE, mode);
}

void MainWindow::on_focusOffButton_clicked()
{
    setFocusMode(NPR_FOCUS_NONE);
    _glViewer->forceFullRedraw();
}

void MainWindow::on_focus2DButton_clicked()
{
    setFocusMode(NPR_FOCUS_SCREEN);
    _glViewer->forceFullRedraw();
}

void MainWindow::on_focus3DButton_clicked()
{
    setFocusMode(NPR_FOCUS_WORLD);
    _glViewer->forceFullRedraw();
}

void MainWindow::on_farSlider_valueChanged( int value )
{
    getCurrentStyle()->transferRef( (NPRStyle::TransferFunc)_currentTransferIndex ).vfar = (float)value / 100;
    _glViewer->updateGL(); 
}

void MainWindow::on_nearSlider_valueChanged( int value )
{
    getCurrentStyle()->transferRef( (NPRStyle::TransferFunc)_currentTransferIndex ).vnear = (float)value / 100;
    _glViewer->updateGL(); 
}

void MainWindow::on_v1Slider_valueChanged( int value )
{
    getCurrentStyle()->transferRef( (NPRStyle::TransferFunc)_currentTransferIndex ).v1 = (float)value / 100;
    _glViewer->updateGL(); 
}

void MainWindow::on_v2Slider_valueChanged( int value )
{
    getCurrentStyle()->transferRef( (NPRStyle::TransferFunc)_currentTransferIndex ).v2 = (float)value / 100;
    _glViewer->updateGL(); 
}

void MainWindow::on_transferComboBox_currentIndexChanged( int index )
{
    _currentTransferIndex = index;

    updateUiFromStyle();
}

void MainWindow::updateUiFromStyle()
{
    NPRStyle* current_style = getCurrentStyle();

    if (current_style)
    {
        const NPRTransfer& currentTrans = current_style->transfer((NPRStyle::TransferFunc)_currentTransferIndex);

        _ui.v1Slider->setValue( (int)(currentTrans.v1 * 100) );
        _ui.v2Slider->setValue( (int)(currentTrans.v2 * 100) );
        _ui.nearSlider->setValue( (int)(currentTrans.vnear * 100) );
        _ui.farSlider->setValue( (int)(currentTrans.vfar * 100) );

        if (current_style->paperTexture())
        {
            _ui.actionDraw_Paper_Texture->setEnabled( true );
            _ui.actionDraw_Paper_Texture->setChecked( 
                NPRSettings::instance().get(NPR_ENABLE_PAPER_TEXTURE));
        }
        else
        {
            _ui.actionDraw_Paper_Texture->setEnabled( false );
            _ui.actionDraw_Paper_Texture->setChecked( false ); 
        }

        if (current_style->backgroundTexture())
        {
            _ui.actionDraw_Background_Texture->setEnabled( true );
            _ui.actionDraw_Background_Texture->setChecked( 
                NPRSettings::instance().get(NPR_ENABLE_BACKGROUND_TEXTURE));
        }
        else
        {
            _ui.actionDraw_Background_Texture->setEnabled( false );
            _ui.actionDraw_Background_Texture->setChecked( false ); 
        }

        const NPRPenStyle* penstyle = current_style->penStyle(_ui.penStyleTypeBox->currentText());
        const NPRPenStyle* basepenstyle = current_style->penStyle(0);

        QString color_frame_style = "border: 1px solid black;background-color: rgb(%1,%2,%3)";
        int r = penstyle->color()[0]*255;
        int g = penstyle->color()[1]*255;
        int b = penstyle->color()[2]*255;
        _ui.penColorFrame->setStyleSheet( color_frame_style.arg(r).arg(g).arg(b) );

        _ui.penTextureEditBox->setText( penstyle->textureFile() );
        _ui.penTextureEditBox->home(false);
        _ui.penWidthBox->setValue(basepenstyle->stripWidth());
        _ui.penLengthScaleBox->setValue(basepenstyle->lengthScale());
        _ui.penOpacityBox->setValue(penstyle->opacity());
        _ui.penElisionWidthBox->setValue(penstyle->elisionWidth());
        _ui.drawInvisibleCheckBox->setChecked(current_style->drawInvisibleLines());
        _ui.enableLineElisionCheckBox->setChecked(current_style->enableLineElision());
    }
}

void MainWindow::updateUiFromScene()
{
    _ui.sceneGraphTreeView->setModel(_npr_scene->itemModel());
    _ui.sceneGraphTreeView->header()->hide();

    connect(_ui.sceneGraphTreeView->selectionModel(), 
        SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), 
        this, 
        SLOT(sceneGraphTreeViewSelectionChanged(const QItemSelection&, const QItemSelection&)));

    _ui.actionUse_VBOs_for_Geometry->setChecked(_npr_scene->areVBOsEnabled());

    const NPRLight* light = _npr_scene->light(0);
    _ui.actionLighting_Ambient_Component->setChecked(light->isAmbientEnabled());
    _ui.actionLighting_Diffuse_Component->setChecked(light->isDiffuseEnabled());
    _ui.actionLighting_Specular_Component->setChecked(light->isSpecularEnabled());
}

void MainWindow::updateUiFromViewer()
{
    _ui.lightingPositionComboBox->setCurrentIndex(_glViewer->lightPreset());
    _ui.lightingDepthSlider->setValue(_glViewer->lightDepth()*1000);
}

void MainWindow::updateUiFromSettings()
{
    NPRSettings& settings = NPRSettings::instance();
    _ui.actionDraw_Lines->setChecked(settings.get(NPR_ENABLE_LINES));
    _ui.actionEnable_Stylized_Lines->setChecked(settings.get(NPR_ENABLE_STYLIZED_LINES));
    _ui.actionDraw_Polygons->setChecked(settings.get(NPR_ENABLE_POLYGONS));
    _ui.actionDraw_Transparent_Faces->setChecked(settings.get(NPR_ENABLE_TRANSPARENT_POLYGONS));
    _ui.actionDraw_Paper_Texture->setChecked(settings.get(NPR_ENABLE_PAPER_TEXTURE));
    _ui.actionDraw_Background_Texture->setChecked(settings.get(NPR_ENABLE_BACKGROUND_TEXTURE));
    _ui.actionEnable_Cutaway->setChecked(settings.get(NPR_ENABLE_CUTAWAY));
    _ui.actionDraw_Cutaway_Lines->setChecked(settings.get(NPR_ENABLE_CUTAWAY_LINES));
    _ui.actionEnable_Line_Artmaps->setChecked(settings.get(NPR_ENABLE_LINE_ARTMAPS));
    _ui.actionUse_VBOs_for_Geometry->setChecked(settings.get(NPR_ENABLE_VBOS));
    _ui.actionEnable_Lighting->setChecked(settings.get(NPR_ENABLE_LIGHTING));

    _ui.actionCheck_Visibility->setChecked(settings.get(NPR_CHECK_LINE_VISIBILITY));
    _ui.actionCheck_Priority->setChecked(settings.get(NPR_CHECK_LINE_PRIORITY));
    _ui.actionFilter_Visibility->setChecked(settings.get(NPR_FILTER_LINE_VISIBILITY));
    _ui.actionFilter_Priority->setChecked(settings.get(NPR_FILTER_LINE_PRIORITY));

    _ui.actionShow_Item_Buffer->setChecked(settings.get(NPR_VIEW_ITEM_BUFFER));
    _ui.actionShow_Priority_Buffer->setChecked(settings.get(NPR_VIEW_PRIORITY_BUFFER));
    _ui.actionShow_Clipping_Buffer->setChecked(settings.get(NPR_VIEW_CLIP_BUFFER));
    _ui.actionShow_Tristrips->setChecked(settings.get(NPR_VIEW_TRI_STRIPS));

    _ui.actionDraw_Occluding_Contours->setChecked(settings.get(NPR_EXTRACT_CONTOURS));
    _ui.actionDraw_Suggestive_Contours->setChecked(settings.get(NPR_EXTRACT_SUGGESTIVE_CONTOURS));
    _ui.actionDraw_Ridges->setChecked(settings.get(NPR_EXTRACT_RIDGES));
    _ui.actionDraw_Valleys->setChecked(settings.get(NPR_EXTRACT_VALLEYS));
    _ui.actionDraw_Isophotes->setChecked(settings.get(NPR_EXTRACT_ISOPHOTES));
    _ui.actionDraw_Boundaries->setChecked(settings.get(NPR_EXTRACT_BOUNDARIES));
    _ui.actionDraw_Apparent_Ridges->setChecked(settings.get(NPR_EXTRACT_APPARENT_RIDGES));
    _ui.actionDraw_Profiles->setChecked(settings.get(NPR_EXTRACT_PROFILES));
    _ui.actionClassify_Boundaries->setChecked(settings.get(NPR_CLASSIFY_BOUNDARIES));
    _ui.actionColor_Lines_By_ID->setChecked(settings.get(NPR_COLOR_LINES_BY_ID));

    _ui.actionCompute_PVS->setChecked(settings.get(NPR_COMPUTE_PVS));

    if (_npr_scene)
    {
        _ui.actionUse_VBOs_for_Geometry->setChecked(_npr_scene->areVBOsEnabled());
    }

    NPRLineVisibilityMethod viz_method = 
        (NPRLineVisibilityMethod)(settings.get(NPR_LINE_VISIBILITY_METHOD));
    switch(viz_method)
    {
        case NPR_SEGMENT_ATLAS: 
            _ui.itembufferSegmentAtlasButton->setChecked(true); break;
        case NPR_SPINE_TEST: 
            _ui.itembufferSpineTestButton->setChecked(true); break;
        default: break;
    }

    int supersamples = settings.get(NPR_LINE_VISIBILITY_SUPERSAMPLE);
    switch (supersamples)
    {
        case 1 : _ui.lowQualityButton->setChecked(true); break;
        case 9 : _ui.mediumQualityButton->setChecked(true); break;
        case 16 : _ui.highQualityButton->setChecked(true); break;
        default: break;
    }

    int focus_mode = settings.get(NPR_FOCUS_MODE); 
    switch (focus_mode)
    {
        case NPR_FOCUS_NONE : _ui.focusOffButton->setChecked(true); break;
        case NPR_FOCUS_SCREEN : _ui.focus2DButton->setChecked(true); break;
        case NPR_FOCUS_WORLD : _ui.focus3DButton->setChecked(true); break;
    }
}

void MainWindow::updateWindowMenu()
{
    _ui.actionShow_Pen_Style_Widget->setChecked(_ui.penStyleDockWidget->isVisible());
    _ui.actionShow_Line_Visibility_Widget->setChecked(_ui.lineVisibilityDockWidget->isVisible());
    _ui.actionShow_Lighting_Widget->setChecked(_ui.lightingDockWidget->isVisible());
    _ui.actionShow_Stylized_Focus_Widget->setChecked(_ui.focusDockWidget->isVisible());
    _ui.actionShow_Scene_Graph_Widget->setChecked(_ui.sceneDockWidget->isVisible());
    _ui.actionShow_Statistics_Widget->setChecked(_ui.statsDockWidget->isVisible());
}

void MainWindow::updateRecentScenesMenu()
{
    for (int i=0; i < MAX_RECENT_SCENES; i++)
    {
        if (_recent_scenes.size() > i && !_recent_scenes[i].isEmpty())
        {
            if (_recent_scenes_actions[i])
            {
                _recent_scenes_actions[i]->setText(_recent_scenes[i]);
            }
            else
            {
                // doesn't seem like the SLOT macro can take a variable argument;
                // it must be a literal string specified in the code (?)
                switch(i)
                {
                    case 0: _recent_scenes_actions[0] = _ui.menuRecent_Scenes->addAction( 
                                _recent_scenes[0], this, SLOT(onmy_recentScene0_triggered()) );
                            break;
                    case 1: _recent_scenes_actions[1] = _ui.menuRecent_Scenes->addAction( 
                                _recent_scenes[1], this, SLOT(onmy_recentScene1_triggered()) );
                            break;
                    case 2: _recent_scenes_actions[2] = _ui.menuRecent_Scenes->addAction( 
                                _recent_scenes[2], this, SLOT(onmy_recentScene2_triggered()) );
                            break;
                    case 3: _recent_scenes_actions[3] = _ui.menuRecent_Scenes->addAction( 
                                _recent_scenes[3], this, SLOT(onmy_recentScene3_triggered()) );
                            break;
                }
                _recent_scenes_actions[i]->setShortcut(Qt::CTRL + Qt::Key_1 + i);
            }
        }
    }
}

void MainWindow::hideAllDockWidgets()
{
    _ui.actionShow_Pen_Style_Widget->setChecked(false);
    _ui.actionShow_Line_Visibility_Widget->setChecked(false);
    _ui.actionShow_Lighting_Widget->setChecked(false);
    _ui.actionShow_Stylized_Focus_Widget->setChecked(false);
    _ui.actionShow_Scene_Graph_Widget->setChecked(false);
    _ui.actionShow_Statistics_Widget->setChecked(false);
}

void MainWindow::on_actionShow_Line_Visibility_Widget_toggled( bool checked )
{
    _ui.lineVisibilityDockWidget->setVisible(checked);
}

void MainWindow::on_actionShow_Scene_Graph_Widget_toggled( bool checked )
{
    _ui.sceneDockWidget->setVisible(checked);
}

void MainWindow::on_actionShow_Pen_Style_Widget_toggled( bool checked )
{
    _ui.penStyleDockWidget->setVisible(checked);
}

void MainWindow::on_actionShow_Lighting_Widget_toggled( bool checked )
{
    _ui.lightingDockWidget->setVisible(checked);
}

void MainWindow::on_actionShow_Stylized_Focus_Widget_toggled( bool checked )
{
    _ui.focusDockWidget->setVisible(checked);
}


void MainWindow::on_actionShow_Statistics_Widget_toggled( bool checked )
{
    _glViewer->setTimersAreDisplayed(checked);
    _ui.statsDockWidget->setVisible(checked);
}

void MainWindow::on_itembufferSegmentAtlasButton_clicked()
{
    updateLineVisibilitySettingsFromUi();
    _glViewer->forceFullRedraw();
}

void MainWindow::on_itembufferSpineTestButton_clicked()
{
    updateLineVisibilitySettingsFromUi();
    _glViewer->forceFullRedraw();
}

void MainWindow::on_lowQualityButton_clicked()
{
    updateLineVisibilitySettingsFromUi();
    _glViewer->forceFullRedraw();
}
void MainWindow::on_mediumQualityButton_clicked()
{
    updateLineVisibilitySettingsFromUi();
    _glViewer->forceFullRedraw();
}
void MainWindow::on_highQualityButton_clicked()
{
    updateLineVisibilitySettingsFromUi();
    _glViewer->forceFullRedraw();
}


void MainWindow::on_actionCamera_Perspective_toggled(bool checked)
{
    if (checked) {
        _glViewer->camera()->setType(qglviewer::Camera::PERSPECTIVE);
    }
    else {
        _glViewer->camera()->setType(qglviewer::Camera::ORTHOGRAPHIC);
    }
    _glViewer->updateGL();
}

void MainWindow::on_actionSave_Screenshot_triggered()
{
    _glViewer->saveSnapshot( false );
}

void MainWindow::on_lightingPositionComboBox_activated(int index)
{
    if (index >= 0 && index < GLViewer::NUM_LIGHT_PRESETS) {
        _glViewer->setLightPreset((GLViewer::LightPreset)index);
        _glViewer->updateGL();
    }
}

void MainWindow::on_lightingDepthSlider_valueChanged(int value)
{
    _glViewer->setLightDepth((float)value / 1000.0f);
    _glViewer->updateGL();
}

void MainWindow::on_actionLighting_Ambient_Component_toggled( bool checked )
{
    _npr_scene->light(0)->setEnableAmbient(checked);
    _glViewer->updateGL();
}

void MainWindow::on_actionLighting_Diffuse_Component_toggled( bool checked )
{
    _npr_scene->light(0)->setEnableDiffuse(checked);
    _glViewer->updateGL();
}
void MainWindow::on_actionLighting_Specular_Component_toggled( bool checked )
{
    _npr_scene->light(0)->setEnableSpecular(checked);
    _glViewer->updateGL();
}

void MainWindow::on_actionEnable_Lighting_toggled( bool checked )
{
    setBoolSetting(NPR_ENABLE_LIGHTING, checked);
    _glViewer->updateGL();
}

void MainWindow::on_actionUse_VBOs_for_Geometry_toggled( bool checked )
{
    setBoolSetting(NPR_ENABLE_VBOS, checked);
    if (_npr_scene)
        _npr_scene->updateVBOs();
    _glViewer->updateGL();
}
    
void MainWindow::setFoV(float degrees)
{
    _glViewer->camera()->setFieldOfView( degrees * ( 3.1415926f / 180.0f ) );
    _npr_scene->setFieldOfView(degrees * ( 3.1415926f / 180.0f ) );
    _glViewer->updateGL();
}

void MainWindow::on_actionPaper_Texture_triggered()
{
    QString filename = 
        QFileDialog::getOpenFileName( this, "Open Paper Texture", ".", "Images (*.ppm *.bmp *.png *.jpg)" );

    if (!filename.isNull())
    {
        QString rel_name = _working_dir.relativeFilePath(filename);

        getCurrentStyle()->loadPaperTexture(rel_name);
        updateUiFromStyle();
        _glViewer->updateGL();
    }
}

void MainWindow::on_actionBackground_Texture_triggered()
{
    QString filename = 
        QFileDialog::getOpenFileName( this, "Open Background Texture", ".", "Images (*.ppm *.bmp *.png *.jpg)" );

    if (!filename.isNull())
    {
        QString rel_name = _working_dir.relativeFilePath(filename);

        getCurrentStyle()->loadBackgroundTexture(rel_name);
        updateUiFromStyle();
        _glViewer->updateGL();
    }
}

void MainWindow::on_actionDraw_Paper_Texture_toggled( bool checked )
{
    if (_ui.actionDraw_Paper_Texture->isEnabled())
    {
        NPRSettings::instance().set(NPR_ENABLE_PAPER_TEXTURE, checked);
        _glViewer->updateGL();
    }
}

void MainWindow::on_actionDraw_Background_Texture_toggled( bool checked )
{
    if (_ui.actionDraw_Background_Texture->isEnabled())
    {
        NPRSettings::instance().set(NPR_ENABLE_BACKGROUND_TEXTURE, checked);
        _glViewer->updateGL();
    }
}

void MainWindow::on_actionPen_Texture_triggered()
{
    changePenTexture("Base Style");
    
}

void MainWindow::on_actionBackground_Color_triggered()
{
    const vec &current_color = getCurrentStyle()->backgroundColor();
    QColor current_qcolor;
    current_qcolor.setRgbF(current_color[0], current_color[1], current_color[2]);
    
    QColor qc = QColorDialog::getColor(current_qcolor);
    if (qc.isValid())
    {
        vec color = vec(qc.redF(), qc.greenF(), qc.blueF());
        getCurrentStyle()->setBackgroundColor(color);
        _glViewer->updateGL();
    }
}

void MainWindow::on_actionPen_Color_triggered()
{
    changePenColor("Base Style");

    QColor qc = QColorDialog::getColor();
    if (qc.isValid())
    {
        vec color = vec(qc.redF(), qc.greenF(), qc.blueF());
        getCurrentStyle()->penStyle(0)->setColor(color);
        _glViewer->updateGL();
    }
}

void MainWindow::changeSessionState( int newstate )
{
    _glViewer->setResetTimersEachFrame(true);

    if (newstate == SESSION_NONE)
    {
        _ui.actionStart_Recording->setEnabled(true);
        _ui.actionStop_Recording->setEnabled(false);
        _ui.actionReplay_Session->setEnabled(false);
        _ui.actionReplay_as_fast_as_possible->setEnabled(false);
        _ui.actionReplay_and_Save_Paths->setEnabled(false);
        _ui.actionReplay_and_Save_Screenshots->setEnabled(false);
        _ui.actionReplay_and_Save_Movie->setEnabled(false);
        _ui.actionStop_Replaying->setEnabled(false);
        _ui.actionOpen_Session->setEnabled(true);
        _ui.actionSave_Session->setEnabled(false);
    }
    else if (newstate == SESSION_LOADED)
    {
        _ui.actionStart_Recording->setEnabled(true);
        _ui.actionStop_Recording->setEnabled(false);
        _ui.actionReplay_Session->setEnabled(true);
        _ui.actionReplay_as_fast_as_possible->setEnabled(true);
        _ui.actionReplay_and_Save_Paths->setEnabled(true);
        _ui.actionReplay_and_Save_Screenshots->setEnabled(true);
        _ui.actionReplay_and_Save_Movie->setEnabled(true);
        _ui.actionStop_Replaying->setEnabled(false);
        _ui.actionOpen_Session->setEnabled(true);
        _ui.actionSave_Session->setEnabled(true);

        disconnect( _current_session, SIGNAL(playbackFinished()), this, SLOT(onmy_replayFinished()) );
    }
    else if (newstate == SESSION_PLAYING)
    {
        _ui.actionStart_Recording->setEnabled(false);
        _ui.actionStop_Recording->setEnabled(false);
        _ui.actionReplay_Session->setEnabled(false);
        _ui.actionReplay_as_fast_as_possible->setEnabled(false);
        _ui.actionReplay_and_Save_Paths->setEnabled(false);
        _ui.actionReplay_and_Save_Screenshots->setEnabled(false);
        _ui.actionReplay_and_Save_Movie->setEnabled(false);
        _ui.actionStop_Replaying->setEnabled(true);
        _ui.actionOpen_Session->setEnabled(false);
        _ui.actionSave_Session->setEnabled(false);

        _glViewer->setResetTimersEachFrame(false);

        connect( _current_session, SIGNAL(playbackFinished()), this, SLOT(onmy_replayFinished()) );
    }
    else if (newstate == SESSION_RECORDING)
    {
        _ui.actionStart_Recording->setEnabled(false);
        _ui.actionStop_Recording->setEnabled(true);
        _ui.actionReplay_Session->setEnabled(false);
        _ui.actionReplay_as_fast_as_possible->setEnabled(false);
        _ui.actionReplay_and_Save_Paths->setEnabled(false);
        _ui.actionReplay_and_Save_Screenshots->setEnabled(false);
        _ui.actionReplay_and_Save_Movie->setEnabled(false);
        _ui.actionStop_Replaying->setEnabled(false);
        _ui.actionOpen_Session->setEnabled(false);
        _ui.actionSave_Session->setEnabled(false);
    }

    _session_state = newstate;
}

void MainWindow::onmy_replayFinished()
{
    changeSessionState( SESSION_LOADED );
}

void MainWindow::on_actionStart_Recording_triggered()
{
    _current_session = new Session();
    _current_session->startRecording( _glViewer, &_ui );
    changeSessionState( SESSION_RECORDING );
}

void MainWindow::on_actionStop_Recording_triggered()
{
    changeSessionState( SESSION_LOADED );
    _current_session->stopRecording();
}

void MainWindow::on_actionReplay_Session_triggered()
{
    changeSessionState( SESSION_PLAYING );
    _current_session->startPlayback( _glViewer, &_ui );
}

void MainWindow::on_actionReplay_as_fast_as_possible_triggered()
{
    changeSessionState( SESSION_PLAYING );
    _current_session->startPlayback( _glViewer, &_ui, Session::PLAYBACK_AS_FAST_AS_POSSIBLE );
}


void MainWindow::on_actionReplay_and_Save_Screenshots_triggered()
{
    QString filename = 
        QFileDialog::getSaveFileName( this, "Save Screenshots", ".", "Images (*.png *.jpg)" );

    if (!filename.isNull())
    {
        changeSessionState( SESSION_PLAYING );
        _current_session->startPlayback( _glViewer, &_ui, 
            Session::PLAYBACK_SCREENSHOTS_INTERPOLATED, filename );
    }
}

void MainWindow::on_actionReplay_and_Save_Movie_triggered()
{
    QString filename = 
        QFileDialog::getSaveFileName( this, "Save Movie", ".", "Movies (*.mov *.avi *.mpg *.mp4)" );

    if (!filename.isNull())
    {
        changeSessionState( SESSION_PLAYING );
        _current_session->startPlayback( _glViewer, &_ui, Session::PLAYBACK_MOVIE, filename );
    }
}

void MainWindow::on_actionStop_Replaying_triggered()
{
    _current_session->stopPlayback();
    changeSessionState( SESSION_LOADED );
}

void MainWindow::on_actionReplay_Settings_triggered()
{
    Session::execSettingsDialog();
}


void MainWindow::resizeToFitViewerSize( int x, int y )
{
    QSize currentsize = size();
    QSize viewersize = _glViewer->size();
    QSize newsize = currentsize - viewersize + QSize(x,y);
    resize( newsize );
}

NPRLight* MainWindow::getCurrentLight()
{
    if (_npr_scene)
    {
        return _npr_scene->light(0);
    }
    else
    {
        return NULL;
    }
}

NPRStyle* MainWindow::getCurrentStyle()
{
    if (_npr_scene)
        return _npr_scene->style();
    else
        return NULL;
}


void MainWindow::on_penStyleTypeBox_currentIndexChanged( const QString& text )
{
    if (text == "Base Style")
    {
        _ui.penStyleSameAsBaseCheck->setEnabled(false);
        _ui.penStyleSameAsBaseCheck->setChecked(true);
        _ui.penStyleControlFrame->setEnabled(true);
    }
    else
    {
        if (getCurrentStyle()->hasPenStyle(text))
        {
            _ui.penStyleSameAsBaseCheck->setEnabled(true);
            _ui.penStyleSameAsBaseCheck->setChecked(false);
            _ui.penStyleControlFrame->setEnabled(true);
        }
        else
        {
            _ui.penStyleSameAsBaseCheck->setEnabled(true);
            _ui.penStyleSameAsBaseCheck->setChecked(true);
            _ui.penStyleControlFrame->setEnabled(false);
        }
    }
    updateUiFromStyle();
}

void MainWindow::on_penStyleSameAsBaseCheck_toggled(bool checked)
{
    NPRStyle* style = getCurrentStyle();
    QString name = _ui.penStyleTypeBox->currentText();
    if (checked)
    {
        if (name != "Base Style" && style->hasPenStyle(name))
        {
            style->deletePenStyle(name);
            _ui.penStyleControlFrame->setEnabled(false);
            updateUiFromStyle();
        }
    }
    else
    {
        if (!style->hasPenStyle(name))
        {
            NPRPenStyle* newstyle = new NPRPenStyle();
            newstyle->copyFrom(*(style->penStyle(0)));
            newstyle->setName(name);
            style->addPenStyle(newstyle);
            _ui.penStyleControlFrame->setEnabled(true);
        }
    }
}

void MainWindow::on_penColorBtn_clicked()
{
    changePenColor(_ui.penStyleTypeBox->currentText());
    updateUiFromStyle();
}

void MainWindow::on_penTextureBtn_clicked()
{
    changePenTexture(_ui.penStyleTypeBox->currentText());
    updateUiFromStyle();
}

void MainWindow::on_penWidthBox_valueChanged(double value)
{
    /*QString name = _ui.penStyleTypeBox->currentText();
    getCurrentStyle()->penStyle(name)->setStripWidth(value);
    _glViewer->forceFullRedraw();*/
    getCurrentStyle()->penStyle(0)->setStripWidth(value);
    _glViewer->updateGL();
}

void MainWindow::on_penOpacityBox_valueChanged(double value)
{
    QString name = _ui.penStyleTypeBox->currentText();
    getCurrentStyle()->penStyle(name)->setOpacity(value);
    _glViewer->forceFullRedraw();
}

void MainWindow::on_penElisionWidthBox_valueChanged(double value)
{
    QString name = _ui.penStyleTypeBox->currentText();
    getCurrentStyle()->penStyle(name)->setElisionWidth(value);
    _glViewer->forceFullRedraw();
}

void MainWindow::on_penLengthScaleBox_valueChanged(double value)
{
    /*QString name = _ui.penStyleTypeBox->currentText();
    getCurrentStyle()->penStyle(name)->setLengthScale(value);
    _glViewer->forceFullRedraw();*/
    getCurrentStyle()->penStyle(0)->setLengthScale(value);
    _glViewer->updateGL();
}

void MainWindow::on_drawInvisibleCheckBox_toggled( bool value )
{
    getCurrentStyle()->setDrawInvisibleLines(value);
    _glViewer->updateGL();
}

void MainWindow::on_enableLineElisionCheckBox_toggled( bool value )
{
    getCurrentStyle()->setEnableLineElision(value);
    _glViewer->updateGL();
}

void MainWindow::on_cameraInterpSpeedBox_valueChanged( double value )
{
    for (int i = 0; i < 10; i++)
    {
        qglviewer::KeyFrameInterpolator* interper = _glViewer->camera()->keyFrameInterpolator(i);
        if (interper)
            interper->setInterpolationSpeed(value);
    }
}

void MainWindow::changePenTexture(const QString& pen_style)
{
    QString filename = 
        myFileDialog( QFileDialog::AcceptOpen, "Open Pen Texture", "Pen Textures (*.pgm *.png *.jpg *.3dt)", _last_texture_dir);

    if (!filename.isNull())
    {
        QString rel_name = _working_dir.relativeFilePath(filename);

        getCurrentStyle()->penStyle(pen_style)->setTexture(rel_name);
        _glViewer->updateGL();
    }
}

void MainWindow::changePenColor(const QString& pen_style)
{
    QColor qc = QColorDialog::getColor();
    if (qc.isValid())
    {
        vec color = vec(qc.redF(), qc.greenF(), qc.blueF());
        getCurrentStyle()->penStyle(pen_style)->setColor(color);
        _glViewer->updateGL();
    }
}

void MainWindow::sceneGraphTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (_npr_scene) {
        QModelIndexList deselectedList = deselected.indexes();
        NPRScene::NodeRefList nodeRefList;
        for (int i = 0; i < deselectedList.size(); i++) {
            nodeRefList << (NPRScene::NodeRef)(deselectedList[i].internalId());
        }
        _npr_scene->deselectTrees(nodeRefList);

        QModelIndexList selectedList = selected.indexes();
        nodeRefList.clear();
        for (int i = 0; i < selectedList.size(); i++) {
            nodeRefList << (NPRScene::NodeRef)(selectedList[i].internalId());
        }
        _npr_scene->selectTrees(nodeRefList);
        
        _npr_scene->updateDrawableLists();

		sceneUpdated();
    }
}




QString MainWindow::myFileDialog( int mode, const QString& caption, const QString& filter, 
                                  QString& last_dir)
{
    QFileDialog dialog(this, caption, last_dir, filter);
    dialog.setAcceptMode((QFileDialog::AcceptMode)mode);

    QString filename;
    int ret = dialog.exec();
    if (ret == QDialog::Accepted)
    {
        last_dir = dialog.directory().path();
        if (dialog.selectedFiles().size() > 0)
        {
            filename = dialog.selectedFiles()[0];
            if (mode == QFileDialog::AcceptSave)
            {
                QStringList acceptable_extensions;
                int last_pos = filter.indexOf("*.", 0);
                while (last_pos > 0)
                {
                    int ext_end = filter.indexOf(QRegExp("[ ;)]"), last_pos);
                    acceptable_extensions << filter.mid(last_pos+1, ext_end-last_pos-1);
                    last_pos = filter.indexOf("*.", last_pos+1);
                }
                if (acceptable_extensions.size() > 0)
                {
                    bool ext_ok = false;
                    for (int i = 0; i < acceptable_extensions.size(); i++)
                    {
                        if (filename.endsWith(acceptable_extensions[i]))
                        {
                            ext_ok = true;
                        }
                    }
                    if (!ext_ok)
                    {
                        filename = filename + acceptable_extensions[0];
                    }
                }
            }
        }
    }
    return filename;
}

void MainWindow::sceneUpdated() {
	if (_session_state != SESSION_PLAYING) {
		_glViewer->updateGL();
	}
}

void MainWindow::updateLineVisibilitySettingsFromUi()
{
    if (_current_renderer == _standard_renderer)
    {
        NPRLineVisibilityMethod viz_method;

        if (_ui.itembufferSegmentAtlasButton->isChecked()) 
        {
            viz_method = NPR_SEGMENT_ATLAS;
        }
        else if (_ui.itembufferSpineTestButton->isChecked()) 
        {
            viz_method = NPR_SPINE_TEST;
        }

        setIntSetting(NPR_LINE_VISIBILITY_METHOD, viz_method);

        if (_ui.lowQualityButton->isChecked()) 
        {
            setIntSetting(NPR_LINE_VISIBILITY_SUPERSAMPLE, 1);
            setFloatSetting(NPR_SEGMENT_ATLAS_DEPTH_SCALE, 1.0);
            setFloatSetting(NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y, 1.0);
        }
        else if (_ui.mediumQualityButton->isChecked()) 
        {
            setIntSetting(NPR_LINE_VISIBILITY_SUPERSAMPLE, 9);
            setFloatSetting(NPR_SEGMENT_ATLAS_DEPTH_SCALE, 2.0);
            setFloatSetting(NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y, 1.0);
        }
        else if (_ui.highQualityButton->isChecked()) 
        {
            setIntSetting(NPR_LINE_VISIBILITY_SUPERSAMPLE, 16);
            setFloatSetting(NPR_SEGMENT_ATLAS_DEPTH_SCALE, 3.0);
            setFloatSetting(NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y, 1.0);
        }
    }
}

void MainWindow::setBoolSetting(NPRBoolSetting name, bool value)
{
    NPRSettings::instance().set(name, value);
    _glViewer->forceFullRedraw();
}

void MainWindow::setIntSetting(NPRIntSetting name, int value)
{
    NPRSettings::instance().set(name, value);
    _glViewer->forceFullRedraw();
}

void MainWindow::setFloatSetting(NPRFloatSetting name, float value)
{
    NPRSettings::instance().set(name, value);
    _glViewer->forceFullRedraw();
}
