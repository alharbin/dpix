/*****************************************************************************\

Scene.h
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

Class for recording and playing back an editing session.

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#ifndef SESSION_H_
#define SESSION_H_

#include "Vec.h"
#include "XForm.h"
#include "timestamp.h"
#include "ui_Session.h"
#include "ui_Interface.h"
#include <vector>

#include <QDomElement>
#include <QString>

using std::vector;

class QGLViewer;
class Console;

class SessionFrame
{
public:
    ~SessionFrame();
	void load( const QDomElement& element );
	void save( QDomDocument& doc, QDomElement& element );

public:
	float	_time;
	xform	_camera_mat;
	int     _active_partition;
	float   _cutaway_angle;
	float	_cutaway_depth;
	bool	_cutaway_enable;
	bool	_cutaway_visible;
	bool	_anim_playing;
};

class Session : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        PLAYBACK_NORMAL,
        PLAYBACK_AS_FAST_AS_POSSIBLE,
        PLAYBACK_SCREENSHOTS,
        PLAYBACK_SCREENSHOTS_INTERPOLATED,
        PLAYBACK_MOVIE,
        NUM_PLAYBACK_MODES
    } PlaybackMode;

    typedef enum {
        STATE_NO_DATA,
        STATE_LOADED,
        STATE_PLAYING,
        STATE_RECORDING,
        NUM_STATES
    } State;

public:
    Session();

	bool load( const QString& filename );
    bool load( const QDomElement& root );
	bool save( const QString& filename );
    bool save( QDomDocument& doc, QDomElement& root );

	void startRecording( QGLViewer* viewer, Ui::MainWindow *ui );
    void stopRecording();

	void startPlayback( QGLViewer* viewer, Ui::MainWindow *ui, PlaybackMode mode = PLAYBACK_NORMAL, 
                                           const QString& filename = QString::null );
    void stopPlayback();

    State getState() const { return _state; }

	float               length() const;
	int					numFrames() const { return _frames.size(); }
	const SessionFrame& frame( int which ) const;

	void recordFrame( const SessionFrame& frame );

    static void execSettingsDialog();
    static void getFFMPEGFromSettings( QString& cmd, QStringList& input_options, QStringList& output_options, float& fps );
    static float getFPSFromSettings();
    static void setConsole( Console* console ) { _console = console; }

public slots:
    void gatherAndRecordFrame();
    void advancePlaybackFrame();
    void dumpScreenshot();

signals:
    void redrawNeeded();
    void recordingStarted();
    void recordingStopped();
    void playbackStarted();
    void playbackFinished();
    void playbackStopped();

protected:
    void convertReplayFramesToMovie();
    void interpolateScreenshotFrames();
    void cleanUpPlayback();

protected:
    State                _state;
    PlaybackMode         _playback_mode;
    QGLViewer*           _viewer;
	Ui::MainWindow*		 _ui_mainwindow;

	vector<SessionFrame> _frames;
    int                  _current_frame;
    bool                 _has_dumped_current_frame;

    timestamp            _session_timer;

    QString              _final_filename;
    QString              _screenshot_file_pattern;
    vector<QString>      _screenshot_filenames;

    static Console*      _console;
};

class SessionSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SessionSettingsDialog(); 

    QString getCommand() { return _ui.commandEdit->text(); }
    float   getFPS() { return _ui.fpsSpinBox->value(); }
    float   getBitrate() { return _ui.bitrateSpinBox->value(); }
    QString getInputOptionsString() { return _ui.inputOptionsLineEdit->text(); }
	QString getOutputOptionsString() { return _ui.outputOptionsLineEdit->text(); }

public slots:
    void on_browseCommandButton_clicked();
    void on_actionUpdateInputOptionsString_triggered();
    void on_actionUpdateOutputOptionsString_triggered();
    void on_actionUpdate_Example_triggered();

protected:
    Ui::SettingsDialog _ui;
};

#endif // SESSION_H_