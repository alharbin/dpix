/*****************************************************************************\

Session.cc
Author: Forrester Cole (fcole@cs.princeton.edu)
Copyright (c) 2009 Forrester Cole

dpix is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include "Session.h"
#include <QFile>
#include <QDomDocument>
#include <QDomText>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <assert.h>
#include <qglviewer.h>
#include "Console.h"
#include "timestamp.h"
#include <QDebug>
#include <QProgressDialog>

#include "GQStats.h"

Console* Session::_console = 0;

SessionFrame::~SessionFrame()
{
}

void SessionFrame::load( const QDomElement& element )
{
	QDomElement t = element.firstChildElement("time");
	assert(!t.isNull());
	_time = t.text().toFloat();

	QDomElement cam = element.firstChildElement("camera_mat");
	assert(!cam.isNull());

	QString es = cam.text();
	QTextStream stream(&es);

	stream	>> _camera_mat[0] >> _camera_mat[4] >> _camera_mat[8] >> _camera_mat[12]
			>> _camera_mat[1] >> _camera_mat[5] >> _camera_mat[9] >> _camera_mat[13]
			>> _camera_mat[2] >> _camera_mat[6] >> _camera_mat[10] >> _camera_mat[14]
			>> _camera_mat[3] >> _camera_mat[7] >> _camera_mat[11] >> _camera_mat[15];	
}

void SessionFrame::save( QDomDocument& doc, QDomElement& element )
{
	QDomElement t = doc.createElement("time");
	QDomText ttext = doc.createTextNode(QString("%1").arg(_time) );

	element.appendChild(t);
	t.appendChild(ttext);

	QDomElement cam = doc.createElement("camera_mat");
	QString cam_string;
	QTextStream stream(&cam_string);
	
	stream	<< "\n" << _camera_mat[0] << " " << _camera_mat[4] << " " << _camera_mat[8] << " " << _camera_mat[12]
			<< "\n" << _camera_mat[1] << " " << _camera_mat[5] << " " << _camera_mat[9] << " " << _camera_mat[13]
			<< "\n" << _camera_mat[2] << " " << _camera_mat[6] << " " << _camera_mat[10] << " " << _camera_mat[14]
			<< "\n" << _camera_mat[3] << " " << _camera_mat[7] << " " << _camera_mat[11] << " " << _camera_mat[15] 
			<< "\n";

	QDomText camtext = doc.createTextNode(cam_string);
	element.appendChild(cam);
	cam.appendChild(camtext);
}

Session::Session()
{
    _viewer = 0;
    _state = STATE_NO_DATA;
    _playback_mode = PLAYBACK_NORMAL;
}

void Session::recordFrame( const SessionFrame& frame )
{
	int i = _frames.size();
	_frames.push_back(frame);
	if (i > 0)
    {
		_frames[i]._time = now() - _session_timer; 
    }
	else
    {
		_frames[i]._time = 0;
    }

    QString msg;
    QTextStream(&msg) << "Recording frame " << i << " at time " << _frames[i]._time << "\n";
    _console->print( msg );
}

const SessionFrame& Session::frame( int which ) const
{
	assert( which >= 0 && which < (int)(_frames.size()) );
	return _frames[which];
}

float Session::length() const
{
	if (_frames.size() == 0)
		return 0;
	else
		return _frames[_frames.size()-1]._time;
}

const int CURRENT_VERSION = 1;

bool Session::load( const QString& filename )
{
	QDomDocument doc("session");
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning("Could not open %s", qPrintable(filename));
		return false;
	}

	QString parse_errors;
	if (!doc.setContent(&file, &parse_errors))
	{
		qWarning("Parse errors: %s", qPrintable(parse_errors));
		return false;
	}

	file.close();

	QDomElement root = doc.documentElement();

    return load(root);
}

bool Session::load( const QDomElement& root )
{
    assert(_state == STATE_NO_DATA || _state == STATE_LOADED );

	QDomElement header = root.firstChildElement("header");
	assert(!header.isNull());
	QDomElement version = header.firstChildElement("version");
	assert(!version.isNull());

	int version_num = version.text().toInt();
	if (version_num != CURRENT_VERSION)
	{
		qWarning("Obsolete file version %d (current is %d)", version_num, CURRENT_VERSION );
		return false;
	}

	_frames.clear();
	QDomElement frames = root.firstChildElement("frames");
	QDomElement frame = frames.firstChildElement();
	while (!frame.isNull())
	{
		SessionFrame sf;
		sf.load( frame );
		_frames.push_back( sf );
		frame = frame.nextSiblingElement();
	}

    _state = STATE_LOADED;

	return true;
}

bool Session::save( const QString& filename )
{
	QDomDocument doc("session");
	QDomElement root = doc.createElement("session");
	doc.appendChild(root);

    bool ret = save(doc, root);
    if (!ret)
        return false;

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
	{
		qWarning("Could not open %s", qPrintable(filename));
		return false;
	}

	file.write(doc.toByteArray());

	file.close();

	return true;
}

bool Session::save( QDomDocument& doc, QDomElement& root )
{
    assert( _state == STATE_LOADED );

	QDomElement header = doc.createElement("header");
	root.appendChild(header);

	QDomElement version = doc.createElement("version");
	QDomText versiontext = doc.createTextNode( QString("%1").arg(CURRENT_VERSION) );
	header.appendChild(version);
	version.appendChild(versiontext);

	QDomElement frames = doc.createElement("frames");
	root.appendChild(frames);
	for (int i = 0; i < (int)(_frames.size()); i++)
	{
		QDomElement frame = doc.createElement("frame");
		_frames[i].save(doc, frame);
		frames.appendChild(frame);
	}

    return true;
}

void Session::startRecording( QGLViewer* viewer, Ui::MainWindow *ui )
{
    assert(_state == STATE_NO_DATA || _state == STATE_LOADED );

    _frames.clear();
    _viewer = viewer;
	_ui_mainwindow = ui;
    _state = STATE_RECORDING;
    _session_timer = now();

    connect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(gatherAndRecordFrame()) );
    emit recordingStarted();
}

void Session::stopRecording()
{
    assert( _state == STATE_RECORDING );

    disconnect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(gatherAndRecordFrame()) );
    _state = STATE_LOADED;
    _viewer = 0;

    emit recordingStopped();
}

void Session::startPlayback( QGLViewer* viewer, Ui::MainWindow *ui, PlaybackMode mode, const QString& filename )
{
    assert( _state == STATE_LOADED );

    _viewer = viewer;
	_ui_mainwindow = ui;
    _state = STATE_PLAYING;
    _playback_mode = mode;
    _current_frame = -1;
    _has_dumped_current_frame = false;
    _screenshot_file_pattern = "";
    _screenshot_filenames.clear();

    connect( this, SIGNAL( redrawNeeded() ), _viewer, SLOT( forceFullRedraw() ) );
    if (_playback_mode == PLAYBACK_SCREENSHOTS)
    {
        int extindex = filename.lastIndexOf('.');
        QTextStream(&_screenshot_file_pattern) 
            << filename.left(extindex) << "%04d" << filename.right(filename.length() - extindex);
        connect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );
    }
    else if (_playback_mode == PLAYBACK_SCREENSHOTS_INTERPOLATED ||
             _playback_mode == PLAYBACK_MOVIE)
    {
        _final_filename = filename;

        QDir temp = QDir("./temp");
        if (!temp.exists())
        {
            QDir work = QDir(".");
            work.mkdir("temp");
            temp = QDir("./temp");
        }

		QTextStream(&_screenshot_file_pattern) << temp.filePath("pre_interp_movie%04d.jpg");

        connect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );
    }

    QString msg;
    QTextStream(&msg) << "Replaying session (" << length() << " s. / " 
                      << numFrames() << " frames)...\n";
    _console->print( msg );

    emit playbackStarted();

    _session_timer = now();
    
    advancePlaybackFrame();
}

void Session::cleanUpPlayback()
{
    assert( _state == STATE_PLAYING );

    disconnect( this, SIGNAL( redrawNeeded() ), _viewer, SLOT( forceFullRedraw() ) );
    disconnect( _viewer, SIGNAL( drawFinished(bool)), this, SLOT(dumpScreenshot()) );

    _state = STATE_LOADED;
    _viewer = 0;
}

void Session::stopPlayback()
{
    cleanUpPlayback(); 

    _console->print("Replay stopped by user.\n");

    emit playbackStopped();
}

void Session::gatherAndRecordFrame()
{
    SessionFrame frame;
    GLdouble mv[16];
    _viewer->camera()->getModelViewMatrix(mv);
    frame._camera_mat = xform( mv );

    recordFrame( frame );
}

void Session::advancePlaybackFrame()
{
    if (_state != STATE_PLAYING)
        return;

    _current_frame++;
    _has_dumped_current_frame = false;

    const SessionFrame& current_frame = frame(_current_frame);
    _viewer->camera()->setFromModelViewMatrix( current_frame._camera_mat );
    _viewer->camera()->loadModelViewMatrix();

    emit redrawNeeded();

    float elapsed_time = now() - _session_timer;
    if (_current_frame < numFrames() - 1)
    {
        float time_until_next = frame(_current_frame+1)._time - elapsed_time;
        if (time_until_next < 0)
            time_until_next = 0;

        if (_playback_mode == PLAYBACK_NORMAL)
        {
            // realtime playback
            QTimer::singleShot( 1000 * time_until_next, this, SLOT(advancePlaybackFrame()));
        }
        else 
        {
            // playback as fast as possible
            QTimer::singleShot( 0, this, SLOT(advancePlaybackFrame()));
        }
    }
    else
    {
        cleanUpPlayback();

        if (_playback_mode == PLAYBACK_MOVIE)
            convertReplayFramesToMovie();
        else if (_playback_mode == PLAYBACK_SCREENSHOTS_INTERPOLATED)
            interpolateScreenshotFrames();

        _console->print(QString("Replay finished. fps: %1\n").arg((float)numFrames() / elapsed_time));

        emit playbackFinished();
    }
}

void Session::dumpScreenshot()
{
    if (!_has_dumped_current_frame)
    {
        QString filename;
        filename.sprintf(qPrintable(_screenshot_file_pattern), _current_frame);
        _screenshot_filenames.push_back(filename);
        
        _viewer->saveSnapshot( filename, true );

        _console->print( QString("Saved %1\n").arg(filename) );

        _has_dumped_current_frame = true;
    }
}

void Session::interpolateScreenshotFrames()
{
    float fps = getFPSFromSettings();

    int   num_frames = int(length() * fps);
    int   cur_input = 0; 

    QDir temp = QDir("./temp");
    assert(temp.exists());

    QProgressDialog progress("Interpolating Screenshots...", "Cancel", 0, num_frames+1, 0);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(2000);

    int extindex = _final_filename.lastIndexOf('.');
    QString prefix = _final_filename.left(extindex);
    vector<QString> tempfiles;
    for (int f = 0; f < num_frames + 1; f++)
    {
		while (f > int(frame(cur_input)._time * fps))
			cur_input++;

		QString targetfile = QString("%1%2.jpg").arg(prefix).arg(f, 5, 10, QLatin1Char('0'));
		qDebug() << _screenshot_filenames[cur_input] << "->" << targetfile << "for time" << frame(cur_input)._time;
        QFile::copy( _screenshot_filenames[cur_input], targetfile );
        progress.setValue(f);
    }
    progress.setValue(num_frames+1);

    // Clean up temp files.
    for (int i = 0; i < (int)(_screenshot_filenames.size()); i++)
        QFile::remove(_screenshot_filenames[i]);

    // rmdir will only succeed if the directory is empty.
    QDir work = QDir(".");
    work.rmdir("temp");
}

void Session::convertReplayFramesToMovie()
{
    QString ffmpeg_cmd;
    QStringList ffmpeg_input_options;
    QStringList ffmpeg_output_options;
    float fps;
    getFFMPEGFromSettings( ffmpeg_cmd, ffmpeg_input_options, ffmpeg_output_options, fps );

    int   num_frames = int(length() * fps);
    int   cur_input = 0; 

    QDir temp = QDir("./temp");
    assert(temp.exists());

    QProgressDialog progress("Interpolating Frames...", "Cancel", 0, num_frames+1, 0);
    int progress_counter = 0;
    int total_temp_files = 3*_screenshot_filenames.size();
    int total_progress = num_frames+1 + 1 /*ffmpeg*/ + total_temp_files + 1;
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(2000);

    vector<QString> tempfiles;
    for (int f = 0; f < num_frames + 1; f++)
    {
		while (f > int(frame(cur_input)._time * fps))
			cur_input++;

        QString targetbasename = QString("post_interp_movie%1.jpg").arg(f, 5, 10, QLatin1Char('0'));
		QString targetfile = temp.filePath(targetbasename);
        tempfiles.push_back(targetfile);
		qDebug() << _screenshot_filenames[cur_input] << "->" << targetfile << "for time" << frame(cur_input)._time;
#ifdef WIN32
        QFile::copy( _screenshot_filenames[cur_input], targetfile );
#else
        QFileInfo fileinfo(_screenshot_filenames[cur_input]);
        QFile::link( fileinfo.absoluteFilePath(), targetfile );
#endif
        progress.setValue(progress_counter++);
    }

    progress.setValue(progress_counter++);
    total_temp_files = tempfiles.size() + _screenshot_filenames.size();
    total_progress = num_frames+1 + 1 /*ffmpeg*/ + total_temp_files + 1;
    progress.setMaximum(total_progress);

    QStringList ffmpeg_args = ffmpeg_input_options;
    ffmpeg_args << "-i" << QString("./temp/post_interp_movie%05d.jpg") << ffmpeg_output_options << _final_filename;

#ifdef WIN32
    ffmpeg_cmd.replace("/", "\\");
    ffmpeg_args.replaceInStrings("/", "\\");
#endif

    progress.setLabelText("Running FFMPEG");

    _console->print( "Running ffmpeg...\n" );
    _console->print( QString("%1 %2\n\n").arg(ffmpeg_cmd).arg(ffmpeg_args.join(" === ")));
    _console->execute( ffmpeg_cmd, ffmpeg_args );

    _console->print( "Cleaning up...\n");

    progress.setValue(progress_counter++);
    progress.setLabelText("Cleaning temporary files...");

    // Clean up temp files.
    for (int i = 0; i < (int)(tempfiles.size()); i++)
    {
        QFile::remove(tempfiles[i]);
        progress.setValue(progress_counter++);
    }

    for (int i = 0; i < (int)(_screenshot_filenames.size()); i++)
    {
        QFile::remove(_screenshot_filenames[i]);
        progress.setValue(progress_counter++);
    }

    // rmdir will only succeed if the directory is empty.
    QDir work = QDir(".");
    work.rmdir("temp");
    
    progress.setValue(progress.maximum());
}

void Session::getFFMPEGFromSettings( QString& cmd, QStringList& input_options, QStringList& output_options, float& fps )
{
    QSettings settings("dpix", "dpix");

    settings.beginGroup("session/ffmpeg");
    cmd = settings.value("cmd", "ffmpeg").toString();

    QString input_options_string = settings.value("input_options", "").toString();
    QString output_options_string = settings.value("output_options", "").toString();
    // this should be changed to read the fps from the options string
    fps = settings.value("fps", 29.97f).toDouble();
    /*float bitrate = settings.value("bitrate", 200).toDouble();

    options << "-r"  << QString::number(fps) << "-b" << QString::number(bitrate);*/
    input_options << input_options_string.split(" ", QString::SkipEmptyParts);
    output_options << output_options_string.split(" ", QString::SkipEmptyParts);
}

float Session::getFPSFromSettings()
{
    QSettings settings("dpix", "dpix");

    settings.beginGroup("session/ffmpeg");
    return settings.value("fps", 29.97f).toDouble();
}

void Session::execSettingsDialog()
{
    SessionSettingsDialog dialog;

    if (dialog.exec() == QDialog::Accepted)
    {
        QSettings settings("dpix", "dpix");

        settings.beginGroup("session/ffmpeg");
        settings.setValue("cmd", dialog.getCommand()); 
        settings.setValue("fps", dialog.getFPS());
        settings.setValue("bitrate", dialog.getBitrate());
        settings.setValue("input_options", dialog.getInputOptionsString());
        settings.setValue("output_options", dialog.getOutputOptionsString());

        settings.sync();
    }
}


SessionSettingsDialog::SessionSettingsDialog()
{
    _ui.setupUi(this);

    QSettings settings("dpix", "dpix");

    settings.beginGroup("session/ffmpeg");
    QString cmd = settings.value("cmd", "ffmpeg").toString();

    float fps = settings.value("fps", 29.97f).toDouble();
    float bitrate = settings.value("bitrate", 200).toDouble();
    QString input_options_string = settings.value("input_options", "").toString();
    QString output_options_string = settings.value("output_options", "").toString();

    _ui.commandEdit->setText(cmd);
    _ui.fpsSpinBox->setValue(fps);
    _ui.bitrateSpinBox->setValue(bitrate);
    if (input_options_string.isEmpty())
        on_actionUpdateInputOptionsString_triggered();
    else
        _ui.inputOptionsLineEdit->setText(input_options_string);

    if (output_options_string.isEmpty())
        on_actionUpdateOutputOptionsString_triggered();
    else
        _ui.outputOptionsLineEdit->setText(output_options_string);

	on_actionUpdate_Example_triggered();
}

void SessionSettingsDialog::on_browseCommandButton_clicked()
{
#ifdef WIN32
    QString filename = 
        QFileDialog::getOpenFileName( this, "Find FFMPEG", ".", "Executable Files (*.exe)" );
#else
    QString filename = 
        QFileDialog::getOpenFileName( this, "Find FFMPEG", ".");
#endif

    if (!filename.isNull())
    {
        _ui.commandEdit->setText(filename);
    }
}

void SessionSettingsDialog::on_actionUpdateInputOptionsString_triggered()
{
    QString new_input_options;
	QTextStream(&new_input_options) << " -r " << _ui.fpsSpinBox->value();

    _ui.inputOptionsLineEdit->setText(new_input_options);

    on_actionUpdate_Example_triggered();
}

void SessionSettingsDialog::on_actionUpdateOutputOptionsString_triggered()
{
    QString new_output_options;
	QTextStream(&new_output_options) << " -b " << _ui.bitrateSpinBox->value();

	_ui.outputOptionsLineEdit->setText(new_output_options);

    on_actionUpdate_Example_triggered();
}

void SessionSettingsDialog::on_actionUpdate_Example_triggered()
{
    QString example_cmd;
    QTextStream(&example_cmd) << _ui.commandEdit->text() << _ui.inputOptionsLineEdit->text() << " -i <snap%05d.jpg>" << _ui.outputOptionsLineEdit->text() << " <out.mpg>";

#ifdef WIN32
    example_cmd.replace("/", "\\");
#endif

    _ui.exampleCommandEdit->setText( example_cmd );
}
                              



