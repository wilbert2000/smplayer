#ifndef GUI_ACTION_PLAYMENU_H
#define GUI_ACTION_PLAYMENU_H

#include "gui/action/menu.h"
#include "corestate.h"


class TCore;

namespace Gui {

class TPlaylist;

namespace Action {

class TAction;

class TMenuSeek: public TMenu {
    Q_OBJECT
public:
    explicit TMenuSeek(QWidget* parent,
                       QWidget* mainwindow,
                       const QString& name,
                       const QString& text,
                       const QString& icon,
                       const QString& sign);

public slots:
    void peerTriggered(QAction* action);

protected:
    TAction* frameAct;
    TAction* seek1Act;
    TAction* seek2Act;
    TAction* seek3Act;
    TAction* plAct;

    virtual void enableActions(bool stopped, bool, bool);

protected slots:
    void setJumpTexts();

private:
    QWidget* main_window;
    QString seek_sign;
    QString timeForJumps(int secs) const;

private slots:
    void setDefaultActionSlot(QAction* act);
};


class TMenuPlay : public TMenu {
	Q_OBJECT
public:
	explicit TMenuPlay(QWidget* parent, TCore* c, Gui::TPlaylist* plist);

protected:
	virtual void enableActions(bool stopped, bool, bool);

private:
	TCore* core;
	Gui::TPlaylist* playlist;

	TAction* playAct;
	TAction* playOrPauseAct;
	QIcon pauseIcon;
	QIcon playIcon;
	TAction* pauseAct;
	TAction* stopAct;
    TAction* seekToAct;

private slots:
	void onStateChanged(TCoreState state);
}; // class TMenuPlay

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_PLAYMENU_H
