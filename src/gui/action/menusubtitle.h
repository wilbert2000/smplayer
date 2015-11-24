#ifndef GUI_SUBTITLEMENU_H
#define GUI_SUBTITLEMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;
class TActionGroup;

class TMenuSubFPS : public TMenu {
public:
	explicit TMenuSubFPS(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void enableActions(bool stopped, bool, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	friend class TMenuSubtitle;
};

class TMenuSubtitle : public TMenu {
	Q_OBJECT
public:
	TMenuSubtitle(QWidget* parent, TCore* c);
	TAction* useForcedSubsOnlyAct;
	TAction* useCustomSubStyleAct;

protected:
	virtual void enableActions(bool stopped, bool, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	TCore* core;

	TAction* decSubPosAct;
	TAction* incSubPosAct;
	TAction* decSubScaleAct;
	TAction* incSubScaleAct;

	TAction* decSubDelayAct;
	TAction* incSubDelayAct;
	TAction* subDelayAct;

	TAction* incSubStepAct;
	TAction* decSubStepAct;

#ifdef MPV_SUPPORT
	TAction* seekNextSubAct;
	TAction* seekPrevSubAct;
#endif

	TAction* nextSubtitleAct;
	TMenu* subtitleTrackMenu;
	TActionGroup* subtitleTrackGroup;
#ifdef MPV_SUPPORT
	TMenu* secondarySubtitleTrackMenu;
	TActionGroup* secondarySubtitleTrackGroup;
#endif

	TAction* loadSubsAct;
	TAction* unloadSubsAct;
	TMenuSubFPS* subFPSMenu;

#ifdef FIND_SUBTITLES
	TAction* showFindSubtitlesDialogAct;
	TAction* openUploadSubtitlesPageAct;
#endif

private slots:
	void updateSubtitles();
};

} // namespace Gui

#endif // GUI_SUBTITLEMENU_H