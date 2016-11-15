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

#ifndef GUI_MAINWINDOWPLUS_H
#define GUI_MAINWINDOWPLUS_H

#include "gui/mainwindow.h"

#include <QSystemTrayIcon>
#include <QPoint>
#include "wzdebug.h"


class QMenu;
class QDockWidget;

namespace Gui {

class TMainWindowPlus : public TMainWindow {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TMainWindowPlus();
    virtual ~TMainWindowPlus();

	virtual bool startHidden();
	virtual void loadConfig();
	virtual void saveConfig();

protected:
	QSystemTrayIcon* tray;
	QMenu* context_menu;

	Action::TAction* quitAct;
	Action::TAction* showTrayAct;
	Action::TAction* showAllAct;

	// To save state
	bool mainwindow_visible;
    bool restore_playlist;

	QDockWidget* playlistdock;

    virtual void changeEvent(QEvent* event);

protected slots:
    // Reimplemented methods
    virtual void closeWindow();
    virtual void setWindowCaption(const QString& title);
    virtual void onMediaInfoChanged();
    virtual void showPlaylist(bool visible);

    // New
    virtual void trayIconActivated(QSystemTrayIcon::ActivationReason);
    virtual void toggleShowAll();
    virtual void showAll(bool b);
    virtual void showAll();
    virtual void quit();
#ifdef Q_OS_OS2
    void trayAvailable();
#endif

private:
    QTimer optimizeSizeTimer;
    double saved_size;

    void switchToTray();
	void retranslateStrings();
    void updateShowAllAct();

private slots:
    void onDockVisibilityChanged(bool visible);
    void setWinTitle();
};

} // namespace Gui

#endif // GUI_MAINWINDOWPLUS_H
