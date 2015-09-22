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

#include "baseplus.h"

#include <QMenu>
#include <QCloseEvent>
#include <QApplication>
#include <QDesktopWidget>

#include "config.h"
#include "myaction.h"
#include "global.h"
#include "images.h"
#include "playlist.h"

#ifdef Q_OS_WIN
#include "favorites.h"
#else
#include "tvlist.h"
#endif

#include "widgetactions.h"

#if DOCK_PLAYLIST
#include <QDockWidget>
#include "playlistdock.h"
#include "desktopinfo.h"

#define PLAYLIST_ON_SIDES 1
#endif

using namespace Global;

namespace Gui {

TBasePlus::TBasePlus( QWidget * parent, Qt::WindowFlags flags)
	: TBase( parent, flags )
{
	// Initialize variables
	mainwindow_visible = true;
	//infowindow_visible = false;
	trayicon_playlist_was_visible = false;
	widgets_size = 0;
#if DOCK_PLAYLIST
	fullscreen_playlist_was_visible = false;
	fullscreen_playlist_was_floating = false;
	compact_playlist_was_visible = false;
	ignore_playlist_events = false;
#endif

	tray = new QSystemTrayIcon( Images::icon("logo", 22), this );

	tray->setToolTip( "SMPlayer" );
	connect( tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), 
             this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

	quitAct = new MyAction(QKeySequence("Ctrl+Q"), this, "quit");
    connect( quitAct, SIGNAL(triggered()), this, SLOT(quit()) );
	openMenu->addAction(quitAct);

	showTrayAct = new MyAction(this, "show_tray_icon" );
	showTrayAct->setCheckable(true);
	connect( showTrayAct, SIGNAL(toggled(bool)),
             tray, SLOT(setVisible(bool)) );

#ifndef Q_OS_OS2
	optionsMenu->addAction(showTrayAct);
#else
	trayAvailable();
	connect( optionsMenu, SIGNAL(aboutToShow()),
             this, SLOT(trayAvailable()) );
#endif

	showAllAct = new MyAction(this, "restore/hide");
	connect( showAllAct, SIGNAL(triggered()),
             this, SLOT(toggleShowAll()) );


	context_menu = new QMenu(this);
	context_menu->addAction(showAllAct);
	context_menu->addSeparator();
	context_menu->addAction(openFileAct);
	context_menu->addMenu(recentfiles_menu);
	context_menu->addAction(openDirectoryAct);
	context_menu->addAction(openDVDAct);
	context_menu->addAction(openURLAct);
	context_menu->addMenu(favorites);
#ifndef Q_OS_WIN
	context_menu->addMenu(tvlist);
	context_menu->addMenu(radiolist);
#endif
	context_menu->addSeparator();
	context_menu->addAction(playOrPauseAct);
	context_menu->addAction(stopAct);
	context_menu->addSeparator();
	context_menu->addAction(playPrevAct);
	context_menu->addAction(playNextAct);
	context_menu->addSeparator();
	context_menu->addAction(showPlaylistAct);
	context_menu->addAction(showPreferencesAct);
	context_menu->addSeparator();
	context_menu->addAction(quitAct);
	
	tray->setContextMenu( context_menu );

#if DOCK_PLAYLIST
	// Playlistdock
	playlistdock = new PlaylistDock(this);
	playlistdock->setObjectName("playlistdock");
	playlistdock->setFloating(false); // To avoid that the playlist is visible for a moment
	playlistdock->setWidget(playlist);
	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea | 
                                  Qt::BottomDockWidgetArea
#if PLAYLIST_ON_SIDES
                                  | Qt::LeftDockWidgetArea | 
                                  Qt::RightDockWidgetArea
#endif
                                  );
	addDockWidget(Qt::BottomDockWidgetArea, playlistdock);
	playlistdock->hide();
	playlistdock->setFloating(true); // Floating by default

	connect( playlistdock, SIGNAL(closed()), this, SLOT(playlistClosed()) );
#if USE_DOCK_TOPLEVEL_EVENT
	connect( playlistdock, SIGNAL(topLevelChanged(bool)), 
             this, SLOT(dockTopLevelChanged(bool)) );
#else
	connect( playlistdock, SIGNAL(visibilityChanged(bool)), 
             this, SLOT(dockVisibilityChanged(bool)) );
#endif // USE_DOCK_TOPLEVEL_EVENT

	connect(this, SIGNAL(openFileRequested()), this, SLOT(showAll()));

	ignore_playlist_events = false;
#endif // DOCK_PLAYLIST
}

TBasePlus::~TBasePlus() {
}

bool TBasePlus::startHidden() {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	return false;
#else
	if ( (!showTrayAct->isChecked()) || (mainwindow_visible) ) 
		return false;
	else
		return true;
#endif
}

void TBasePlus::switchToTray() {

	exitFullscreen();
	showAll(false); // Hide windows
	if (core->state() == Core::Playing)
		core->stop();

	if (pref->balloon_count > 0) {
		tray->showMessage( "SMPlayer",
			tr("SMPlayer is still running here"),
			QSystemTrayIcon::Information, 3000 );
		pref->balloon_count--;
	}
}

void TBasePlus::closeWindow() {
	qDebug("Gui::TBasePlus::closeWindow");

	if (tray->isVisible()) {
		switchToTray();
	} else {
		TBase::closeWindow();
	}
}

void TBasePlus::quit() {
	qDebug("Gui::TBasePlus::quit");

	// Bypass switch to tray
	TBase::closeWindow();
}

void TBasePlus::retranslateStrings() {
	qDebug("Gui::TBasePlus::retranslateStrings");

	TBase::retranslateStrings();

	quitAct->change( Images::icon("exit"), tr("&Quit") );
	showTrayAct->change( Images::icon("systray"), tr("S&how icon in system tray") );

	updateShowAllAct();

#if DOCK_PLAYLIST
    playlistdock->setWindowTitle( tr("Playlist") );
#endif
}

void TBasePlus::updateShowAllAct() {
	if (isVisible()) 
		showAllAct->change( tr("&Hide") );
	else
		showAllAct->change( tr("&Restore") );
}

void TBasePlus::saveConfig(const QString &group) {
	qDebug("Gui::TBasePlus::saveConfig");

	TBase::saveConfig(group);

	QSettings * set = settings;
	// Store inside group derived class
	set->beginGroup( group );
	set->beginGroup( "base_gui_plus");

	set->setValue( "show_tray_icon", showTrayAct->isChecked() );
	set->setValue( "mainwindow_visible", isVisible() );

	set->setValue( "trayicon_playlist_was_visible", trayicon_playlist_was_visible );
	set->setValue( "widgets_size", widgets_size );
#if DOCK_PLAYLIST
	set->setValue( "fullscreen_playlist_was_visible", fullscreen_playlist_was_visible );
	set->setValue( "fullscreen_playlist_was_floating", fullscreen_playlist_was_floating );
	set->setValue( "compact_playlist_was_visible", compact_playlist_was_visible );
	set->setValue( "ignore_playlist_events", ignore_playlist_events );
#endif

	set->endGroup();
	set->endGroup();
}

void TBasePlus::loadConfig(const QString &group) {
	qDebug("Gui::TBasePlus::loadConfig");

	TBase::loadConfig(group);

	QSettings * set = settings;
	// load from group derived class
	set->beginGroup( group );
	set->beginGroup( "base_gui_plus");

	bool show_tray_icon = set->value( "show_tray_icon", false).toBool();
	showTrayAct->setChecked( show_tray_icon );
	//tray->setVisible( show_tray_icon );

	mainwindow_visible = set->value("mainwindow_visible", true).toBool();

	trayicon_playlist_was_visible = set->value( "trayicon_playlist_was_visible", trayicon_playlist_was_visible ).toBool();
	widgets_size = set->value( "widgets_size", widgets_size ).toInt();
#if DOCK_PLAYLIST
	fullscreen_playlist_was_visible = set->value( "fullscreen_playlist_was_visible", fullscreen_playlist_was_visible ).toBool();
	fullscreen_playlist_was_floating = set->value( "fullscreen_playlist_was_floating", fullscreen_playlist_was_floating ).toBool();
	compact_playlist_was_visible = set->value( "compact_playlist_was_visible", compact_playlist_was_visible ).toBool();
	ignore_playlist_events = set->value( "ignore_playlist_events", ignore_playlist_events ).toBool();
#endif

	set->endGroup();
	set->endGroup();

	updateShowAllAct();
}


void TBasePlus::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
	qDebug("Gui::TBasePlus::trayIconActivated: %d", reason);

	updateShowAllAct();

	if (reason == QSystemTrayIcon::Trigger) {
		toggleShowAll();
	}
	else
	if (reason == QSystemTrayIcon::MiddleClick) {
		core->playOrPause();
	}
}

void TBasePlus::toggleShowAll() {
	// Ignore if tray is not visible
	if (tray->isVisible()) {
		showAll( !isVisible() );
	}
}

void TBasePlus::showAll() {
	if (!isVisible()) showAll(true);
}

void TBasePlus::showAll(bool b) {
	if (!b) {
		// Hide all
#if DOCK_PLAYLIST
		trayicon_playlist_was_visible = (playlistdock->isVisible() && 
                                         playlistdock->isFloating() );
		if (trayicon_playlist_was_visible)
			playlistdock->hide();

		/*
		trayicon_playlist_was_visible = playlistdock->isVisible();
		playlistdock->hide();
		*/
#else
		trayicon_playlist_was_visible = playlist->isVisible();
		playlist_pos = playlist->pos();
		playlist->hide();
#endif

		hide();

		/*
		infowindow_visible = info_window->isVisible();
		infowindow_pos = info_window->pos();
		info_window->hide();
		*/
	} else {
		// Show all
		show();

#if DOCK_PLAYLIST
		if (trayicon_playlist_was_visible) {
			playlistdock->show();
		}
#else
		if (trayicon_playlist_was_visible) {
			playlist->move(playlist_pos);
			playlist->show();
		}
#endif

		/*
		if (infowindow_visible) {
			info_window->show();
			info_window->move(infowindow_pos);
		}
		*/
	}
	updateShowAllAct();
}

void TBasePlus::resizeWindow(int w, int h) {
	qDebug("Gui::TBasePlus::resizeWindow: %d, %d", w, h);

	if (tray->isVisible() && !isVisible())
		showAll(true);

	TBase::resizeWindow(w, h );
}

void TBasePlus::updateMediaInfo() {
	qDebug("Gui::TBasePlus::updateMediaInfo");
	TBase::updateMediaInfo();

	tray->setToolTip( windowTitle() );
}

void TBasePlus::setWindowCaption(const QString & title) {
	tray->setToolTip( title );

	TBase::setWindowCaption( title );
}


// Playlist stuff
void TBasePlus::aboutToEnterFullscreen() {
	//qDebug("Gui::TBasePlus::aboutToEnterFullscreen");

	TBase::aboutToEnterFullscreen();

#if DOCK_PLAYLIST
	playlistdock->setAllowedAreas(Qt::NoDockWidgetArea);

	int playlist_screen = QApplication::desktop()->screenNumber(playlistdock);
	int mainwindow_screen = QApplication::desktop()->screenNumber(this);

	fullscreen_playlist_was_visible = playlistdock->isVisible();
	fullscreen_playlist_was_floating = playlistdock->isFloating();

	ignore_playlist_events = true;

	// Hide the playlist if it's in the same screen as the main window
	if ((playlist_screen == mainwindow_screen) /* || 
        (!fullscreen_playlist_was_floating) */ ) 
	{
		playlistdock->setFloating(true);
		playlistdock->hide();
	}
#endif
}

void TBasePlus::aboutToExitFullscreen() {
	//qDebug("Gui::TBasePlus::aboutToExitFullscreen");

	TBase::aboutToExitFullscreen();

#if DOCK_PLAYLIST
	playlistdock->setAllowedAreas(Qt::TopDockWidgetArea | 
                                  Qt::BottomDockWidgetArea
                                  #if PLAYLIST_ON_SIDES
                                  | Qt::LeftDockWidgetArea | 
                                  Qt::RightDockWidgetArea
                                  #endif
                                  );

	if (fullscreen_playlist_was_visible) {
		playlistdock->show();
	}
	playlistdock->setFloating( fullscreen_playlist_was_floating );
	ignore_playlist_events = false;
#endif

	//qDebug("Gui::TBasePlus::aboutToExitFullscreen done");
}

void TBasePlus::aboutToEnterCompactMode() {
#if DOCK_PLAYLIST
	compact_playlist_was_visible = (playlistdock->isVisible() && 
                                    !playlistdock->isFloating());
	if (compact_playlist_was_visible)
		playlistdock->hide();
#endif

    widgets_size = height() - panel->height();
	qDebug("Gui::TBasePlus::aboutToEnterCompactMode: widgets_size: %d", widgets_size);

	TBase::aboutToEnterCompactMode();

	if (pref->resize_method == Preferences::Always) {
		resize( width(), height() - widgets_size );
	}
}

void TBasePlus::aboutToExitCompactMode() {
	TBase::aboutToExitCompactMode();

	if (pref->resize_method == Preferences::Always) {
		resize( width(), height() + widgets_size );
	}

#if DOCK_PLAYLIST
	if (compact_playlist_was_visible)
		playlistdock->show();
#endif
}

#if DOCK_PLAYLIST
void TBasePlus::showPlaylist(bool b) {
	qDebug("Gui::TBasePlus::showPlaylist: %d", b);
	//qDebug("Gui::TBasePlus::showPlaylist (before): playlist visible: %d", playlistdock->isVisible());
	//qDebug("Gui::TBasePlus::showPlaylist (before): playlist position: %d, %d", playlistdock->pos().x(), playlistdock->pos().y());
	//qDebug("Gui::TBasePlus::showPlaylist (before): playlist size: %d x %d", playlistdock->size().width(), playlistdock->size().height());

	if ( !b ) {
		playlistdock->hide();
	} else {
		exitFullscreenIfNeeded();
		playlistdock->show();

		// Check if playlist is outside of the screen
		if (playlistdock->isFloating()) {
			if (!DesktopInfo::isInsideScreen(playlistdock)) {
				qWarning("TBasePlus::showPlaylist: playlist is outside of the screen");
				playlistdock->move(0,0);
			}
		}
	}
	//updateWidgets();

	//qDebug("Gui::TBasePlus::showPlaylist (after): playlist visible: %d", playlistdock->isVisible());
	//qDebug("Gui::TBasePlus::showPlaylist (after): playlist position: %d, %d", playlistdock->pos().x(), playlistdock->pos().y());
	//qDebug("Gui::TBasePlus::showPlaylist (after): playlist size: %d x %d", playlistdock->size().width(), playlistdock->size().height());
}

void TBasePlus::playlistClosed() {
	showPlaylistAct->setChecked(false);
}

#if !USE_DOCK_TOPLEVEL_EVENT
void TBasePlus::dockVisibilityChanged(bool visible) {
	qDebug("Gui::TBasePlus::dockVisibilityChanged: %d", visible);

	if (!playlistdock->isFloating()) {
		if (!visible) shrinkWindow(); else stretchWindow();
	}
}

#else

void TBasePlus::dockTopLevelChanged(bool floating) {
	qDebug("Gui::TBasePlus::dockTopLevelChanged: %d", floating);

	if (floating) shrinkWindow(); else stretchWindow();
}
#endif

void TBasePlus::stretchWindow() {
	qDebug("Gui::TBasePlus::stretchWindow");
	if ((ignore_playlist_events) || (pref->resize_method!=Preferences::Always)) return;

	qDebug("Gui::TBasePlus::stretchWindow: dockWidgetArea: %d", (int) dockWidgetArea(playlistdock) );

	if ( (dockWidgetArea(playlistdock) == Qt::TopDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::BottomDockWidgetArea) )
	{
		int new_height = height() + playlistdock->height();

		//if (new_height > DesktopInfo::desktop_size(this).height()) 
		//	new_height = DesktopInfo::desktop_size(this).height() - 20;

		qDebug("Gui::TBasePlus::stretchWindow: stretching: new height: %d", new_height);
		resize( width(), new_height );

		//resizeWindow(core->mset.win_width, core->mset.win_height);
	}

	else

	if ( (dockWidgetArea(playlistdock) == Qt::LeftDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::RightDockWidgetArea) )
	{
		int new_width = width() + playlistdock->width();

		qDebug("Gui::TBasePlus::stretchWindow: stretching: new width: %d", new_width);
		resize( new_width, height() );
	}
}

void TBasePlus::shrinkWindow() {
	qDebug("Gui::TBasePlus::shrinkWindow");
	if ((ignore_playlist_events) || (pref->resize_method!=Preferences::Always)) return;

	qDebug("Gui::TBasePlus::shrinkWindow: dockWidgetArea: %d", (int) dockWidgetArea(playlistdock) );

	if ( (dockWidgetArea(playlistdock) == Qt::TopDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::BottomDockWidgetArea) )
	{
		int new_height = height() - playlistdock->height();
		qDebug("Gui::TBasePlus::shrinkWindow: shrinking: new height: %d", new_height);
		resize( width(), new_height );

		//resizeWindow(core->mset.win_width, core->mset.win_height);
	}

	else

	if ( (dockWidgetArea(playlistdock) == Qt::LeftDockWidgetArea) ||
         (dockWidgetArea(playlistdock) == Qt::RightDockWidgetArea) )
	{
		int new_width = width() - playlistdock->width();

		qDebug("Gui::TBasePlus::shrinkWindow: shrinking: new width: %d", new_width);
		resize( new_width, height() );
	}
}

#endif

// Convenience functions intended for other GUI's
TimeSliderAction * TBasePlus::createTimeSliderAction(QWidget * parent) {
	TimeSliderAction * timeslider_action = new TimeSliderAction( parent );
	timeslider_action->setObjectName("timeslider_action");

	connect( timeslider_action, SIGNAL( posChanged(int) ), 
             core, SLOT(goToPosition(int)) );
	connect( core, SIGNAL(positionChanged(int)), 
             timeslider_action, SLOT(setPos(int)) );

	connect( timeslider_action, SIGNAL( draggingPos(int) ),
             this, SLOT(displayGotoTime(int)) );
#if ENABLE_DELAYED_DRAGGING
	timeslider_action->setDragDelay( pref->time_slider_drag_delay );

	connect( timeslider_action, SIGNAL( delayedDraggingPos(int) ), 
             this, SLOT(goToPosOnDragging(int)) );
#else
	connect( timeslider_action, SIGNAL( draggingPos(int) ), 
             this, SLOT(goToPosOnDragging(int)) );
#endif

	connect(timeslider_action, SIGNAL(wheelUp(Preferences::WheelFunction)),
			core, SLOT(wheelUp(Preferences::WheelFunction)));
	connect(timeslider_action, SIGNAL(wheelDown(Preferences::WheelFunction)),
			core, SLOT(wheelDown(Preferences::WheelFunction)));

	return timeslider_action;
}

VolumeSliderAction * TBasePlus::createVolumeSliderAction(QWidget * parent) {
	VolumeSliderAction * volumeslider_action = new VolumeSliderAction(parent);
	volumeslider_action->setObjectName("volumeslider_action");

	connect( volumeslider_action, SIGNAL( valueChanged(int) ), 
             core, SLOT( setVolume(int) ) );
	connect( core, SIGNAL(volumeChanged(int)),
             volumeslider_action, SLOT(setValue(int)) );

	return volumeslider_action;
}

#ifdef Q_OS_OS2
// we test if xcenter is available at all. if not disable the tray action. this is possible when xcenter is not opened or crashed
void TBasePlus::trayAvailable() {
	if (!tray->isSystemTrayAvailable()) {
			optionsMenu->removeAction(showTrayAct);
	}
	else {
		optionsMenu->addAction(showTrayAct);
	}
}
#endif

} // namespace Gui

#include "moc_baseplus.cpp"