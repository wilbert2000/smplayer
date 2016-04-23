#ifndef GUI_ACTION_MENUAUDIOTRACKS_H
#define GUI_ACTION_MENUAUDIOTRACKS_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

class TMenuAudioTracks : public TMenu {
	Q_OBJECT
public:
    explicit TMenuAudioTracks(TBase* mw, TCore* c);
protected:
    virtual void enableActions();
private:
	TCore* core;
	TAction* nextAudioTrackAct;
	TActionGroup* audioTrackGroup;
private slots:
	void updateAudioTracks();
}; // class TMenuAudioTracks

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUAUDIOTRACKS_H
