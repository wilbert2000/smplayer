/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _PROC_PLAYERPROCESS_H
#define _PROC_PLAYERPROCESS_H

#include <QObject>
#include <QVariant>
#include <QRegExp>

#include "config.h"
#include "myprocess.h"
#include "mediadata.h"
#include "playerid.h"
#include "assstyles.h"


namespace Proc {

// OSD position used by setOSDPos and MplayerWindow
extern QPoint default_osd_pos;

	
class PlayerProcess : public MyProcess
{
	Q_OBJECT

public:
	enum ScreenshotType { Single = 0, Multiple = 1 };

	PlayerProcess(PlayerID::Player pid, MediaData * mdata);

	PlayerID::Player player() { return player_id; }
	bool isMPlayer() { return (player_id == PlayerID::MPLAYER); }
	bool isMPV() { return (player_id == PlayerID::MPV); }
	bool isFullyStarted() { return isRunning() && notified_player_is_running; }

	virtual bool startPlayer();

	void writeToStdin(QString text, bool log = true);

	// Command line options
	virtual void setMedia(const QString & media, bool is_playlist = false) = 0;
	virtual void setFixedOptions() = 0;
	virtual void disableInput() = 0;
	virtual void setOption(const QString & option_name, const QVariant & value = QVariant()) = 0;
	virtual void addUserOption(const QString & option) = 0;
	virtual void addVF(const QString & filter_name, const QVariant & value = QVariant()) = 0;
	virtual void addAF(const QString & filter_name, const QVariant & value = QVariant()) = 0;
	virtual void addStereo3DFilter(const QString & in, const QString & out) = 0;
	virtual void setSubStyles(const AssStyles & styles, const QString & assStylesFile = QString::null) = 0;

	// Slave commands
	void quit(int exit_code);
	virtual void setVolume(int v) = 0;
	virtual void setOSDLevel(int level) = 0;
	virtual void setAudio(int ID) = 0;
	virtual void setVideo(int ID) = 0;
	virtual void setSubtitle(SubData::Type type, int ID) = 0;
	virtual void disableSubtitles() = 0;
	virtual void setSecondarySubtitle(int ID) = 0;
	virtual void disableSecondarySubtitles() = 0;
	virtual void setSubtitlesVisibility(bool b) = 0;
	virtual void seekPlayerTime(double secs, int mode, bool precise, bool currently_paused) = 0;
	virtual void seek(double secs, int mode, bool precise, bool currently_paused);
	virtual void mute(bool b) = 0;
	virtual void setPause(bool b) = 0;
	virtual void frameStep() = 0;
	virtual void frameBackStep() = 0;
	virtual void showOSDText(const QString & text, int duration, int level) = 0;
	virtual void showFilenameOnOSD() = 0;
	virtual void showTimeOnOSD() = 0;
	virtual void setContrast(int value) = 0;
	virtual void setBrightness(int value) = 0;
	virtual void setHue(int value) = 0;
	virtual void setSaturation(int value) = 0;
	virtual void setGamma(int value) = 0;
	virtual void setChapter(int ID) = 0;
	virtual void setExternalSubtitleFile(const QString & filename) = 0;
	virtual void setSubPos(int pos) = 0;
	virtual void setSubScale(double value) = 0;
	virtual void setSubStep(int value) = 0;
	virtual void setSubForcedOnly(bool b) = 0;
	virtual void setSpeed(double value) = 0;
	virtual void enableKaraoke(bool b) = 0;
	virtual void enableExtrastereo(bool b) = 0;
	virtual void enableVolnorm(bool b, const QString & option) = 0;
	virtual void setAudioEqualizer(const QString & values) = 0;
	virtual void setAudioDelay(double delay) = 0;
	virtual void setSubDelay(double delay) = 0;
	virtual void setLoop(int v) = 0;
	virtual void takeScreenshot(ScreenshotType t, bool include_subtitles = false) = 0;
	virtual void setTitle(int ID) = 0;
	virtual void changeVF(const QString & filter, bool enable, const QVariant & option = QVariant()) = 0;
	virtual void changeStereo3DFilter(bool enable, const QString & in, const QString & out) = 0;

	virtual void discSetMousePos(int x, int y) = 0;
	virtual void discButtonPressed(const QString & button_name) = 0;

	virtual void setAspect(double aspect) = 0;
	virtual void setFullscreen(bool b) = 0;
#if PROGRAM_SWITCH
	virtual void setTSProgram(int ID) = 0;
#endif
	virtual void toggleDeinterlace() = 0;
	virtual void setOSDPos(const QPoint &pos) = 0;
	virtual void setOSDScale(double value) = 0;
	virtual void setChannelsFile(const QString &) = 0;

	static PlayerProcess * createPlayerProcess(const QString &player_bin, MediaData *md);

// Signals
signals:
	void processExited(bool normal_exit);

	void playerFullyLoaded();

	void lineAvailable(QString line);

	void receivedVideoOutResolution(int, int);
	void receivedCurrentSec(double sec);
	void receivedCurrentFrame(int frame);
	void receivedPause();
	void receivedVO(QString);
	void receivedAO(QString);
	void receivedEndOfFile();

	void receivedMessage(QString);
	void receivedCacheEmptyMessage(QString);
	void receivedCreatingIndex(QString);
	void receivedConnectingToMessage(QString);
	void receivedResolvingMessage(QString);
	void receivedBuffering();
	void receivedBufferingEnded();
	void receivedScreenshot(QString);
	void receivedUpdatingFontCache();
	void receivedForbiddenText();

	void receivedStreamTitle(QString);
	void receivedStreamTitleAndUrl(QString,QString);

	void failedToParseMplayerVersion(QString line_with_mplayer_version);

	//! Emitted if a new video track has been added or an old one changed
	void receivedVideoTrackInfo();

	//! Emitted if player changed video track
	void receivedVideoTrackChanged(int);

	//! Emitted if a new audio track has been added or an old one changed
	void receivedAudioTrackInfo();

	//! Emitted if player changed audio track
	void receivedAudioTrackChanged(int);

	//! Emitted if a new subtitle has been added or an old one changed
	void receivedSubtitleTrackInfo();

	//! Emitted if player changed subtitle track
	void receivedSubtitleTrackChanged();

	void receivedTitleTrackInfo();
	void receivedTitleTrackChanged(int);

	void receivedChapterInfo();

	void durationChanged(double);

	void receivedTitleIsMenu();
	void receivedTitleIsMovie();

public slots:
	void parseBytes(QByteArray ba);

protected:
	PlayerID::Player player_id;

	MediaData* md;

	bool notified_player_is_running;
	int waiting_for_answers;

	bool received_end_of_file;

	double guiTimeToPlayerTime(double sec);
	double playerTimeToGuiTime(double sec);

	virtual int getFrame(double time_sec, const QString &line) = 0;
	void notifyTitleTrackChanged(int new_title);
	void notifyDuration(double duration);
	virtual void checkTime(double sec);
	void notifyTime(double time_sec, const QString &line);
	bool waitForAnswers();

	virtual void playingStarted();
	virtual bool parseLine(QString &line);
	virtual bool parseStatusLine(double time_sec, double duration, QRegExp &rx, QString &line);
	virtual bool parseAudioProperty(const QString &name, const QString &value);
	virtual bool parseVideoProperty(const QString &name, const QString &value);
	virtual bool parseMetaDataProperty(QString name, QString value);
	virtual bool parseProperty(const QString &name, const QString &value);

protected slots:
	void processError(QProcess::ProcessError);
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
	int line_count;

	int waiting_for_answers_safe_guard;

	bool quit_send;

	int prev_frame;
};

} // namespace Proc

#endif // _PROC_PLAYERPROCESS_H