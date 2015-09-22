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

#ifndef _GUI_BASE_H_
#define _GUI_BASE_H_

#include <QMainWindow>
#include <QNetworkProxy>

#include "config.h"
#include "guiconfig.h"
#include "mediadata.h"
#include "mediasettings.h"
#include "preferences.h"
#include "core.h"

#ifdef Q_OS_WIN
#ifdef AVOID_SCREENSAVER
/* Disable screensaver by event */
#include <windows.h>
#endif
#endif

//#define SHARE_MENU

class QWidget;
class QMenu;
class LogWindow;
class MplayerWindow;

class QLabel;
class FilePropertiesDialog;
class VideoEqualizer;
class AudioEqualizer;
class Playlist;
#ifdef FIND_SUBTITLES
class FindSubtitlesWindow;
#endif

#ifdef VIDEOPREVIEW
class VideoPreview;
#endif

class MyAction;
class MyActionGroup;
class PreferencesDialog;
class Favorites;
class TVList;
class UpdateChecker;

#ifdef SHAREWIDGET
class ShareWidget;
#endif

namespace Gui {
	

class TBase : public QMainWindow
{
    Q_OBJECT
    
public:
	TBase(QWidget* parent = 0, Qt::WindowFlags flags = 0);
	~TBase();

	/* Return true if the window shouldn't show on startup */
	virtual bool startHidden() { return false; };

	//! Execute all actions in \a actions. The actions should be
	//! separated by spaces. Checkable actions could have a parameter:
	//! true or false.
	void runActions(QString actions);

	//! Execute all the actions after the video has started to play
	void runActionsLater(QString actions) { pending_actions_to_run = actions; };

#ifdef LOG_SMPLAYER
	//! Saves the line from the smplayer output
	void recordSmplayerLog(QString line);
#endif

	Core * getCore() { return core; };
	Playlist * getPlaylist() { return playlist; };

	virtual void loadConfig(const QString &group);
	virtual void saveConfig(const QString &group);

	void retranslate() { retranslateStrings(); }

public slots:
	virtual void open(const QString &file); // Generic open, autodetect type.
	virtual void openFile();
	virtual void openFiles(QStringList files);
	virtual void openFavorite(QString file);
	virtual void openURL();
	virtual void openURL(QString url);
	virtual void openVCD();
	virtual void openAudioCD();
	virtual void openDVD();
	virtual void openDVDFromFolder();
	virtual void openDVDFromFolder(const QString &directory);
	void openBluRay();
	void openBluRayFromFolder();
	void openBluRayFromFolder(QString directory);
	virtual void openDirectory();
	virtual void openDirectory(QString directory);

	virtual void helpFirstSteps();
	virtual void helpFAQ();
	virtual void helpCLOptions();
	virtual void helpCheckUpdates();
#ifdef REMINDER_ACTIONS
	virtual void helpDonate();
#endif
	virtual void helpShowConfig();
	virtual void helpAbout();

#ifdef SHARE_MENU
	virtual void shareSMPlayer();
#endif

	virtual void loadSub();
	virtual void loadAudioFile(); // Load external audio file

	void setInitialSubtitle(const QString & subtitle_file);

#ifdef FIND_SUBTITLES
	virtual void showFindSubtitlesDialog();
	virtual void openUploadSubtitlesPage(); //turbos
#endif

#ifdef VIDEOPREVIEW
	virtual void showVideoPreviewDialog();
#endif

#ifdef YOUTUBE_SUPPORT
	virtual void showTubeBrowser();
#endif

	virtual void showPlaylist();
	virtual void showPlaylist(bool b);
	virtual void showVideoEqualizer();
	virtual void showVideoEqualizer(bool b);
	virtual void showAudioEqualizer();
	virtual void showAudioEqualizer(bool b);
#ifdef LOG_MPLAYER
	virtual void showMplayerLog();
#endif
#ifdef LOG_SMPLAYER
	virtual void showLog();
#endif
	virtual void showPreferencesDialog();
	virtual void showFilePropertiesDialog();

	virtual void showGotoDialog();
	virtual void showSubDelayDialog();
	virtual void showAudioDelayDialog();
	virtual void showStereo3dDialog();

	virtual void exitFullscreen();
	virtual void toggleFullscreen();
	virtual void toggleFullscreen(bool);

	virtual void toggleCompactMode();
	virtual void toggleCompactMode(bool);

	void setStayOnTop(bool b);
	virtual void changeStayOnTop(int);
	virtual void checkStayOnTop(Core::State);
	void toggleStayOnTop();

	void setForceCloseOnFinish(int n) { arg_close_on_finish = n; };
	int forceCloseOnFinish() { return arg_close_on_finish; };

	void setForceStartInFullscreen(int n) { arg_start_in_fullscreen = n; };
	int forceStartInFullscreen() { return arg_start_in_fullscreen; };

	void slotNoVideo();

protected slots:
	virtual void closeWindow();

	virtual void setJumpTexts();

	// Replace for setCaption (in Qt 4 it's not virtual)
	virtual void setWindowCaption(const QString & title); 

	//virtual void openRecent(int item);
	virtual void openRecent();
	virtual void enterFullscreenOnPlay();
	virtual void exitFullscreenOnStop();
	virtual void exitFullscreenIfNeeded();
	virtual void playlistHasFinished();

	virtual void displayState(Core::State state);
	virtual void displayMessage(QString message, int time);
	virtual void displayMessage(QString message);
	virtual void gotCurrentTime(double);
	virtual void gotDuration(double);

	virtual void updateWidgets();
	virtual void updateVideoTracks();
	virtual void updateAudioTracks();
	virtual void updateSubtitles();
	virtual void updateTitles();
	virtual void updateChapters();
	virtual void updateAngles();
	virtual void updateVideoEqualizer();
	virtual void updateAudioEqualizer();
	virtual void setDefaultValuesFromVideoEqualizer();
	virtual void changeVideoEqualizerBySoftware(bool b);

	virtual void newMediaLoaded();
	virtual void updateMediaInfo();

	void gotNoFileToPlay();

	void checkPendingActionsToRun();

#if REPORT_OLD_MPLAYER
	void checkMplayerVersion();
	void displayWarningAboutOldMplayer();
#endif

#ifdef CHECK_UPGRADED
	void checkIfUpgraded();
#endif

#if defined(REMINDER_ACTIONS) && !defined(SHAREWIDGET)
	void checkReminder();
#endif

#ifdef YOUTUBE_SUPPORT
	void YTNoSslSupport();
	void YTNoSignature(const QString &);
	#ifdef YT_USE_YTSIG
	void YTUpdateScript();
	#endif
#endif
	void gotForbidden();

#if AUTODISABLE_ACTIONS
	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();
#endif
	virtual void togglePlayAction(Core::State);

	void toggleDoubleSize();
	void resizeMainWindow(int w, int h, bool try_twice = true);
	virtual void resizeWindow(int w, int h);

	/* virtual void playlistVisibilityChanged(); */

	virtual void displayGotoTime(int);
	//! You can call this slot to jump to the specified percentage in the video, while dragging the slider.
	virtual void goToPosOnDragging(int);

	virtual void showPopupMenu();
	virtual void showPopupMenu( QPoint p );
	/*
	virtual void mouseReleaseEvent( QMouseEvent * e );
	virtual void mouseDoubleClickEvent( QMouseEvent * e );
	*/

	virtual void leftClickFunction();
	virtual void rightClickFunction();
	virtual void doubleClickFunction();
	virtual void middleClickFunction();
	virtual void xbutton1ClickFunction();
	virtual void xbutton2ClickFunction();
	virtual void processFunction(QString function);

	virtual void dragEnterEvent( QDragEnterEvent * ) ;
	virtual void dropEvent ( QDropEvent * );

	virtual void applyNewPreferences();
	virtual void applyFileProperties();

	virtual void clearRecentsList();

	virtual void loadActions();
	virtual void saveActions();

	// Single instance stuff
#ifdef SINGLE_INSTANCE
	void handleMessageFromOtherInstances(const QString& message);
#endif

	//! Called when core can't parse the mplayer version and there's no
	//! version supplied by the user
	void askForMplayerVersion(QString);

	void showExitCodeFromPlayer(int exit_code);
	void showErrorFromPlayer(QProcess::ProcessError);

	// stylesheet
#if ALLOW_CHANGE_STYLESHEET
	virtual QString loadQss(QString filename);
	virtual void changeStyleSheet(QString style);
#endif

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	void clear_just_stopped();
#endif
#endif

#ifdef LOG_MPLAYER
	//! Clears the mplayer log
	void clearMplayerLog();

	//! Saves the line from the mplayer output
	void recordMplayerLog(QString line);

	//! Saves the mplayer log to a file every time a file is loaded
	void autosaveMplayerLog();
#endif

signals:
	void frameChanged(int);
	void ABMarkersChanged(int secs_a, int secs_b);
	void videoInfoChanged(int width, int height, double fps);
	void timeChanged(QString time_ready_to_print);

	/*
	void wheelUp();
	void wheelDown();
	*/
	/*
	void doubleClicked();
	void leftClicked();
	void middleClicked();
	*/

	//! Sent when another instance requested to play a file
	void openFileRequested();

	void requestRestart();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent(QEvent * event);
	virtual void moveEvent(QMoveEvent * event);
#if QT_VERSION < 0x050000
	virtual void hideEvent( QHideEvent * );
	virtual void showEvent( QShowEvent * );
#else
	virtual bool event(QEvent * e);
	bool was_minimized;
#endif
#ifdef Q_OS_WIN
	#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	virtual bool winEvent ( MSG * m, long * result );
	#endif
#endif

	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

protected:
	void createCore();
	void createMplayerWindow();
	void createVideoEqualizer();
	void createAudioEqualizer();
	void createPlaylist();
	void createPanel();
	void createPreferencesDialog();
	void createFilePropertiesDialog();
	void setDataToFileProperties();
	void createActions();
#if AUTODISABLE_ACTIONS
	void setActionsEnabled(bool);
#endif
	void createMenus();
	void updateRecents();
	void configureDiscDevices();
	void setupNetworkProxy();
	virtual void closeEvent( QCloseEvent * e );

protected:
	/* virtual void wheelEvent( QWheelEvent * e ) ; */

protected:
	QWidget * panel;

	// Menu File
	MyAction * openFileAct;
	MyAction * openDirectoryAct;
	MyAction * openPlaylistAct;
	MyAction * openVCDAct;
	MyAction * openAudioCDAct;
	MyAction * openDVDAct;
	MyAction * openDVDFolderAct;
	MyAction * openBluRayAct;
	MyAction * openBluRayFolderAct;
	MyAction * openURLAct;
	MyAction * exitAct;
	MyAction * clearRecentsAct;

	// Menu Play
	MyAction * playAct;
	MyAction * playOrPauseAct;
	MyAction * pauseAct;
	MyAction * stopAct;
	MyAction * frameStepAct;
	MyAction * frameBackStepAct;
	MyAction * rewind1Act;
	MyAction * rewind2Act;
	MyAction * rewind3Act;
	MyAction * forward1Act;
	MyAction * forward2Act;
	MyAction * forward3Act;
	MyAction * repeatAct;
	MyAction * setAMarkerAct;
	MyAction * setBMarkerAct;
	MyAction * clearABMarkersAct;
	MyAction * gotoAct;

	// Menu Speed
	MyAction * normalSpeedAct;
	MyAction * halveSpeedAct;
	MyAction * doubleSpeedAct;
	MyAction * decSpeed10Act;
	MyAction * incSpeed10Act;
	MyAction * decSpeed4Act;
	MyAction * incSpeed4Act;
	MyAction * decSpeed1Act;
	MyAction * incSpeed1Act;

	// Menu Video
	MyAction * fullscreenAct;
	MyAction * compactAct;
	MyAction * videoEqualizerAct;
	MyAction * screenshotAct;
	MyAction * screenshotsAct;
#ifdef VIDEOPREVIEW
	MyAction * videoPreviewAct;
#endif
	MyAction * flipAct;
	MyAction * mirrorAct;
	MyAction * stereo3dAct;
	MyAction * postProcessingAct;
	MyAction * phaseAct;
	MyAction * deblockAct;
	MyAction * deringAct;
	MyAction * gradfunAct;
	MyAction * addNoiseAct;
	MyAction * addLetterboxAct;
	MyAction * upscaleAct;

	// Menu Audio
	MyAction * audioEqualizerAct;
	MyAction * muteAct;
	MyAction * decVolumeAct;
	MyAction * incVolumeAct;
	MyAction * decAudioDelayAct;
	MyAction * incAudioDelayAct;
	MyAction * audioDelayAct; // Ask for delay
	MyAction * extrastereoAct;
	MyAction * karaokeAct;
	MyAction * volnormAct;
	MyAction * loadAudioAct;
	MyAction * unloadAudioAct;

	// Menu Subtitles
	MyAction * loadSubsAct;
	MyAction * unloadSubsAct;
	MyAction * decSubDelayAct;
	MyAction * incSubDelayAct;
	MyAction * subDelayAct; // Ask for delay
	MyAction * decSubPosAct;
	MyAction * incSubPosAct;
	MyAction * incSubStepAct;
	MyAction * decSubStepAct;
	MyAction * incSubScaleAct;
	MyAction * decSubScaleAct;
	MyAction * useCustomSubStyleAct;
	MyAction * useForcedSubsOnlyAct;
#ifdef FIND_SUBTITLES
	MyAction * showFindSubtitlesDialogAct;
	MyAction * openUploadSubtitlesPageAct;//turbos  
#endif

	// Menu Options
	MyAction * showPlaylistAct;
	MyAction * showPropertiesAct;
	MyAction * showPreferencesAct;
#ifdef YOUTUBE_SUPPORT
	MyAction * showTubeBrowserAct;
#endif
#ifdef LOG_MPLAYER
	MyAction * showLogMplayerAct;
#endif
#ifdef LOG_SMPLAYER
	MyAction * showLogSmplayerAct;
#endif

	// Menu Help
	MyAction * showFirstStepsAct;
	MyAction * showFAQAct;
	MyAction * showCLOptionsAct; // Command line options
	MyAction * showCheckUpdatesAct;
#if defined(YOUTUBE_SUPPORT) && defined(YT_USE_YTSIG)
	MyAction * updateYTAct;
#endif
	MyAction * showConfigAct;
#ifdef REMINDER_ACTIONS
	MyAction * donateAct;
#endif
	MyAction * aboutThisAct;

#ifdef SHARE_MENU
	MyAction * facebookAct;
	MyAction * twitterAct;
	MyAction * gmailAct;
	MyAction * hotmailAct;
	MyAction * yahooAct;
#endif

	// OSD
	MyAction * incOSDScaleAct;
	MyAction * decOSDScaleAct;

	// Playlist
	MyAction * playPrevAct;
	MyAction * playNextAct;

	// Actions not in menus
#if !USE_MULTIPLE_SHORTCUTS
	MyAction * decVolume2Act;
	MyAction * incVolume2Act;
#endif
	MyAction * exitFullscreenAct;
	MyAction * nextOSDLevelAct;
	MyAction * decContrastAct;
	MyAction * incContrastAct;
	MyAction * decBrightnessAct;
	MyAction * incBrightnessAct;
	MyAction * decHueAct;
	MyAction * incHueAct;
	MyAction * decSaturationAct;
	MyAction * incSaturationAct;
	MyAction * decGammaAct;
	MyAction * incGammaAct;
	MyAction * nextVideoAct;
	MyAction * nextAudioAct;
	MyAction * nextSubtitleAct;
	MyAction * nextChapterAct;
	MyAction * prevChapterAct;
	MyAction * doubleSizeAct;
	MyAction * resetVideoEqualizerAct;
	MyAction * resetAudioEqualizerAct;
	MyAction * showContextMenuAct;
	MyAction * nextAspectAct;
	MyAction * nextWheelFunctionAct;

	MyAction * showFilenameAct;
	MyAction * showTimeAct;
	MyAction * toggleDeinterlaceAct;

	// Moving and zoom
	MyAction * moveUpAct;
	MyAction * moveDownAct;
	MyAction * moveLeftAct;
	MyAction * moveRightAct;
	MyAction * incZoomAct;
	MyAction * decZoomAct;
	MyAction * resetZoomAct;
	MyAction * autoZoomAct;
	MyAction * autoZoom169Act;
	MyAction * autoZoom235Act;


	// OSD Action Group 
	MyActionGroup * osdGroup;
	MyAction * osdNoneAct;
	MyAction * osdSeekAct;
	MyAction * osdTimerAct;
	MyAction * osdTotalAct;

	// Denoise Action Group
	MyActionGroup * denoiseGroup;
	MyAction * denoiseNoneAct;
	MyAction * denoiseNormalAct;
	MyAction * denoiseSoftAct;

	// Blur-sharpen group
	MyActionGroup * unsharpGroup;
	MyAction * unsharpNoneAct;
	MyAction * blurAct;
	MyAction * sharpenAct;

	// Window Size Action Group
	MyActionGroup * sizeGroup;
	MyAction * size50;
	MyAction * size75;
	MyAction * size100;
	MyAction * size125;
	MyAction * size150;
	MyAction * size175;
	MyAction * size200;
	MyAction * size300;
	MyAction * size400;

	// Deinterlace Action Group
	MyActionGroup * deinterlaceGroup;
	MyAction * deinterlaceNoneAct;
	MyAction * deinterlaceL5Act;
	MyAction * deinterlaceYadif0Act;
	MyAction * deinterlaceYadif1Act;
	MyAction * deinterlaceLBAct;
	MyAction * deinterlaceKernAct;

	// Aspect Action Group
	MyActionGroup * aspectGroup;
	MyAction * aspectDetectAct;
	MyAction * aspectNoneAct;
	MyAction * aspect11Act;		// 1:1
	MyAction * aspect32Act;		// 3:2
	MyAction * aspect43Act;		// 4:3
	MyAction * aspect118Act;	// 11:8
	MyAction * aspect54Act;		// 5:4
	MyAction * aspect149Act;	// 14:9
	MyAction * aspect1410Act;	// 14:10
	MyAction * aspect169Act;	// 16:9
	MyAction * aspect1610Act;	// 16:10
	MyAction * aspect235Act;	// 2.35:1

	// Rotate Group
	MyActionGroup * rotateGroup;
	MyAction * rotateNoneAct;
	MyAction * rotateClockwiseFlipAct;
	MyAction * rotateClockwiseAct;
	MyAction * rotateCounterclockwiseAct;
	MyAction * rotateCounterclockwiseFlipAct;

	// Menu StayOnTop
	MyActionGroup * onTopActionGroup;
	MyAction * onTopAlwaysAct;
	MyAction * onTopNeverAct;
	MyAction * onTopWhilePlayingAct;
	MyAction * toggleStayOnTopAct;

#if USE_ADAPTER
	// Screen Group
	MyActionGroup * screenGroup;
	MyAction * screenDefaultAct;
#endif

	// Closed Captions Group
	MyActionGroup * ccGroup;
	MyAction * ccNoneAct;
	MyAction * ccChannel1Act;
	MyAction * ccChannel2Act;
	MyAction * ccChannel3Act;
	MyAction * ccChannel4Act;

	// External sub fps Group
	MyActionGroup * subFPSGroup;
	MyAction * subFPSNoneAct;
	/* MyAction * subFPS23Act; */
	MyAction * subFPS23976Act;
	MyAction * subFPS24Act;
	MyAction * subFPS25Act;
	MyAction * subFPS29970Act;
	MyAction * subFPS30Act;

	// Audio Channels Action Group
	MyActionGroup * channelsGroup;
	/* MyAction * channelsDefaultAct; */
	MyAction * channelsStereoAct;
	MyAction * channelsSurroundAct;
	MyAction * channelsFull51Act;
	MyAction * channelsFull61Act;
	MyAction * channelsFull71Act;

	// Stereo Mode Action Group
	MyActionGroup * stereoGroup;
	MyAction * stereoAct;
	MyAction * leftChannelAct;
	MyAction * rightChannelAct;
	MyAction * monoAct;
	MyAction * reverseAct;

	// Other groups
#if PROGRAM_SWITCH
	MyActionGroup * programTrackGroup;
#endif
	MyActionGroup * videoTrackGroup;
	MyActionGroup * audioTrackGroup;
	MyActionGroup * subtitleTrackGroup;
#ifdef MPV_SUPPORT
	MyActionGroup * secondarySubtitleTrackGroup;
#endif
	MyActionGroup * titleGroup;
	MyActionGroup * angleGroup;
	MyActionGroup * chapterGroup;

	MyAction * dvdnavUpAct;
	MyAction * dvdnavDownAct;
	MyAction * dvdnavLeftAct;
	MyAction * dvdnavRightAct;
	MyAction * dvdnavMenuAct;
	MyAction * dvdnavSelectAct;
	MyAction * dvdnavPrevAct;
	MyAction * dvdnavMouseAct;

	// MENUS
	QMenu *openMenu;
	QMenu *playMenu;
	QMenu *videoMenu;
	QMenu *audioMenu;
	QMenu *subtitlesMenu;
	QMenu *browseMenu;
	QMenu *optionsMenu;
	QMenu *helpMenu;

	QMenu * disc_menu;
	QMenu * subtitles_track_menu;
#ifdef MPV_SUPPORT
	QMenu * secondary_subtitles_track_menu;
#endif
#if PROGRAM_SWITCH
	QMenu * programtrack_menu;
#endif
	QMenu * videotrack_menu;
	QMenu * audiotrack_menu;
	QMenu * titles_menu;
	QMenu * chapters_menu;
	QMenu * angles_menu;
	QMenu * aspect_menu;
	QMenu * osd_menu;
	QMenu * deinterlace_menu;
	QMenu * denoise_menu;
	QMenu * unsharp_menu;
	QMenu * videosize_menu;
	QMenu * audiochannels_menu;
	QMenu * stereomode_menu;

	QMenu * speed_menu;
	QMenu * ab_menu; // A-B menu
	QMenu * videofilter_menu;
	QMenu * audiofilter_menu;
#if defined(LOG_MPLAYER) || defined(LOG_SMPLAYER)
	QMenu * logs_menu;
#endif
	QMenu * zoom_menu;
	QMenu * rotate_menu;
	QMenu * ontop_menu;
#if USE_ADAPTER
	QMenu * screen_menu;
#endif
	QMenu * closed_captions_menu;
	QMenu * subfps_menu;

#ifdef SHARE_MENU
	QMenu * share_menu;
#endif

	QMenu * popup;
	QMenu * recentfiles_menu;

#ifdef LOG_MPLAYER
	LogWindow * mplayer_log_window;
#endif
#ifdef LOG_SMPLAYER
	LogWindow * smplayer_log_window;
#endif
	LogWindow * clhelp_window;

	PreferencesDialog *pref_dialog;
	FilePropertiesDialog *file_dialog;
	Playlist * playlist;
	VideoEqualizer * video_equalizer;
	AudioEqualizer * audio_equalizer;
#ifdef FIND_SUBTITLES
	FindSubtitlesWindow * find_subs_dialog;
#endif
#ifdef VIDEOPREVIEW
	VideoPreview * video_preview;
#endif

	Core * core;
	MplayerWindow *mplayerwindow;

	Favorites * favorites;

	TVList * tvlist;
	TVList * radiolist;

#ifdef UPDATE_CHECKER
	UpdateChecker * update_checker;
#endif

#ifdef SHAREWIDGET
	ShareWidget * sharewidget;
#endif

	QStringList actions_list;

	QString pending_actions_to_run;

	// Force settings from command line
	int arg_close_on_finish; // -1 = not set, 1 = true, 0 = false
	int arg_start_in_fullscreen; // -1 = not set, 1 = true, 0 = false

private:
	QString default_style;

	// Variables to restore pos and size of the window
	// when exiting from fullscreen mode.
	QPoint win_pos;
	QSize win_size;
	bool was_maximized;

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
#ifdef AVOID_SCREENSAVER
	/* Disable screensaver by event */
	bool just_stopped;
#endif
#endif

#ifdef LOG_MPLAYER
	QString mplayer_log;
#endif
#ifdef LOG_SMPLAYER
	QString smplayer_log;
#endif

	bool ignore_show_hide_events;
	bool block_resize;

	void hidePanel();
};

} // namespace Gui

#endif // _GUI_BASE_H_
