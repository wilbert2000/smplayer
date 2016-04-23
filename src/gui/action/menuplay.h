#ifndef GUI_ACTION_PLAYMENU_H
#define GUI_ACTION_PLAYMENU_H

#include "gui/action/menu.h"
#include "corestate.h"


class TCore;

namespace Gui {

class TBase;
class TPlaylist;

namespace Action {

class TAction;

class TMenuSeek: public TMenu {
    Q_OBJECT
public:
    explicit TMenuSeek(QWidget* parent,
                       TBase* mainwindow,
                       const QString& name,
                       const QString& text,
                       const QString& sign);

public slots:
    void updateDefaultAction();

protected:
    TAction* frameAct;
    TAction* seek1Act;
    TAction* seek2Act;
    TAction* seek3Act;
    TAction* plAct;

    virtual void enableActions();

    int actionToInt(QAction* action) const;
    TAction* intToAction(int i) const;

protected slots:
    void setJumpTexts();

private:
    QString seek_sign;

    QString timeForJumps(int secs) const;

private slots:
    void onTriggered(QAction* action);
};


class TMenuInOut : public TMenu {
    Q_OBJECT
public:
    explicit TMenuInOut(TBase* mw, TCore* c);
protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
    virtual void onAboutToShow();
private:
    TCore* core;
    QActionGroup* group;
    TAction* repeatAct;
private slots:
    void upd();
};


class TMenuPlay : public TMenu {
	Q_OBJECT
public:
    explicit TMenuPlay(TBase* mw, TCore* c, Gui::TPlaylist* plist);

protected:
    virtual void enableActions();

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
}; // class TMenuPlay

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_PLAYMENU_H
