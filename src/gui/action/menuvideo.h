#ifndef GUI_VIDEOMENU_H
#define GUI_VIDEOMENU_H

#include "gui/action/menu.h"


class TCore;
class TPlayerWindow;

namespace Gui {

class TAction;
class TActionGroup;
class TBase;
class TVideoEqualizer;

class TMenuVideo : public TMenu {
public:
	TMenuVideo(TBase* parent,
			   TCore* c,
			   TPlayerWindow* playerwindow,
			   TVideoEqualizer* videoEqualizer);
	void fullscreenChanged(bool fullscreen);

protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	TCore* core;

	TAction* fullscreenAct;
	TAction* exitFullscreenAct;

#if USE_ADAPTER
	TMenu* screenMenu;
	TActionGroup* screenGroup;
#endif

	TAction* equalizerAct;
	TAction* resetVideoEqualizerAct;

	TAction* decContrastAct;
	TAction* incContrastAct;
	TAction* decBrightnessAct;
	TAction* incBrightnessAct;
	TAction* decHueAct;
	TAction* incHueAct;
	TAction* decSaturationAct;
	TAction* incSaturationAct;
	TAction* decGammaAct;
	TAction* incGammaAct;

	TAction* stereo3DAct;
	TAction* flipAct;
	TAction* mirrorAct;

	TAction* screenshotAct;
	TAction* screenshotsAct;

#ifdef CAPTURE_STREAM
	TAction * capturingAct;
#endif
};

} // namespace Gui

#endif // GUI_VIDEOMENU_H