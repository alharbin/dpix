/******************************************************************************\
 *                                                                            *
 *  filename : MainWindow.h                                                   *
 *  authors  : Forrester Cole                                                 *
 *                                                                            *
 *  Main window for viewfltk. Holds a FluidInterface widget. Handles user     *
 *  interface actions.                                                        *
 *                                                                            *
\******************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include "ui_Interface.h"
#include "Console.h"
#include "Scene.h"
#include "NPRSettings.h"

class GLViewer;
class NPRRenderer;
class NPRRendererStandard;
class NPRScene;
class NPRLight;
class NPRStyle;
class NPRDrawable;
class QSlider;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow( );
    ~MainWindow( );
    void init( const QDir& working_dir, const QString& scenename );
    GLViewer* getGLViewer() { return _glViewer; }

    void resizeToFitViewerSize( int x, int y );
    void setFoV(float degrees);

  protected:
    void closeEvent(QCloseEvent* event );

public slots:
    void on_actionShow_Tristrips_toggled( bool checked )           
        { setBoolSetting(NPR_VIEW_TRI_STRIPS, checked); }
    void on_actionShow_Clipping_Buffer_toggled( bool checked ) 
        { setBoolSetting(NPR_VIEW_CLIP_BUFFER, checked); };
    void on_actionShow_Priority_Buffer_toggled( bool checked ) 
        { setBoolSetting(NPR_VIEW_PRIORITY_BUFFER, checked); };
    void on_actionDraw_Lines_toggled( bool checked ) 
        { setBoolSetting(NPR_ENABLE_LINES, checked); };
    void on_actionDraw_Profiles_toggled( bool checked ) 
        { setBoolSetting(NPR_EXTRACT_PROFILES, checked); };
    void on_actionDraw_Polygons_toggled( bool checked ) 
        { setBoolSetting(NPR_ENABLE_POLYGONS, checked); };
    void on_actionDraw_Transparent_Faces_toggled( bool checked ) 
        { setBoolSetting(NPR_ENABLE_TRANSPARENT_POLYGONS, checked); };
    void on_actionCheck_Visibility_toggled( bool checked ) 
        { setBoolSetting(NPR_CHECK_LINE_VISIBILITY, checked); };
    void on_actionCheck_At_Spine_toggled( bool checked ) 
        { setBoolSetting(NPR_CHECK_LINE_VISIBILITY_AT_SPINE, checked); };
    void on_actionCheck_Priority_toggled( bool checked ) 
        { setBoolSetting(NPR_CHECK_LINE_PRIORITY, checked); };
    void on_actionFilter_Visibility_toggled( bool checked ) 
        { setBoolSetting(NPR_FILTER_LINE_VISIBILITY, checked); };
    void on_actionFilter_Priority_toggled( bool checked ) 
        { setBoolSetting(NPR_FILTER_LINE_PRIORITY, checked); };
    void on_actionShow_FPS_toggled( bool checked );
    void on_actionEnable_Stylized_Lines_toggled( bool checked ) 
        { setBoolSetting(NPR_ENABLE_STYLIZED_LINES, checked); };
    void on_actionEnable_Color_Blur_toggled( bool checked )
        { setBoolSetting(NPR_ENABLE_COLOR_BLUR, checked); }
    void on_actionColor_Lines_By_ID_toggled( bool checked )
        { setBoolSetting(NPR_COLOR_LINES_BY_ID, checked); }
    void on_actionCompute_PVS_toggled( bool checked )
        { setBoolSetting(NPR_COMPUTE_PVS, checked); }

    void on_actionOpen_Style_triggered();
    void on_actionSave_Style_triggered();
    void on_actionSave_Style_As_triggered();  
    void on_actionOpen_Scene_triggered();
    void on_actionSave_Scene_triggered();
    void on_actionSave_Scene_As_triggered();  
    void on_actionCamera_Perspective_toggled(bool checked);
    void on_actionSave_Screenshot_triggered();
    void on_actionReload_Shaders_triggered();
    void on_actionLighting_Lambertian_triggered();
    void on_actionLighting_Front_and_Back_triggered();
    void on_actionLighting_Warm_and_Cool_triggered();
    void on_actionFlat_Shading_toggled( bool checked );

    void on_actionStart_Recording_triggered();
    void on_actionStop_Recording_triggered();
    void on_actionReplay_Session_triggered();
    void on_actionReplay_as_fast_as_possible_triggered();
    void on_actionReplay_and_Save_Screenshots_triggered();
    void on_actionReplay_and_Save_Movie_triggered();
    void on_actionStop_Replaying_triggered();
    void on_actionReplay_Settings_triggered();

    void on_actionPaper_Texture_triggered();
    void on_actionBackground_Texture_triggered();
    void on_actionPen_Texture_triggered();
    void on_actionBackground_Color_triggered();
    void on_actionPen_Color_triggered();

    void on_actionDraw_Paper_Texture_toggled( bool checked ); 
    void on_actionDraw_Background_Texture_toggled( bool checked ); 

    void on_actionWindow_128x128_triggered() { resizeToFitViewerSize(128,128); }
    void on_actionWindow_256x256_triggered() { resizeToFitViewerSize(256,256); }
    void on_actionWindow_512x512_triggered() { resizeToFitViewerSize(512,512); }
    void on_actionWindow_640x480_triggered() { resizeToFitViewerSize(640,480); }
    void on_actionWindow_400x800_triggered() { resizeToFitViewerSize(400,800); }
    void on_actionWindow_600x800_triggered() { resizeToFitViewerSize(600,800); }
    void on_actionWindow_800x600_triggered() { resizeToFitViewerSize(800,600); }
    void on_actionWindow_800x800_triggered() { resizeToFitViewerSize(800,800); }
    void on_actionWindow_1024x512_triggered() { resizeToFitViewerSize(1024,512); }
    void on_actionWindow_1024x768_triggered() { resizeToFitViewerSize(1024,768); }
    void on_actionShow_Console_triggered() { _console->show(); }
    
    void on_actionLighting_Ambient_Component_toggled( bool checked );
    void on_actionLighting_Diffuse_Component_toggled( bool checked );
    void on_actionLighting_Specular_Component_toggled( bool checked );
    void on_actionEnable_Lighting_toggled( bool checked );
    void on_actionUse_VBOs_for_Geometry_toggled( bool checked );

    void on_actionCamera_FoV_30_Degrees_triggered() { setFoV(30); }
    void on_actionCamera_FoV_45_Degrees_triggered() { setFoV(45); }
    void on_actionCamera_FoV_90_Degrees_triggered() { setFoV(90); }

    void on_actionShow_Line_Visibility_Widget_toggled( bool checked );
    void on_actionShow_Scene_Graph_Widget_toggled( bool checked );
    void on_actionShow_Pen_Style_Widget_toggled( bool checked );
    void on_actionShow_Lighting_Widget_toggled( bool checked );
    void on_actionShow_Stylized_Focus_Widget_toggled( bool checked );
    void on_actionShow_Statistics_Widget_toggled( bool checked );

    void on_lineVisibilityDockWidget_visibilityChanged( bool visible ) { Q_UNUSED(visible); updateWindowMenu(); }
    void on_sceneDockWidget_visibilityChanged( bool visible ) {  Q_UNUSED(visible); updateWindowMenu(); }
    void on_penStyleDockWidget_visibilityChanged( bool visible ) {  Q_UNUSED(visible); updateWindowMenu(); }
    void on_focusDockWidget_visibilityChanged( bool visible ) {  Q_UNUSED(visible); updateWindowMenu(); }
    void on_lightingDockWidget_visibilityChanged( bool visible ) {  Q_UNUSED(visible); updateWindowMenu(); }
    void on_statsDockWidget_visibilityChanged( bool visible ) {  Q_UNUSED(visible); updateWindowMenu(); }

    void on_focusOffButton_clicked();
    void on_focus2DButton_clicked();
    void on_focus3DButton_clicked();
    
    void on_farSlider_valueChanged( int value );
    void on_nearSlider_valueChanged( int value );
    void on_v1Slider_valueChanged( int value );
    void on_v2Slider_valueChanged( int value );
    void on_transferComboBox_currentIndexChanged( int index );
    
    void on_lightingPositionComboBox_activated(int index);
    void on_lightingDepthSlider_valueChanged(int value);
    
    void on_itembufferSegmentAtlasButton_clicked();
    void on_itembufferSpineTestButton_clicked();

    void on_lowQualityButton_clicked();
    void on_mediumQualityButton_clicked();
    void on_highQualityButton_clicked();

    void on_penStyleTypeBox_currentIndexChanged( const QString& text );
    void on_penStyleSameAsBaseCheck_toggled(bool checked);
    void on_penColorBtn_clicked();
    void on_penTextureBtn_clicked();
    void on_penWidthBox_valueChanged(double value);
    void on_penOpacityBox_valueChanged(double value);
    void on_penElisionWidthBox_valueChanged(double value);
    void on_penLengthScaleBox_valueChanged(double value);

    void on_drawInvisibleCheckBox_toggled( bool value );
    void on_enableLineElisionCheckBox_toggled( bool value );

    void on_cameraInterpSpeedBox_valueChanged( double value );

    void onmy_replayFinished();

    void onmy_recentScene0_triggered() { openRecentScene(0); }
    void onmy_recentScene1_triggered() { openRecentScene(1); }
    void onmy_recentScene2_triggered() { openRecentScene(2); }
    void onmy_recentScene3_triggered() { openRecentScene(3); }
    
    void sceneGraphTreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected:
    bool saveStyle( const QString& filename );
    bool openScene( const QString& filename );
    bool saveScene( const QString& filename );
    bool openRecentScene( int which );
    void addCurrentSceneToRecentList();
    void makeWindowTitle();
    void updateUiFromStyle();
    void updateUiFromScene();
    void updateUiFromViewer();
    void updateUiFromSettings();
    void updateWindowMenu();
    void updateRecentScenesMenu();
    void hideAllDockWidgets();
    void updateLineVisibilitySettingsFromUi();
    void setBoolSetting(NPRBoolSetting name, bool value);
    void setIntSetting(NPRIntSetting name, int value);
    void setFloatSetting(NPRFloatSetting name, float value);

    QString myFileDialog( int mode, const QString& caption, const QString& filter, QString& last_dir );

    void changePenColor(const QString& pen_style);
    void changePenTexture(const QString& pen_style);

    void changeSessionState( int newstate );

    void clearRenderers();
    void allocRenderers();
  
	void sceneUpdated();

    NPRLight* getCurrentLight();
    NPRStyle* getCurrentStyle();

    void setFocusMode( int mode );


  private:
    GLViewer*		_glViewer;
    NPRRenderer*    _current_renderer;
    NPRRendererStandard*   _standard_renderer;
    NPRScene*       _npr_scene;
    Ui::MainWindow  _ui;
    Console*        _console;
    Scene*          _dpix_scene;

    QString   _stylename;
    QString   _scenename;

    QStringList _recent_scenes;
    vector<QAction*> _recent_scenes_actions;

    QDir        _working_dir;

    QString     _last_scene_dir;
    QString     _last_style_dir;
    QString     _last_texture_dir;
    QString     _last_export_dir;
    QString     _last_camera_dir;

    Session*  _current_session;
    int		  _session_state;
    static const int SESSION_NONE = 0;
    static const int SESSION_LOADED = 1;
    static const int SESSION_PLAYING = 2;
    static const int SESSION_RECORDING = 3;
    
    int       _currentTransferIndex;
};

#endif
