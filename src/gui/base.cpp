/*  WZPlayer, GUI front-end for mplayer and MPV.
	Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/base.h"

#include <QDebug>
#include <QMessageBox>
#include <QLabel>
#include <QMenu>
#include <QFileInfo>
#include <QApplication>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QHBoxLayout>
#include <QCursor>
#include <QTimer>
#include <QStyle>
#include <QRegExp>
#include <QStatusBar>
#include <QActionGroup>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDesktopServices>
#include <QInputDialog>
#include <QClipboard>
#include <QMimeData>
#include <QNetworkProxy>

#include "proc/errormsg.h"
#include "version.h"
#include "desktop.h"
#include "discname.h"
#include "extensions.h"
#include "log.h"
#include "colorutils.h"
#include "images.h"
#include "helper.h"
#include "mediadata.h"
#include "playerwindow.h"
#include "core.h"
#include "clhelp.h"
#include "filedialog.h"

#include "settings/paths.h"
#include "settings/preferences.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"

#include "gui/action/action.h"
#include "gui/action/actiongroup.h"
#include "gui/action/timeslider.h"
#include "gui/action/widgetactions.h"
#include "gui/action/actionseditor.h"
#include "gui/action/editabletoolbar.h"
#include "gui/action/menu.h"
#include "gui/action/menuopen.h"
#include "gui/action/menuplay.h"
#include "gui/action/menuvideo.h"
#include "gui/action/menuaudio.h"
#include "gui/action/menusubtitle.h"
#include "gui/action/menubrowse.h"
#include "gui/action/menuwindow.h"
#include "gui/action/menuhelp.h"

#include "gui/logwindow.h"
#include "gui/playlist.h"
#include "gui/autohidetimer.h"
#include "gui/filepropertiesdialog.h"
#include "gui/inputdvddirectory.h"
#include "gui/about.h"
#include "gui/inputurl.h"
#include "gui/timedialog.h"
#include "gui/playlist.h"
#include "gui/videoequalizer.h"
#include "gui/eqslider.h"
#include "gui/audioequalizer.h"
#include "gui/stereo3ddialog.h"
#include "gui/action/tvlist.h"
#include "gui/updatechecker.h"

#include "gui/pref/dialog.h"
#include "gui/pref/general.h"
#include "gui/pref/interface.h"
#include "gui/pref/input.h"
#include "gui/pref/advanced.h"

#ifdef FIND_SUBTITLES
#include "findsubtitleswindow.h"
#endif

#ifdef MPRIS2
#include "mpris2/mpris2.h"
#endif

#ifdef Q_OS_WIN
#include "gui/deviceinfo.h"
#endif


using namespace Settings;

namespace Gui {

using namespace Action;


TBase::TBase() :
	QMainWindow(),
	toolbar_menu(0),
	help_window(0),
	pref_dialog(0),
	file_properties_dialog(0),
#ifdef FIND_SUBTITLES
	find_subs_dialog(0),
#endif
	menubar_visible(true),
	statusbar_visible(true),
	fullscreen_menubar_visible(false),
	fullscreen_statusbar_visible(true),
	arg_close_on_finish(-1),
	arg_start_in_fullscreen(-1),
	ignore_show_hide_events(false),
	center_window(false),
	update_checker(0) {

	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(TConfig::PROGRAM_NAME);
	setAcceptDrops(true);

	// Reset size factor
	if (pref->save_window_size_on_exit) {
		force_resize = false;
	} else {
		force_resize = true;
		pref->size_factor = 1.0;
	}

	// Resize window to default size
	resize(pref->default_size);

	// Setup move window timer merging multiple move requests into one
	move_window_timer.setSingleShot(true);
	move_window_timer.setInterval(0);
	connect(&move_window_timer, SIGNAL(timeout()),
			this, SLOT(moveWindowMerged()));

	// Create objects:
	createPanel();
	setCentralWidget(panel);

	createPlayerWindow();
	createCore();
	createPlaylist();
	createVideoEqualizer();
	createAudioEqualizer();

	log_window = new TLogWindow(this, "logwindow");

	createActions();
	createToolbars();
	createMenus();

	setupNetworkProxy();
	changeStayOnTop(pref->stay_on_top);

	update_checker = new TUpdateChecker(this, &pref->update_checker_data);

#ifdef MPRIS2
	if (pref->use_mpris2)
		new Mpris2(this, this);
#endif

	retranslateStrings();
}

TBase::~TBase() {
}

void TBase::createPanel() {

	panel = new QWidget(this);
	panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	panel->setMinimumSize(QSize(1, 1));
	panel->setFocusPolicy(Qt::StrongFocus);
}

void TBase::createPlayerWindow() {

	playerwindow = new TPlayerWindow(panel);
	playerwindow->setObjectName("playerwindow");

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setSpacing(0);
	layout->setMargin(0);
	layout->addWidget(playerwindow);
	panel->setLayout(layout);

	// Connect player window mouse events
	connect(playerwindow, SIGNAL(doubleClicked()),
			this, SLOT(doubleClickFunction()));
	connect(playerwindow, SIGNAL(leftClicked()),
			this, SLOT(leftClickFunction()));
	connect(playerwindow, SIGNAL(rightClicked()),
			this, SLOT(rightClickFunction()));
	connect(playerwindow, SIGNAL(middleClicked()),
			this, SLOT(middleClickFunction()));
	connect(playerwindow, SIGNAL(xbutton1Clicked()),
			this, SLOT(xbutton1ClickFunction()));
	connect(playerwindow, SIGNAL(xbutton2Clicked()),
			this, SLOT(xbutton2ClickFunction()));
	connect(playerwindow, SIGNAL(moveWindow(QPoint)),
			this, SLOT(moveWindow(QPoint)));
}

void TBase::createCore() {

	core = new TCore(this, playerwindow);

    connect(core, SIGNAL(positionChanged(double)),
            this, SLOT(onPositionChanged(double)));
	connect(core, SIGNAL(showFrame(int)),
			this, SIGNAL(frameChanged(int)));
	connect(core, SIGNAL(durationChanged(double)),
			this, SLOT(gotDuration(double)));

	connect(core, SIGNAL(stateChanged(TCoreState)),
			this, SLOT(onStateChanged(TCoreState)));
	connect(core, SIGNAL(stateChanged(TCoreState)),
			this, SLOT(checkStayOnTop(TCoreState)), Qt::QueuedConnection);

	connect(core, SIGNAL(mediaSettingsChanged()),
			this, SLOT(onMediaSettingsChanged()));
	connect(core, SIGNAL(videoOutResolutionChanged(int, int)),
			this, SLOT(onVideoOutResolutionChanged(int,int)));
	connect(core, SIGNAL(needResize(int, int)),
			this, SLOT(resizeWindow(int, int)));

	connect(core, SIGNAL(showMessage(const QString&, int)),
			this, SLOT(displayMessage(const QString&, int)));
	connect(core, SIGNAL(showMessage(const QString&)),
			this, SLOT(displayMessage(const QString&)));

	connect(core, SIGNAL(newMediaStartedPlaying()),
			this, SLOT(onNewMediaStartedPlaying()), Qt::QueuedConnection);

	connect(core, SIGNAL(mediaLoaded()),
			this, SLOT(setActionsEnabled()));

	connect(core, SIGNAL(mediaInfoChanged()),
			this, SLOT(onMediaInfoChanged()));

	connect(core, SIGNAL(mediaStopped()),
			this, SLOT(exitFullscreenOnStop()));

	connect(core, SIGNAL(playerError(QProcess::ProcessError)),
			this, SLOT(onPlayerError(QProcess::ProcessError)));
	connect(core, SIGNAL(playerFinishedWithError(int)),
			this, SLOT(onPlayerFinishedWithError(int)));
}

void TBase::createPlaylist() {

	playlist = new TPlaylist(this, core);
	connect(playlist, SIGNAL(playlistEnded()),
			this, SLOT(playlistHasFinished()));
	connect(playlist, SIGNAL(displayMessage(const QString&, int)),
			this, SLOT(displayMessage(const QString&, int)));
}

void TBase::createVideoEqualizer() {

	video_equalizer = new TVideoEqualizer(this);
	video_equalizer->setBySoftware(pref->use_soft_video_eq);

	connect(video_equalizer, SIGNAL(contrastChanged(int)),
			core, SLOT(setContrast(int)));
	connect(video_equalizer, SIGNAL(brightnessChanged(int)),
			core, SLOT(setBrightness(int)));
	connect(video_equalizer, SIGNAL(hueChanged(int)),
			core, SLOT(setHue(int)));
	connect(video_equalizer, SIGNAL(saturationChanged(int)),
			core, SLOT(setSaturation(int)));
	connect(video_equalizer, SIGNAL(gammaChanged(int)),
			core, SLOT(setGamma(int)));

	connect(video_equalizer, SIGNAL(requestToChangeDefaultValues()),
			this, SLOT(setDefaultValuesFromVideoEqualizer()));
	connect(video_equalizer, SIGNAL(bySoftwareChanged(bool)),
			this, SLOT(changeVideoEqualizerBySoftware(bool)));

	connect(core, SIGNAL(videoEqualizerNeedsUpdate()),
			this, SLOT(updateVideoEqualizer()));
}

void TBase::createAudioEqualizer() {

	audio_equalizer = new TAudioEqualizer(this);

	connect(audio_equalizer->eq[0], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq0(int)));
	connect(audio_equalizer->eq[1], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq1(int)));
	connect(audio_equalizer->eq[2], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq2(int)));
	connect(audio_equalizer->eq[3], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq3(int)));
	connect(audio_equalizer->eq[4], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq4(int)));
	connect(audio_equalizer->eq[5], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq5(int)));
	connect(audio_equalizer->eq[6], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq6(int)));
	connect(audio_equalizer->eq[7], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq7(int)));
	connect(audio_equalizer->eq[8], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq8(int)));
	connect(audio_equalizer->eq[9], SIGNAL(valueChanged(int)),
			core, SLOT(setAudioEq9(int)));

	connect(audio_equalizer, SIGNAL(applyClicked(const Settings::TAudioEqualizerList&)),
			core, SLOT(setAudioAudioEqualizerRestart(const Settings::TAudioEqualizerList&)));
	connect(audio_equalizer, SIGNAL(valuesChanged(const Settings::TAudioEqualizerList&)),
			core, SLOT(setAudioEqualizer(const Settings::TAudioEqualizerList&)));
}

void TBase::createActions() {
	qDebug("Gui::TBase::createActions");

	showContextMenuAct = new Action::TAction(this, "show_context_menu", tr("Show context menu"));
	connect(showContextMenuAct, SIGNAL(triggered()), this, SLOT(showContextMenu()));

    nextWheelFunctionAct = new TAction(this, "next_wheel_function", tr("Next wheel function"), 0, Qt::Key_W);
	connect(nextWheelFunctionAct, SIGNAL(triggered()), core, SLOT(nextWheelFunction()));

	// Time slider
    timeslider_action = new TTimeSliderAction(this);
	timeslider_action->setObjectName("timeslider_action");

    connect(core, SIGNAL(positionChanged(double)),
            timeslider_action, SLOT(setPosition(double)));
    connect(core, SIGNAL(durationChanged(double)),
            timeslider_action, SLOT(setDuration(double)));
    connect(timeslider_action, SIGNAL(positionChanged(double)),
            core, SLOT(seekTime(double)));
    connect(timeslider_action, SIGNAL(percentageChanged(double)),
            core, SLOT(seekPercentage(double)));
    connect(timeslider_action, SIGNAL(dragPositionChanged(double)),
            this, SLOT(onDragPositionChanged(double)));

	connect(timeslider_action, SIGNAL(wheelUp(Settings::TPreferences::TWheelFunction)),
			core, SLOT(wheelUp(Settings::TPreferences::TWheelFunction)));
	connect(timeslider_action, SIGNAL(wheelDown(Settings::TPreferences::TWheelFunction)),
			core, SLOT(wheelDown(Settings::TPreferences::TWheelFunction)));

	// Volume slider action
	volumeslider_action = new TVolumeSliderAction(this, core->getVolume());
	volumeslider_action->setObjectName("volumeslider_action");
	connect(volumeslider_action, SIGNAL(valueChanged(int)), core, SLOT(setVolume(int)));
	connect(core, SIGNAL(volumeChanged(int)), volumeslider_action, SLOT(setValue(int)));

	// Time label actions
	time_label_action = new TTimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	// Menu bar
	viewMenuBarAct = new TAction(this, "toggle_menubar", tr("Me&nu bar"), "", Qt::Key_F2);
	viewMenuBarAct->setCheckable(true);
	viewMenuBarAct->setChecked(true);
	connect(viewMenuBarAct, SIGNAL(toggled(bool)), menuBar(), SLOT(setVisible(bool)));

	// Toolbars
	editToolbarAct = new TAction(this, "edit_toolbar1", tr("Edit main &toolbar..."));
	editToolbar2Act = new TAction(this, "edit_toolbar2", tr("Edit extra t&oolbar..."));

	// Control bar
	editControlBarAct = new TAction(this, "edit_controlbar", tr("Edit control &bar.."));

	// Status bar
	viewStatusBarAct = new TAction(this, "toggle_statusbar", tr("&Status bar"), "", Qt::Key_F6);
	viewStatusBarAct->setCheckable(true);
	viewStatusBarAct->setChecked(true);
	connect(viewStatusBarAct, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));
} // createActions

void TBase::createMenus() {

	// MENUS
	openMenu = new TMenuOpen(this, playlist);
	menuBar()->addMenu(openMenu);
	playMenu = new TMenuPlay(this, core, playlist);
	menuBar()->addMenu(playMenu);
	videoMenu = new TMenuVideo(this, core, playerwindow, video_equalizer);
	menuBar()->addMenu(videoMenu);
	audioMenu = new TMenuAudio(this, core, audio_equalizer);
	menuBar()->addMenu(audioMenu);
	subtitleMenu = new TMenuSubtitle(this, core);
	menuBar()->addMenu(subtitleMenu);
	browseMenu = new TMenuBrowse(this, core);
	menuBar()->addMenu(browseMenu);

	// statusbar_menu added to toolbar_menu by createToolbarMenu()
	// and filled by descendants::createMenus()
	statusbar_menu = new QMenu(this);
	toolbar_menu = createToolbarMenu();
	windowMenu = new TMenuWindow(this, core, toolbar_menu, playlist, log_window);
	menuBar()->addMenu(windowMenu);

	helpMenu = new TMenuHelp(this);
	menuBar()->addMenu(helpMenu);

	// POPUP MENU
	popup = new QMenu(this);
	popup->addMenu(openMenu);
	popup->addMenu(playMenu);
	popup->addMenu(videoMenu);
	popup->addMenu(audioMenu);
	popup->addMenu(subtitleMenu);
	popup->addMenu(browseMenu);
	popup->addMenu(windowMenu);
} // createMenus()

QMenu* TBase::createToolbarMenu() {

	// Use name "toolbar_menu" only for first
	QString name = toolbar_menu ? "" : "toolbar_menu";
    QMenu* menu = new TMenu(this, this, name, tr("&Toolbars"), "toolbars");

	menu->addAction(viewMenuBarAct);
	menu->addAction(toolbar->toggleViewAction());
	menu->addAction(toolbar2->toggleViewAction());
	menu->addAction(controlbar->toggleViewAction());
	menu->addAction(viewStatusBarAct);

	menu->addSeparator();
	menu->addAction(editToolbarAct);
	menu->addAction(editToolbar2Act);
	menu->addAction(editControlBarAct);

	menu->addSeparator();
	menu->addMenu(statusbar_menu);

	connect(menu, SIGNAL(aboutToShow()), auto_hide_timer, SLOT(disable()));
	connect(menu, SIGNAL(aboutToHide()), auto_hide_timer, SLOT(enable()));

	return menu;
} // createToolbarMenu

// Called by main window to show context popup.
// Main window takes ownership of menu.
QMenu* TBase::createPopupMenu() {
	//qDebug("Gui::TBase::createPopupMenu");
	return createToolbarMenu();
}

void TBase::showStatusBarPopup(const QPoint& pos) {
	//qDebug("Gui::TBase::showStatusBarPopup: x: %d y: %d", pos.x(), pos.y());
	execPopup(this, toolbar_menu, statusBar()->mapToGlobal(pos));
}

void TBase::createToolbars() {

	menuBar()->setObjectName("menubar");

	// Control bar
	controlbar = new TEditableToolbar(this);
	controlbar->setObjectName("controlbar");
	QStringList actions;
	actions << "show_playlist|0|1"
			<< "separator|0|1"
			<< "play_or_pause"
            << "timeslider_action"
            << "rewind_menu"
            << "forward_menu"
            << "in_out_points_menu|0|1"
            << "separator|0|1"
            << "osd_menu|0|1"
            << "aspect_menu|1|1"
            << "videosize_menu|1|0"
            << "reset_zoom|0|1"
            << "separator|0|1"
            << "mute|0|1"
			<< "volumeslider_action"
            << "separator|0|1"
			<< "fullscreen";
	controlbar->setDefaultActions(actions);
	addToolBar(Qt::BottomToolBarArea, controlbar);
	connect(editControlBarAct, SIGNAL(triggered()),
			controlbar, SLOT(edit()));

	QAction* action = controlbar->toggleViewAction();
	action->setObjectName("toggle_controlbar");
	action->setShortcut(Qt::Key_F5);

	// Main toolbar
	toolbar = new TEditableToolbar(this);
	toolbar->setObjectName("toolbar1");
	actions.clear();
	actions << "open_url" << "favorites_menu" << "separator"
			<< "osd_menu" << "toolbar_menu" << "stay_on_top_menu"
			<< "separator" << "show_file_properties" << "show_playlist"
			<< "show_log" << "separator" << "show_preferences";
	toolbar->setDefaultActions(actions);
	addToolBar(Qt::TopToolBarArea, toolbar);
	connect(editToolbarAct, SIGNAL(triggered()),
			toolbar, SLOT(edit()));

	action = toolbar->toggleViewAction();
	action->setObjectName("toggle_toolbar1");
	action->setShortcut(Qt::Key_F3);

	// Extra toolbar
	toolbar2 = new TEditableToolbar(this);
	toolbar2->setObjectName("toolbar2");
	actions.clear();
	actions << "audiotrack_menu" << "subtitlestrack_menu";
	toolbar2->setDefaultActions(actions);
	addToolBar(Qt::TopToolBarArea, toolbar2);
	connect(editToolbar2Act, SIGNAL(triggered()),
			toolbar2, SLOT(edit()));

	action = toolbar2->toggleViewAction();
	action->setObjectName("toggle_toolbar2");
	action->setShortcut(Qt::Key_F4);

	toolbar2->hide();

	// Statusbar
	statusBar()->setObjectName("statusbar");
	statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(statusBar(), SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(showStatusBarPopup(const QPoint&)));

	// Add toolbars to auto_hide_timer
	auto_hide_timer = new TAutoHideTimer(this, playerwindow);
	auto_hide_timer->add(controlbar->toggleViewAction(), controlbar);
	auto_hide_timer->add(toolbar->toggleViewAction(), toolbar);
	auto_hide_timer->add(toolbar2->toggleViewAction(), toolbar2);
	auto_hide_timer->add(viewMenuBarAct, menuBar());
	auto_hide_timer->add(viewStatusBarAct, statusBar());
	connect(playerwindow, SIGNAL(draggingChanged(bool)),
			auto_hide_timer, SLOT(setDraggingPlayerWindow(bool)));
}

void TBase::setupNetworkProxy() {
	//qDebug("Gui::TBase::setupNetworkProxy");

	QNetworkProxy proxy;

	if (pref->use_proxy && !pref->proxy_host.isEmpty()) {
		proxy.setType((QNetworkProxy::ProxyType) pref->proxy_type);
		proxy.setHostName(pref->proxy_host);
		proxy.setPort(pref->proxy_port);
		if ((!pref->proxy_username.isEmpty()) && (!pref->proxy_password.isEmpty())) {
			proxy.setUser(pref->proxy_username);
			proxy.setPassword(pref->proxy_password);
		}
		qDebug("Gui::TBase::setupNetworkProxy: using proxy: host: %s, port: %d, type: %d",
			   pref->proxy_host.toUtf8().constData(), pref->proxy_port, pref->proxy_type);
	} else {
		// No proxy
		proxy.setType(QNetworkProxy::NoProxy);
		qDebug("Gui::TBase::setupNetworkProxy: no proxy");
	}

	QNetworkProxy::setApplicationProxy(proxy);
}

void TBase::setActionsEnabled(bool b) {
	qDebug("Gui::TBase::setActionsEnabled: %d", b);

	// Time slider
	timeslider_action->enable(b);

	emit enableActions(!b, !core->mdat.noVideo(), core->mdat.audios.count() > 0);
}

void TBase::retranslateStrings() {
	qDebug("Gui::TBase::retranslateStrings");

	setWindowIcon(Images::icon("logo", 64));

	// Toolbars
	toolbar_menu->menuAction()->setText(tr("&Toolbars"));
	toolbar_menu->menuAction()->setIcon(Images::icon("toolbars"));

	// Main toolbar
	toolbar->setWindowTitle(tr("&Main toolbar"));
	toolbar->toggleViewAction()->setIcon(Images::icon("main_toolbar"));

	// Extra toolbar
	toolbar2->setWindowTitle(tr("&Extra toolbar"));
	toolbar2->toggleViewAction()->setIcon(Images::icon("extra_toolbar"));

	// Control bar
	controlbar->setWindowTitle(tr("&Control bar"));
	controlbar->toggleViewAction()->setIcon(Images::icon("controlbar"));

	// Status bar
	statusbar_menu->menuAction()->setText(tr("St&atusbar"));
	statusbar_menu->menuAction()->setIcon(Images::icon("statusbar"));

	// Sliders
	timeslider_action->setText(tr("Time slider"));
	volumeslider_action->setText(tr("Volume slider"));

	// Playlist
	playlist->retranslateStrings();

	// Log window
	log_window->retranslateStrings();

	// Help window
	if (help_window) {
		help_window->retranslateStrings();
	}

	// Update actions view in preferences
	// It has to be done, here. The actions are translated after the
	// preferences dialog.
	if (pref_dialog)
		pref_dialog->mod_input()->actions_editor->updateView();
} // retranslateStrings()

void TBase::setFloatingToolbarsVisible(bool visible) {

	if (toolbar->isFloating()) {
		toolbar->setVisible(visible);
	}
	if (toolbar2->isFloating()) {
		toolbar2->setVisible(visible);
	}
	if (controlbar->isFloating()) {
		controlbar->setVisible(visible);
	}
}

void TBase::showEvent(QShowEvent* event) {
	qDebug("Gui::TBase::showEvent");

	if (event) {
		QMainWindow::showEvent(event);
	}

	//qDebug("Gui::TBase::showEvent: pref->pause_when_hidden: %d", pref->pause_when_hidden);
	if (pref->pause_when_hidden && core->state() == STATE_PAUSED && !ignore_show_hide_events) {
		qDebug("Gui::TBase::showEvent: unpausing");
		core->play();
	}

	setFloatingToolbarsVisible(true);
}

void TBase::hideEvent(QHideEvent* event) {
	qDebug("Gui::TBase::hideEvent");

	if (event) {
		QMainWindow::hideEvent(event);
	}

	if (pref->pause_when_hidden && core->state() == STATE_PLAYING && !ignore_show_hide_events) {
		qDebug("Gui::TBase::hideEvent: pausing");
		core->pause();
	}

	setFloatingToolbarsVisible(false);
}

void TBase::changeEvent(QEvent* e) {

	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QMainWindow::changeEvent(e);

#if QT_VERSION_MAJOR >= 5
		// Emulate show/hide events for Qt >= 5
		if(e->type() == QEvent::WindowStateChange) {
			bool was_min = static_cast<QWindowStateChangeEvent*>(e)->oldState() == Qt::WindowMinimized;
			if (was_min) {
				if (!isMinimized()) {
					showEvent(0);
				}
			} else if (isMinimized()) {
				hideEvent(0);
			}
		}
#endif

	}
}

void TBase::setWindowCaption(const QString& title) {
	setWindowTitle(title);
}

void TBase::createPreferencesDialog() {

	QApplication::setOverrideCursor(Qt::WaitCursor);
	pref_dialog = new Pref::TDialog(this);
	pref_dialog->setModal(false);
	connect(pref_dialog, SIGNAL(applied()),
			 this, SLOT(applyNewPreferences()));
	QApplication::restoreOverrideCursor();
}

void TBase::createFilePropertiesDialog() {
	qDebug("Gui::TBase::createFilePropertiesDialog");

	QApplication::setOverrideCursor(Qt::WaitCursor);
	file_properties_dialog = new TFilePropertiesDialog(this, &core->mdat);
	file_properties_dialog->setModal(false);
	connect(file_properties_dialog, SIGNAL(applied()),
			 this, SLOT(applyFileProperties()));
	QApplication::restoreOverrideCursor();
}

void TBase::handleMessageFromOtherInstances(const QString& message) {
	qDebug("Gui::TBase::handleMessageFromOtherInstances: '%s'", message.toUtf8().constData());

	int pos = message.indexOf(' ');
	if (pos > -1) {
		QString command = message.left(pos);
		QString arg = message.mid(pos+1);
		qDebug("command: '%s'", command.toUtf8().constData());
		qDebug("arg: '%s'", arg.toUtf8().constData());

		if (command == "open_file") {
			emit openFileRequested();
			open(arg);
		}
		else
		if (command == "open_files") {
			QStringList file_list = arg.split(" <<sep>> ");
			emit openFileRequested();
			openFiles(file_list);
		}
		else
		if (command == "add_to_playlist") {
			QStringList file_list = arg.split(" <<sep>> ");
			playlist->addFiles(file_list);
		}
		else
		if (command == "media_title") {
			QStringList list = arg.split(" <<sep>> ");
			core->addForcedTitle(list[0], list[1]);
		}
		else
		if (command == "action") {
			processFunction(arg);
		}
		else
		if (command == "load_sub") {
			setInitialSubtitle(arg);
			if (core->state() != STATE_STOPPED) {
				core->loadSub(arg);
			}
		}
	}
}

TActionList TBase::getAllNamedActions() {

	// Get all actions with a name
	TActionList all_actions = findChildren<QAction*>();
	for (int i = all_actions.count() - 1; i >= 0; i--) {
		QAction* a = all_actions[i];
		if (a->objectName().isEmpty() || a->isSeparator()) {
			all_actions.removeAt(i);
		}
	}

	return all_actions;
}

void TBase::loadConfig() {
	qDebug("Gui::TBase::loadConfig");

	// Get all actions with a name
	TActionList all_actions = getAllNamedActions();
	// Load actions from outside group derived class
    Action::TActionsEditor::loadFromConfig(pref, all_actions);

	// Load from inside group derived class for backwards compatibility
	pref->beginGroup(settingsGroupName());

	if (pref->save_window_size_on_exit) {
		QPoint p = pref->value("pos", pos()).toPoint();
		QSize s = pref->value("size", size()).toSize();
		int state = pref->value("state", 0).toInt();
		if (s.width() < 200 || s.height() < 200) {
			s = pref->default_size;
		}

		move(p);
		resize(s);
		setWindowState((Qt::WindowStates) state);
		if (p.isNull()) {
            TDesktop::centerWindow(this);
		}
		TDesktop::keepInsideDesktop(this);
	} else {
        TDesktop::centerWindow(this);
		// Need to center again after video loaded
		center_window = true;
	}

	pref->beginGroup("actions");
	toolbar->setActionsFromStringList(pref->value("toolbar1",
		toolbar->defaultActions()).toStringList(), all_actions);
	toolbar2->setActionsFromStringList(pref->value("toolbar2",
		toolbar2->defaultActions()).toStringList(), all_actions);
	// Using old name "controlwidget" to pick up old toolbars
	controlbar->setActionsFromStringList(pref->value("controlwidget",
		controlbar->defaultActions()).toStringList(), all_actions);
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	toolbar->setIconSize(pref->value("toolbar1",
		toolbar->iconSize()).toSize());
	toolbar2->setIconSize(pref->value("toolbar2",
		toolbar2->iconSize()).toSize());
	// Using old name "controlwidget" to pick up old toolbars
	controlbar->setIconSize(pref->value("controlwidget",
		controlbar->iconSize()).toSize());
	pref->endGroup();

	menubar_visible = pref->value("menubar_visible", menubar_visible).toBool();
	viewMenuBarAct->setChecked(menubar_visible);
	fullscreen_menubar_visible = pref->value("fullscreen_menubar_visible", fullscreen_menubar_visible).toBool();

	statusbar_visible = pref->value("statusbar_visible", statusbar_visible).toBool();
	viewStatusBarAct->setChecked(statusbar_visible);
	fullscreen_statusbar_visible = pref->value("fullscreen_statusbar_visible", fullscreen_statusbar_visible).toBool();

	restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion());

	pref->endGroup();

	playlist->loadSettings();
	log_window->loadConfig();

	// Disable actions
	setActionsEnabled(false);
}

void TBase::saveConfig() {
	qDebug("Gui::TBase::saveConfig");

	pref->beginGroup(settingsGroupName());

	if (pref->save_window_size_on_exit) {
		pref->setValue("pos", pos());
		pref->setValue("size", size());
		pref->setValue("state", (int) windowState());
	}

	// Toolbars
	pref->beginGroup("actions");
	pref->setValue("toolbar1", toolbar->actionsToStringList());
	pref->setValue("toolbar2", toolbar2->actionsToStringList());
	// Using old name "controlwidget" for backward compat
	pref->setValue("controlwidget", controlbar->actionsToStringList());
	pref->endGroup();

	pref->beginGroup("toolbars_icon_size");
	pref->setValue("toolbar1", toolbar->iconSize());
	pref->setValue("toolbar2", toolbar2->iconSize());
	// Using old name "controlwidget" for backward compat
	pref->setValue("controlwidget", controlbar->iconSize());
	pref->endGroup();

	pref->setValue("menubar_visible", !menuBar()->isHidden());
	pref->setValue("fullscreen_menubar_visible", fullscreen_menubar_visible);
	pref->setValue("statusbar_visible", !statusBar()->isHidden());
	pref->setValue("fullscreen_statusbar_visible", fullscreen_statusbar_visible);

	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));

	pref->endGroup();

	playlist->saveSettings();
	log_window->saveConfig();
	if (help_window) {
		help_window->saveConfig();
	}
}

void TBase::save() {

    displayMessage(tr("Saving settings"), 0);
    if (pref->clean_config) {
        pref->clean_config = false;
        pref->remove("");
        Action::TActionsEditor::saveToConfig(pref, this);
    }
    saveConfig();
    pref->save();
}

void TBase::closeEvent(QCloseEvent* e)  {
	qDebug("Gui::TBase::closeEvent");

	core->close();
	exitFullscreen();
    save();
	e->accept();
}

void TBase::closeWindow() {
	qDebug("Gui::TBase::closeWindow");

	close();
}

// Overriden by TBasePlus
void TBase::showPlaylist(bool b) {
	playlist->setVisible(b);
}

void TBase::showPreferencesDialog() {
	qDebug("Gui::TBase::showPreferencesDialog");

	if (!pref_dialog) {
		createPreferencesDialog();
	}

	pref_dialog->setData(pref);

	pref_dialog->mod_input()->actions_editor->clear();
	pref_dialog->mod_input()->actions_editor->addActions(this);

	// Set playlist preferences
	pref_dialog->mod_interface()->setDirectoryRecursion(playlist->directoryRecursion());
	pref_dialog->mod_interface()->setSavePlaylistOnExit(playlist->savePlaylistOnExit());

	pref_dialog->show();
}

void TBase::restartWZPlayer(bool reset_style) {
	qDebug("Gui::TBase::restartWZPlayer");

	emit requestRestart(reset_style);
	// Close and restart with the new settings
	close();
	return;
}

// The user has pressed OK in preferences dialog
void TBase::applyNewPreferences() {
	qDebug("Gui::TBase::applyNewPreferences");

	QString old_player_bin = pref->player_bin;

	// Update pref from dialog
	pref_dialog->getData(pref);

    // Update and save playlist preferences
    Pref::TInterface* mod_interface = pref_dialog->mod_interface();
    playlist->setDirectoryRecursion(mod_interface->directoryRecursion());
    playlist->setSavePlaylistOnExit(mod_interface->savePlaylistOnExit());
    playlist->saveSettings();

    // Update and save actions
	pref_dialog->mod_input()->actions_editor->applyChanges();
    Action::TActionsEditor::saveToConfig(pref, this);

	// Commit changes
    pref->save();

	// Style change needs recreation of main window
    if (mod_interface->styleChanged()) {
		// Request restart and optional reset style to default
        qDebug("Gui::TBase::applyNewPreferences: style changed, restarting");
		restartWZPlayer(pref->style.isEmpty());
		return;
	}

    // Player bin, icon set or language change need restart TApp
    if (pref->player_bin != old_player_bin
        || mod_interface->iconsetChanged()
        || mod_interface->languageChanged()) {
		// Request restart, don't reset style
        qDebug("Gui::TBase::applyNewPreferences: playerbin, icon set or language changed, restarting");
        restartWZPlayer(false);
		return;
	}

	// Keeping the current main window

	// Update logging
	TLog::log->setLogDebugMessages(pref->log_debug_enabled);
	// Verbose handled by restart core
	TLog::log->setLogFileEnabled(pref->log_file);

	// Update application font
	if (!pref->default_font.isEmpty()) {
		QFont f;
		f.fromString(pref->default_font);
		if (QApplication::font() != f) {
			qDebug() << "Gui::TBase::applyNewPreferences: setting new font"
					 << pref->default_font;
			QApplication::setFont(f);
		}
	}

	// Set color key, depends on VO
	playerwindow->setColorKey();

	// Forced demuxer
	core->mset.forced_demuxer = pref->use_lavf_demuxer ? "lavf" : "";

	// Video equalizer
	video_equalizer->setBySoftware(pref->use_soft_video_eq);

	// Subtitles
	subtitleMenu->useCustomSubStyleAct->setChecked(pref->use_custom_ass_style);

	// Interface continued
	// Show panel
	if (!pref->hide_video_window_on_audio_files && !panel->isVisible()) {
		resize(width(), height() + 200);
		panel->show();
	}
	// Show tags in window title
	onMediaInfoChanged();
	// Hide toolbars delay
	auto_hide_timer->setInterval(pref->floating_hide_delay);
	// Recents
    if (mod_interface->recentsChanged()) {
		openMenu->updateRecents();
	}

	// Keyboard and mouse
	playerwindow->setDelayLeftClick(pref->delay_left_click);

	// Network
	setupNetworkProxy();

	// Reenable actions to reflect changes
	setActionsEnabled(core->state() != STATE_STOPPED);

    // TODO: move code above to preferencesChanged() signal
    emit preferencesChanged();

	// Restart video if needed
	if (pref_dialog->requiresRestart()) {
		core->restart();
	}
} // TBase::applyNewPreferences()

void TBase::showFilePropertiesDialog() {
	qDebug("Gui::TBase::showFilePropertiesDialog");

	if (!file_properties_dialog) {
		createFilePropertiesDialog();
	}
	setDataToFileProperties();
	file_properties_dialog->show();
}

void TBase::setDataToFileProperties() {
	qDebug("TBase::setDataToFileProperties");

	InfoReader *i = InfoReader::obj();
	i->getInfo();
	file_properties_dialog->setCodecs(i->vcList(), i->acList(), i->demuxerList());

	// Save a copy of the demuxer, video and audio codec
	if (core->mset.original_demuxer.isEmpty()) 
		core->mset.original_demuxer = core->mdat.demuxer;
	if (core->mset.original_video_codec.isEmpty()) 
		core->mset.original_video_codec = core->mdat.video_codec;
	if (core->mset.original_audio_codec.isEmpty()) 
		core->mset.original_audio_codec = core->mdat.audio_codec;

	// Set demuxer, video and audio codec
	QString demuxer = core->mset.forced_demuxer;
	if (demuxer.isEmpty())
		demuxer = core->mdat.demuxer;
	QString vc = core->mset.forced_video_codec;
	if (vc.isEmpty())
		vc = core->mdat.video_codec;
	QString ac = core->mset.forced_audio_codec;
	if (ac.isEmpty())
		ac = core->mdat.audio_codec;

	file_properties_dialog->setDemuxer(demuxer, core->mset.original_demuxer);
	file_properties_dialog->setAudioCodec(ac, core->mset.original_audio_codec);
	file_properties_dialog->setVideoCodec(vc, core->mset.original_video_codec);

	file_properties_dialog->setPlayerAdditionalArguments(core->mset.player_additional_options);
	file_properties_dialog->setPlayerAdditionalVideoFilters(core->mset.player_additional_video_filters);
	file_properties_dialog->setPlayerAdditionalAudioFilters(core->mset.player_additional_audio_filters);

	file_properties_dialog->showInfo();
}

void TBase::applyFileProperties() {
	qDebug("Gui::TBase::applyFileProperties");

	bool need_restart = false;

#undef TEST_AND_SET
#define TEST_AND_SET(Pref, Dialog) \
	if (Pref != Dialog) { Pref = Dialog; need_restart = true; }

	bool demuxer_changed = false;

	QString prev_demuxer = core->mset.forced_demuxer;

	QString demuxer = file_properties_dialog->demuxer();
	if (demuxer == core->mset.original_demuxer) demuxer="";
	TEST_AND_SET(core->mset.forced_demuxer, demuxer);

	if (prev_demuxer != core->mset.forced_demuxer) {
		// Demuxer changed
		demuxer_changed = true;
		core->mset.current_audio_id = TMediaSettings::NoneSelected;
		core->mset.current_sub_idx = TMediaSettings::NoneSelected;
	}

	QString ac = file_properties_dialog->audioCodec();
	if (ac == core->mset.original_audio_codec) ac="";
	TEST_AND_SET(core->mset.forced_audio_codec, ac);

	QString vc = file_properties_dialog->videoCodec();
	if (vc == core->mset.original_video_codec) vc="";
	TEST_AND_SET(core->mset.forced_video_codec, vc);

	TEST_AND_SET(core->mset.player_additional_options, file_properties_dialog->playerAdditionalArguments());
	TEST_AND_SET(core->mset.player_additional_video_filters, file_properties_dialog->playerAdditionalVideoFilters());
	TEST_AND_SET(core->mset.player_additional_audio_filters, file_properties_dialog->playerAdditionalAudioFilters());

#undef TEST_AND_SET

	// Restart the video to apply
	if (need_restart) {
		if (demuxer_changed) {
			core->reload();
		} else {
			core->restart();
		}
	}
}

void TBase::onMediaInfoChanged() {
	qDebug("Gui::TBase::onMediaInfoChanged");

	if (file_properties_dialog && file_properties_dialog->isVisible()) {
		setDataToFileProperties();
	}

	QString title = core->mdat.displayName(pref->show_tag_in_window_title);
	setWindowCaption(title + " - " + TConfig::PROGRAM_NAME);
	emit mediaFileTitleChanged(core->mdat.filename, title);
}

void TBase::onNewMediaStartedPlaying() {
	qDebug("Gui::TBase::onNewMediaStartedPlaying");

	enterFullscreenOnPlay();

	// Recents
	pref->history_recents.addItem(core->mdat.filename, core->mdat.title);
	openMenu->updateRecents();

	checkPendingActionsToRun();
}

void TBase::updateVideoEqualizer() {
	qDebug("Gui::TBase::updateVideoEqualizer");

	video_equalizer->setContrast(core->mset.contrast);
	video_equalizer->setBrightness(core->mset.brightness);
	video_equalizer->setHue(core->mset.hue);
	video_equalizer->setSaturation(core->mset.saturation);
	video_equalizer->setGamma(core->mset.gamma);
}

void TBase::updateAudioEqualizer() {
	qDebug("Gui::TBase::updateAudioEqualizer");

	audio_equalizer->setEqualizer(core->getAudioEqualizer());
}

void TBase::setDefaultValuesFromVideoEqualizer() {
	qDebug("Gui::TBase::setDefaultValuesFromVideoEqualizer");

	pref->initial_contrast = video_equalizer->contrast();
	pref->initial_brightness = video_equalizer->brightness();
	pref->initial_hue = video_equalizer->hue();
	pref->initial_saturation = video_equalizer->saturation();
	pref->initial_gamma = video_equalizer->gamma();

	QMessageBox::information(this, tr("Information"), 
							 tr("The current values have been stored to be "
								"used as default."));
}

void TBase::changeVideoEqualizerBySoftware(bool b) {
	qDebug("Gui::TBase::changeVideoEqualizerBySoftware: %d", b);

	if (b != pref->use_soft_video_eq) {
		pref->use_soft_video_eq = b;
		core->restart();
	}
}

void TBase::openDirectory() {
	qDebug("Gui::TBase::openDirectory");

	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Choose a directory"),
					pref->latest_dir);

	if (!s.isEmpty()) {
		playlist->playDirectory(s);
	}
}

void TBase::open(const QString &file) {
	qDebug() << "Gui::TBase::open:" << file;

	if (file.isEmpty()) {
		qWarning("Gui::TBase::open: filename is empty");
		return;
	}
	if (!playlist->maybeSave()) {
		return;
	}

	QFileInfo fi(file);
	if (fi.exists()) {
		if (fi.isDir()) {
			playlist->playDirectory(file);
			return;
		}
		pref->latest_dir = fi.absolutePath();
	}

	core->open(file);
	qDebug("Gui::TBase::open: done");
}

void TBase::openFiles(QStringList files, int item) {
	qDebug("Gui::TBase::openFiles");

	if (files.empty()) {
		qWarning("Gui::TBase::openFiles: no files in list to open");
		return;
	}

#ifdef Q_OS_WIN
	files = Helper::resolveSymlinks(files); // Check for Windows shortcuts
#endif

	if (files.count() == 1) {
		open(files[0]);
	} else if (playlist->maybeSave()) {
		qDebug("Gui::TBase::openFiles: starting new playlist");
		if (item < 0)
			files.sort();
		playlist->clear();
		playlist->addFiles(files);
		if (item >= 0) {
			playlist->playItem(item);
		} else {
			playlist->startPlay();
		}
	}
}

void TBase::openFile() {
	qDebug("Gui::TBase::openFile");

	TExtensions e;
	QString s = MyFileDialog::getOpenFileName(
					   this, tr("Choose a file"), pref->latest_dir,
					   tr("Multimedia") + e.allPlayable().forFilter()+";;" +
					   tr("Video") + e.video().forFilter()+";;" +
					   tr("Audio") + e.audio().forFilter()+";;" +
					   tr("Playlists") + e.playlist().forFilter()+";;" +
					   tr("All files") +" (*.*)");

	if (!s.isEmpty()) {
		open(s);
	}
}

void TBase::openRecent() {
	qDebug("Gui::TBase::openRecent");

	QAction *a = qobject_cast<QAction *> (sender());
	if (a) {
		int item = a->data().toInt();
		QString filename = pref->history_recents.item(item);
		if (!filename.isEmpty())
			open(filename);
	}
}

void TBase::openURL() {
	qDebug("Gui::TBase::openURL");

	TInputURL d(this);

	// Get url from clipboard
	QString clipboard_text = QApplication::clipboard()->text();
	if (!clipboard_text.isEmpty() && (clipboard_text.contains("/")

#ifdef Q_OS_WIN
			|| clipboard_text.contains("\\")
#endif

		)) {
		d.setURL(clipboard_text);
	}

	for (int n = 0; n < pref->history_urls.count(); n++) {
		d.setURL(pref->history_urls.url(n));
	}

	if (d.exec() == QDialog::Accepted) {
		QString url = d.url();
		if (!url.isEmpty()) {
			pref->history_urls.addUrl(url);
			open(url);
		}
	}
}

void TBase::configureDiscDevices() {
	QMessageBox::information(this, TConfig::PROGRAM_NAME + tr(" - Information"),
			tr("The CDROM / DVD drives are not configured yet.\n"
			   "The configuration dialog will be shown now, "
			   "so you can do it."), QMessageBox::Ok);
	
	showPreferencesDialog();
	pref_dialog->showSection(Pref::TDialog::SECTION_DRIVES);
}

void TBase::openVCD() {
	qDebug("Gui::TBase::openVCD");

	if (pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		core->openDisc(TDiscName("vcd", pref->vcd_initial_title, pref->cdrom_device));
	}
}

void TBase::openAudioCD() {
	qDebug("Gui::TBase::openAudioCD");

	if (pref->cdrom_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		core->open("cdda://");
	}
}

void TBase::openDVD() {
	qDebug("Gui::TBase::openDVD");

	if (pref->dvd_device.isEmpty()) {
		configureDiscDevices();
	} else if (playlist->maybeSave()) {
		core->openDisc(TDiscName(pref->dvd_device, pref->useDVDNAV()));
	}
}

void TBase::openDVDFromFolder() {
	qDebug("Gui::TBase::openDVDFromFolder");

	if (playlist->maybeSave()) {
		TInputDVDDirectory *d = new TInputDVDDirectory(this);
		d->setFolder(pref->last_dvd_directory);
		if (d->exec() == QDialog::Accepted) {
			qDebug("Gui::TBase::openDVDFromFolder: accepted");
			openDVDFromFolder(d->folder());
		}

		delete d;
	}
}

void TBase::openDVDFromFolder(const QString &directory) {

	pref->last_dvd_directory = directory;
	core->openDisc(TDiscName(directory, pref->useDVDNAV()));
}

void TBase::openBluRay() {
	qDebug("Gui::TBase::openBluRay");

	if (pref->bluray_device.isEmpty()) {
		configureDiscDevices();
	} else {
		core->openDisc(TDiscName("br", 0, pref->bluray_device));
	}
}

void TBase::openBluRayFromFolder() {
	qDebug("Gui::TBase::openBluRayFromFolder");

	if (playlist->maybeSave()) {
		QString dir = QFileDialog::getExistingDirectory(this, tr("Select the Blu-ray folder"),
			pref->last_dvd_directory, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (!dir.isEmpty()) {
			pref->last_dvd_directory = dir;
			core->openDisc(TDiscName("br", 0, dir));
		}
	}
}

void TBase::loadSub() {
	qDebug("Gui::TBase::loadSub");

	TExtensions e;
	QString s = MyFileDialog::getOpenFileName(
		this, tr("Choose a file"),
		pref->latest_dir,
		tr("Subtitles") + e.subtitles().forFilter()+ ";;" +
		tr("All files") +" (*.*)");

	if (!s.isEmpty())
		core->loadSub(s);
}

void TBase::setInitialSubtitle(const QString & subtitle_file) {
	qDebug("Gui::TBase::setInitialSubtitle: '%s'", subtitle_file.toUtf8().constData());

	core->setInitialSubtitle(subtitle_file);
}

void TBase::loadAudioFile() {
	qDebug("Gui::TBase::loadAudioFile");

	TExtensions e;
	QString s = MyFileDialog::getOpenFileName(
		this, tr("Choose a file"),
		pref->latest_dir,
		tr("Audio") + e.audio().forFilter()+";;" +
		tr("All files") +" (*.*)");

	if (!s.isEmpty())
		core->loadAudioFile(s);
}

void TBase::helpCLOptions() {

	if (help_window == 0) {
		help_window = new TLogWindow(this, "helpwindow");
		help_window->setWindowTitle(tr("%1 command line options").arg(TConfig::PROGRAM_NAME));
		help_window->loadConfig();
	}

	// Hide event clears the help window content, so recreate it
	help_window->setHtml(CLHelp::help(true));
	help_window->show();
}

void TBase::helpCheckUpdates() {
	update_checker->check();
}

void TBase::showConfigFolder() {
	QDesktopServices::openUrl(QUrl::fromLocalFile(TPaths::configPath()));
}

void TBase::helpAbout() {
	TAbout d(this);
	d.exec();
}

void TBase::showSeekToDialog() {

	TTimeDialog d(this);
    d.setWindowTitle(tr("Seek"));
	d.setMaximumTime((int) core->mdat.duration);
	d.setTime((int) core->mset.current_sec);
	if (d.exec() == QDialog::Accepted) {
        core->seekTime(d.time());
	}
}

void TBase::showAudioDelayDialog() {
	bool ok;
	#if QT_VERSION >= 0x050000
	int delay = QInputDialog::getInt(this, tr("%1 - Audio delay").arg(TConfig::PROGRAM_NAME),
		tr("Audio delay (in milliseconds):"), core->mset.audio_delay,
		-3600000, 3600000, 1, &ok);
	#else
	int delay = QInputDialog::getInteger(this, tr("%1 - Audio delay").arg(TConfig::PROGRAM_NAME),
		tr("Audio delay (in milliseconds):"), core->mset.audio_delay,
		-3600000, 3600000, 1, &ok);
	#endif
	if (ok) {
		core->setAudioDelay(delay);
	}
}

void TBase::showSubDelayDialog() {
	bool ok;
	#if QT_VERSION >= 0x050000
	int delay = QInputDialog::getInt(this, tr("%1 - Subtitle delay").arg(TConfig::PROGRAM_NAME),
		tr("Subtitle delay (in milliseconds):"), core->mset.sub_delay,
		-3600000, 3600000, 1, &ok);
	#else
	int delay = QInputDialog::getInteger(this, tr("%1 - Subtitle delay").arg(TConfig::PROGRAM_NAME),
		tr("Subtitle delay (in milliseconds):"), core->mset.sub_delay,
		-3600000, 3600000, 1, &ok);
	#endif
	if (ok) {
		core->setSubDelay(delay);
	}
}

void TBase::showStereo3dDialog() {
	TStereo3dDialog d(this);
	d.setInputFormat(core->mset.stereo3d_in);
	d.setOutputFormat(core->mset.stereo3d_out);

	if (d.exec() == QDialog::Accepted) {
		core->changeStereo3d(d.inputFormat(), d.outputFormat());
	}
}

void TBase::exitFullscreen() {
	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
}

void TBase::toggleFullscreen() {
	qDebug("Gui::TBase::toggleFullscreen");

	toggleFullscreen(!pref->fullscreen);
}

void TBase::toggleFullscreen(bool b) {
	qDebug("Gui::TBase::toggleFullscreen: %d", b);

	if (b == pref->fullscreen) {
		qDebug("Gui::TBase::toggleFullscreen: nothing to do, returning");
		return;
	}
	pref->fullscreen = b;
	emit fullscreenChanged();

	if (pref->fullscreen) {
		aboutToEnterFullscreen();
		showFullScreen();
		didEnterFullscreen();
	} else {
		aboutToExitFullscreen();
		showNormal();
		didExitFullscreen();
	}

	setFocus(); // Fixes bug #2493415
}

void TBase::aboutToEnterFullscreen() {
	//qDebug("Gui::TBase::aboutToEnterFullscreen");

	emit aboutToEnterFullscreenSignal();

	// Save current state
    was_maximized = isMaximized();
	menubar_visible = !menuBar()->isHidden();
	statusbar_visible = !statusBar()->isHidden();

	pref->beginGroup(settingsGroupName());
	pref->setValue("toolbars_state", saveState(Helper::qtVersion()));
	pref->endGroup();
}

void TBase::didEnterFullscreen() {
	//qDebug("Gui::TBase::didEnterFullscreen");

	// Restore fullscreen state
	viewMenuBarAct->setChecked(fullscreen_menubar_visible);
	viewStatusBarAct->setChecked(fullscreen_statusbar_visible);

	pref->beginGroup(settingsGroupName());
	if (!restoreState(pref->value("toolbars_state_fullscreen").toByteArray(),
					  Helper::qtVersion())) {
		// First time there is no fullscreen toolbar state
		qWarning("Gui::TBase::didEnterFullscreen: failed to restore fullscreen toolbar state");
		toolbar->hide();
		toolbar2->hide();
	}
	pref->endGroup();

	emit didEnterFullscreenSignal();

	auto_hide_timer->start();
}

void TBase::aboutToExitFullscreen() {
	//qDebug("Gui::TBase::aboutToExitFullscreen");

	auto_hide_timer->stop();

	// Save fullscreen state
	fullscreen_menubar_visible = !menuBar()->isHidden();
	fullscreen_statusbar_visible = !statusBar()->isHidden();

	pref->beginGroup(settingsGroupName());
	pref->setValue("toolbars_state_fullscreen", saveState(Helper::qtVersion()));
	pref->endGroup();
}

void TBase::didExitFullscreen() {
	//qDebug("Gui::TBase::didExitFullscreen");

    // Restore maximizednormal state
	if (was_maximized) {
		showMaximized();
    }

	viewMenuBarAct->setChecked(menubar_visible);
	viewStatusBarAct->setChecked(statusbar_visible);

	pref->beginGroup(settingsGroupName());
	if (!restoreState(pref->value("toolbars_state").toByteArray(), Helper::qtVersion()))
		qWarning("Gui::TBase::didExitFullscreen: failed to restore toolbar state");
	pref->endGroup();

	emit didExitFullscreenSignal();
}

void TBase::leftClickFunction() {
	qDebug("Gui::TBase::leftClickFunction");

	if (core->mdat.detected_type == TMediaData::TYPE_DVDNAV
		&& playerwindow->videoWindow()->underMouse()) {
		core->dvdnavMouse();
	} else if (!pref->mouse_left_click_function.isEmpty()) {
		processFunction(pref->mouse_left_click_function);
	}
}

void TBase::rightClickFunction() {
	qDebug("Gui::TBase::rightClickFunction");

	if (!pref->mouse_right_click_function.isEmpty()) {
		processFunction(pref->mouse_right_click_function);
	}
}

void TBase::doubleClickFunction() {
	qDebug("Gui::TBase::doubleClickFunction");

	if (!pref->mouse_double_click_function.isEmpty()) {
		processFunction(pref->mouse_double_click_function);
	}
}

void TBase::middleClickFunction() {
	qDebug("Gui::TBase::middleClickFunction");

	if (!pref->mouse_middle_click_function.isEmpty()) {
		processFunction(pref->mouse_middle_click_function);
	}
}

void TBase::xbutton1ClickFunction() {
	qDebug("Gui::TBase::xbutton1ClickFunction");

	if (!pref->mouse_xbutton1_click_function.isEmpty()) {
		processFunction(pref->mouse_xbutton1_click_function);
	}
}

void TBase::xbutton2ClickFunction() {
	qDebug("Gui::TBase::xbutton2ClickFunction");

	if (!pref->mouse_xbutton2_click_function.isEmpty()) {
		processFunction(pref->mouse_xbutton2_click_function);
	}
}

void TBase::moveWindowMerged() {

	move(pos() + move_window_diff);
	move_window_diff = QPoint(0, 0);
}

// Called by playerwindow when dragging main window
void TBase::moveWindow(QPoint diff) {

	// Merge multiple moves into one for machines that cannot keep up
	move_window_diff += diff;
	// Zero timeout, calls moveWindowMerged()
	move_window_timer.start();
}

void TBase::processFunction(QString function) {

	// Check function for checkable actions
	static QRegExp func_rx("(.*) (true|false)");
	bool value = false;
	bool checkableFunction = false;

	if (func_rx.indexIn(function) >= 0) {
		function = func_rx.cap(1);
		value = func_rx.cap(2) == "true";
		checkableFunction = true;
	}

    QAction* action = findChild<QAction*>(function);
	if (action) {
		if (action->isEnabled()) {
			if (action->isCheckable() && checkableFunction) {
				qDebug() << "Gui::TBase::processFunction: setting checked action"
						 << function << value;
				action->setChecked(value);
			} else {
				qDebug() << "Gui::TBase::processFunction: triggering action" << function;
				action->trigger();
			}
		} else {
			qDebug() << "Gui::TBase::processFunction: canceling disabled action"
					 << function;
		}
	} else {
		qWarning() << "Gui::TBase::processFunction: action" << function << "not found";
	}
}

void TBase::runActions(QString actions) {
	qDebug("Gui::TBase::runActions");

	actions = actions.simplified(); // Remove white space

	QAction* action;
	QStringList actionsList = actions.split(" ");

	for (int n = 0; n < actionsList.count(); n++) {
		QString actionStr = actionsList[n];
		QString par = ""; //the parameter which the action takes

		//set par if the next word is a boolean value
		if ((n+1) < actionsList.count()) {
			if ((actionsList[n+1].toLower() == "true") || (actionsList[n+1].toLower() == "false")) {
				par = actionsList[n+1].toLower();
				n++;
			} //end if
		} //end if

        action = findChild<QAction*>(actionStr);
        if (action) {
			qDebug("Gui::TBase::runActions: running action: '%s' (par: '%s')",
				   actionStr.toUtf8().data(), par.toUtf8().data());

			if (action->isCheckable()) {
				if (par.isEmpty()) {
					//action->toggle();
					action->trigger();
				} else {
					action->setChecked((par == "true"));
				} //end if
			} else {
				action->trigger();
			} //end if
		} else {
			qWarning("Gui::TBase::runActions: action: '%s' not found",actionStr.toUtf8().data());
		} //end if
	} //end for
}

void TBase::checkPendingActionsToRun() {
	qDebug("Gui::TBase::checkPendingActionsToRun");

	QString actions;
	if (!pending_actions_to_run.isEmpty()) {
		actions = pending_actions_to_run;
		pending_actions_to_run.clear();
		if (!pref->actions_to_run.isEmpty()) {
			actions = pref->actions_to_run +" "+ actions;
		}
	} else {
		actions = pref->actions_to_run;
	}

	if (!actions.isEmpty()) {
		qDebug("Gui::TBase::checkPendingActionsToRun: actions: '%s'", actions.toUtf8().constData());
		runActions(actions);
	}
}

void TBase::dragEnterEvent(QDragEnterEvent *e) {
	qDebug("Gui::TBase::dragEnterEvent");

	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void TBase::dropEvent(QDropEvent *e) {
	qDebug("Gui::TBase::dropEvent");

	QStringList files;

	if (e->mimeData()->hasUrls()) {
		QList <QUrl> urls = e->mimeData()->urls();
		for (int n = 0; n < urls.count(); n++) {
			QUrl url = urls[n];
			if (url.isValid()) {
				QString filename;
				if (url.scheme() == "file")
					filename = url.toLocalFile();
				else filename = url.toString();
				qDebug() << "Gui::TBase::dropEvent: adding" << filename;
				files.append(filename);
			} else {
				qWarning() << "Gui::TBase::dropEvent:: ignoring" << url.toString();
			}
		}
	}

	qDebug("Gui::TBase::dropEvent: number of files: %d", files.count());
	openFiles(files);
}

void TBase::showContextMenu() {
	showContextMenu(QCursor::pos());
}

void TBase::showContextMenu(QPoint p) {
	//qDebug("Gui::TBase::showContextMenu: %d, %d", p.x(), p.y());
	execPopup(this, popup, p);
}

// Called when a video has started to play
void TBase::enterFullscreenOnPlay() {
	//qDebug("Gui::TBase::enterFullscreenOnPlay: arg_start_in_fullscreen: %d, pref->start_in_fullscreen: %d",
	//	   arg_start_in_fullscreen, pref->start_in_fullscreen);

	if (arg_start_in_fullscreen != 0) {
		if ((arg_start_in_fullscreen == 1) || (pref->start_in_fullscreen)) {
			if (!pref->fullscreen)
				toggleFullscreen(true);
		}
	}
}

// Called when the playlist has stopped
void TBase::exitFullscreenOnStop() {

	if (pref->fullscreen) {
		toggleFullscreen(false);
	}
}

void TBase::playlistHasFinished() {
	qDebug("Gui::TBase::playlistHasFinished");

	core->stop();

	qDebug("Gui::TBase::playlistHasFinished: arg_close_on_finish: %d, pref->close_on_finish: %d", arg_close_on_finish, pref->close_on_finish);

	if (arg_close_on_finish != 0) {
		if ((arg_close_on_finish == 1) || (pref->close_on_finish)) {
			close();
		}
	}
}

void TBase::onStateChanged(TCoreState state) {
	qDebug() << "Gui::TBase::onStateChanged: new state" << core->stateToString();

	switch (state) {
		case STATE_PLAYING:
			displayMessage(tr("Playing %1").arg(core->mdat.displayName()));
			auto_hide_timer->startAutoHideMouse();
			break;
		case STATE_PAUSED:
			displayMessage(tr("Paused"));
			auto_hide_timer->stopAutoHideMouse();
			break;
		case STATE_STOPPED:
			setActionsEnabled(false);
			setWindowCaption(TConfig::PROGRAM_NAME);
			displayMessage(tr("Stopped"));
			auto_hide_timer->stopAutoHideMouse();
			break;
	}
}

void TBase::displayMessage(const QString& message, int time) {
	statusBar()->showMessage(message, time);
}

void TBase::onPositionChanged(double sec) {
    //qDebug("Gui::TBase::onPositionChanged: %f", sec);

	QString time =
		Helper::formatTime((int) sec) + " / " +
		Helper::formatTime(qRound(core->mdat.duration));
	time_label_action->setText(time);

	emit timeChanged(time);
}

void TBase::gotDuration(double duration) {
	Q_UNUSED(duration)

	// Uses duration in text
    onPositionChanged(core->mset.current_sec);
}

void TBase::changeSize(double factor) {
	qDebug("TBase::changeSize: %f", factor);

	if (core->mdat.noVideo())
		return;

	if (pref->fullscreen) {
		// Adjust zoom to match the requested size factor
		QSize video_size = QSize(core->mdat.video_out_width,
								 core->mdat.video_out_height) * factor;
		QSize desktop_size = TDesktop::size(this);
		double zoom = (double) video_size.width() / desktop_size.width();
		double zoomY = (double) video_size.height() / desktop_size.height();
		if (zoomY > zoom) {
			zoom = zoomY;
		}
		core->setZoom(zoom);
	} else {
		// Normal screen
		pref->size_factor = factor;
		bool center = false;
		if (isMaximized()) {
			showNormal();
			// Need center, on X windows the original pos is not restored
			center = true;
		}
		resizeWindow(core->mdat.video_out_width, core->mdat.video_out_height);
		if (center) {
            TDesktop::centerWindow(this);
		}
	}

	core->displayMessage(tr("Size %1%").arg(QString::number(qRound(factor * 100))));
}

void TBase::changeSize(int percentage) {
	qDebug("TBase::changeSize: %d%%", percentage);
	changeSize((double) percentage / 100);
}

void TBase::toggleDoubleSize() {

	if (pref->size_factor != 1.0)
		changeSize(1.0);
	else changeSize(2.0);
}

void TBase::hidePanel() {
	qDebug("Gui::TBase::hidePanel");

	if (panel->isVisible()) {
		// Exit from fullscreen mode
		if (pref->fullscreen) {
			toggleFullscreen(false);
			update();
		}

		int width = this->width();
		if (width > pref->default_size.width())
			width = pref->default_size.width();
		resize(width, height() - panel->height());
		panel->hide();
	}
}

double TBase::getNewSizeFactor() {

	double size_factor = 1.0;
	QSize available_size = TDesktop::availableSize(playerwindow);
	QSize res = playerwindow->resolution();
	QSize video_size = res;

	if (pref->fullscreen) {
		size_factor = (double) available_size.width() / video_size.width();
		double size_factor_y = (double) available_size.height() / video_size.height();
		if (size_factor_y < size_factor) {
			return size_factor_y;
		}
		return size_factor;
	}

	// Limit size to 0.6 of available desktop
	const double f = 0.6;
	double factor;
	double max = f * available_size.height();
	// Adjust height first
	if (video_size.height() > max) {
		factor = max / res.height();
		qDebug("Gui::TBase::getNewSizeFactor: height larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = res * size_factor;
	}
	// Adjust width
	max = f * available_size.width();
	if (video_size.width() > max) {
		factor = max / res.width();
		qDebug("Gui::TBase::getNewSizeFactor: width larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = res * size_factor;
	}

	if (size_factor != 1.0) {
		// Round to predefined values
		int factor_int = qRound(size_factor * 100);
		const int factors[] = {25, 50, 75, 100, 125, 150, 175, 200, 300, 400 };
		for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); i++) {
			int predef = factors[i];
			int d = predef / 10;
			if (d < 10) d = 10;
			if (qAbs(factor_int - predef) < d) {
				factor = (double) predef / 100;
				qDebug("Gui::TBase::getNewSizeFactor: rounding size factor from %f to %f",
					   size_factor, factor);
				return factor;
			}
		}

		// Make width multiple of 16
		int new_w = ((video_size.width() + 8) / 16) * 16;
		if (new_w != video_size.width()) {
			size_factor = qRound((double) new_w / res.width());
			qDebug("Gui::TBase::getNewSizeFactor: optimizing size_factor to %f for width %d",
				   size_factor, new_w);
		}
	}

	qDebug("Gui::TBase::getNewSizeFactor: selected size factor %f", size_factor);
	return size_factor;
}

// Slot called by signal videoOutResolutionChanged
void TBase::onVideoOutResolutionChanged(int w, int h) {
	qDebug("Gui::TBase::onVideoOutResolutionChanged: %d, %d", w, h);

	if (w <= 0 || h <= 0) {
		// No video
		if (pref->hide_video_window_on_audio_files) {
			hidePanel();
		}
	} else {
		// Have video
		if (!panel->isVisible()) {
			panel->show();
		}
		// force_resize is only set for the first video
		// when pref->save_window_size_on_exit not set.
		// Leave maximized window as is.
		if (!isMaximized() && (pref->resize_on_load || force_resize)) {
			// Try size factor 1.0
			pref->size_factor = getNewSizeFactor();
			resizeWindow(w, h);
		} else {
			// Adjust the size factor to the current window size
			playerwindow->updateSizeFactor();
		}
	}

	if (center_window) {
		center_window = false;
		if (!isMaximized()) {
            TDesktop::centerWindow(this);
		}
	}

	force_resize = false;
}

// Slot called by signal needResize
void TBase::resizeWindow(int w, int h) {
	// qDebug("Gui::TBase::resizeWindow: %d, %d", w, h);

	if (!pref->fullscreen && !isMaximized()) {
		resizeMainWindow(w, h, pref->size_factor);
		TDesktop::keepInsideDesktop(this);
	}
}

void TBase::resizeMainWindow(int w, int h, double size_factor, bool try_twice) {
	qDebug("Gui::TBase::resizeMainWindow: requested video size %d x %d, size factor %f",
		   w, h, pref->size_factor);

	QSize panel_size = QSize(w, h) * size_factor;
	if (panel_size == panel->size()) {
		qDebug("Gui::TBase::resizeMainWindow: panel has requested size");
		return;
	}

	QSize new_size = size() + panel_size - panel->size();
	//qDebug("Gui::TBase::resizeMainWindow: resizing window from %d x %d to %d x %d",
	//	   width(), height(), new_size.width(), new_size.height());
	resize(new_size);

	if (panel->size() != panel_size) {
		// Resizing the main window can change the height of the tool bars,
		// which will change the height of the panel during the resize.
		// Often fixed by resizing once again, using the new panel height.
		if (try_twice) {
			//qDebug("Gui::TBase::resizeMainWindow: panel size now %d x %d. Wanted size %d x %d. Trying a second time",
			//	   panel->size().width(), panel->size().height(),
			//	   panel_size.width(), panel_size.height());
			resizeMainWindow(w, h, size_factor, false);
		} else {
			qDebug("Gui::TBase::resizeMainWindow: resize failed. Panel size now %d x %d. Wanted size %d x %d",
				   panel->size().width(), panel->size().height(),
				   panel_size.width(), panel_size.height());
		}
	}

}

// Slot called when media settings reset or loaded
void TBase::onMediaSettingsChanged() {
	qDebug("Gui::TBase::onMediaSettingsChanged");

	emit mediaSettingsChanged(&core->mset);

	updateVideoEqualizer();
	updateAudioEqualizer();
}

void TBase::onDragPositionChanged(double t) {

    QString s = tr("Jump to %1").arg(Helper::formatTime(t));
	statusBar()->showMessage(s, 1000);

	if (pref->fullscreen) {
		core->displayTextOnOSD(s);
	}
}

void TBase::setStayOnTop(bool b) {
	qDebug("Gui::TBase::setStayOnTop: %d", b);

	bool stay_on_top = windowFlags() & Qt::WindowStaysOnTopHint;
	if (b == stay_on_top) {
		// identical do nothing
        qDebug("Gui::TBase::setStayOnTop: WindowStaysOnTopHint flag already up2date");
		return;
	}

	ignore_show_hide_events = true;
	bool visible = isVisible();
	QPoint old_pos = pos();

	if (b) {
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
	} else {
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
	}

	move(old_pos);
	if (visible) {
		show();
	}
	ignore_show_hide_events = false;
}

void TBase::changeStayOnTop(int stay_on_top) {
    qDebug("Gui::TBase::changeStayOnTop: %d", stay_on_top);

    switch (stay_on_top) {
        case TPreferences::AlwaysOnTop : setStayOnTop(true); break;
        case TPreferences::NeverOnTop  : setStayOnTop(false); break;
        case TPreferences::WhilePlayingOnTop : setStayOnTop((core->state() == STATE_PLAYING)); break;
	}

	pref->stay_on_top = (Settings::TPreferences::TOnTop) stay_on_top;
	emit stayOnTopChanged(stay_on_top);
}

void TBase::checkStayOnTop(TCoreState state) {
	//qDebug("Gui::TBase::checkStayOnTop");

	if (!pref->fullscreen
		&& (pref->stay_on_top == Settings::TPreferences::WhilePlayingOnTop)) {
		setStayOnTop((state == STATE_PLAYING));
	}
}

void TBase::toggleStayOnTop() {

    if (pref->stay_on_top == Settings::TPreferences::NeverOnTop)
        changeStayOnTop(Settings::TPreferences::AlwaysOnTop);
    else
        changeStayOnTop(Settings::TPreferences::NeverOnTop);
}

void TBase::onPlayerFinishedWithError(int exit_code) {
	qDebug("Gui::TBase::onPlayerFinishedWithError: %d", exit_code);

	QString msg = Proc::TErrorMsg::message(exit_code) + " (" + core->mdat.filename + ")";
	displayMessage(msg, 0);

	static bool busy = false;
	if (pref->report_player_crashes && !busy) {
		busy = true;
		QMessageBox::warning(this, tr("%1 process error").arg(pref->playerName()),
							 msg + " \n" + tr("See menu Window -> View log for additional details."),
							 QMessageBox::Ok);
		busy = false;
	}
}

void TBase::onPlayerError(QProcess::ProcessError e) {
	qDebug("Gui::TBase::onPlayerError: %d", e);
	onPlayerFinishedWithError(Proc::TErrorMsg::processErrorToErrorID(e));
}


#ifdef FIND_SUBTITLES
void TBase::showFindSubtitlesDialog() {
	qDebug("Gui::TBase::showFindSubtitlesDialog");

	if (!find_subs_dialog) {
		find_subs_dialog = new FindSubtitlesWindow(this, Qt::Window | Qt::WindowMinMaxButtonsHint);
		find_subs_dialog->setSettings(Settings::pref);
		find_subs_dialog->setWindowIcon(windowIcon());
#if DOWNLOAD_SUBS
		connect(find_subs_dialog, SIGNAL(subtitleDownloaded(const QString &)),
				core, SLOT(loadSub(const QString &)));
#endif
	}

	find_subs_dialog->show();
	find_subs_dialog->setMovie(core->mdat.filename);
}

void TBase::openUploadSubtitlesPage() {	
	QDesktopServices::openUrl(QUrl("http://www.opensubtitles.org/upload"));
}
#endif

#if defined(Q_OS_WIN) && defined(DISABLE_SCREENSAVER)
bool TBase::winEvent (MSG* m, long* result) {
	//qDebug("Gui::TBase::winEvent");

	if (m->message == WM_SYSCOMMAND) {
		if (((m->wParam & 0xFFF0) == SC_SCREENSAVE)
            || ((m->wParam & 0xFFF0) == SC_MONITORPOWER)) {
			qDebug("Gui::TBase::winEvent: received SC_SCREENSAVE or SC_MONITORPOWER");
			qDebug("Gui::TBase::winEvent: playing: %d", core->state() == STATE_PLAYING);
			qDebug("Gui::TBase::winEvent: video: %d", !core->mdat.noVideo());
			
            if (core->state() == STATE_PLAYING && core->mdat.hasVideo()) {
				qDebug("Gui::TBase::winEvent: not allowing screensaver");
				(*result) = 0;
				return true;
			}

			qDebug("Gui::TBase::winEvent: allowing screensaver");
			return false;
		}
	}
	return false;
}
#endif

} // namespace Gui

#include "moc_base.cpp"
