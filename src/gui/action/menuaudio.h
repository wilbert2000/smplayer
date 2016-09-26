#ifndef GUI_ACTION_AUDIOMENU_H
#define GUI_ACTION_AUDIOMENU_H

#include "gui/action/menu.h"


namespace Gui {

class TMainWindow;
class TAudioEqualizer;

namespace Action {

class TAction;


class TMenuAudio : public TMenu {
public:
    explicit TMenuAudio(TMainWindow* mw, TAudioEqualizer* audioEqualizer);

protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
    TAction* muteAct;
    TAction* decVolumeAct;
    TAction* incVolumeAct;

    TAction* decAudioDelayAct;
    TAction* incAudioDelayAct;
    TAction* audioDelayAct;

    TAction* audioEqualizerAct;
    TAction* resetAudioEqualizerAct;

    QMenu* audioFilterMenu;
    TAction* volnormAct;
    TAction* extrastereoAct;
    TAction* karaokeAct;

    TAction* loadAudioAct;
    TAction* unloadAudioAct;
}; // class TMenuAudio

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_AUDIOMENU_H
