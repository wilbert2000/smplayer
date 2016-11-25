#ifndef GUI_VIDEOMENU_H
#define GUI_VIDEOMENU_H

#include "gui/action/menu/menu.h"


namespace Gui {

class TPlayerWindow;
class TMainWindow;
class TVideoEqualizer;

namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuVideo : public TMenu {
	Q_OBJECT
public:
    TMenuVideo(TMainWindow* mw,
               TPlayerWindow* playerwindow,
               TVideoEqualizer* videoEqualizer);

protected:
    virtual void enableActions();

private:
	TAction* fullscreenAct;
	TAction* exitFullscreenAct;

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

	TAction* screenshotAct;
	TAction* screenshotsAct;
	TAction * capturingAct;

private slots:
	void onFullscreenChanged();
}; // class TMenuVideo

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_VIDEOMENU_H