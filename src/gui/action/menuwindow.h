#ifndef GUI_ACTION_MENUWINDOW_H
#define GUI_ACTION_MENUWINDOW_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {
namespace Action {

class TAction;

class TMenuWindow : public TMenu {
public:
	TMenuWindow(QWidget* parent,
				 TCore* core,
				 QMenu* toolBarMenu,
				 QWidget* playlist,
				 QWidget* logWindow);
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENUWINDOW_H