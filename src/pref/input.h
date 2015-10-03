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

#ifndef _PREF_INPUT_H_
#define _PREF_INPUT_H_

#include "ui_input.h"
#include "settings/preferences.h"
#include "pref/widget.h"
#include <QStringList>

class TPreferences;

namespace Pref {

class TInput : public TWidget, public Ui::TInput
{
	Q_OBJECT

public:
	TInput( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	virtual ~TInput();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
	void setData(Settings::TPreferences * pref);

    // Apply changes
	void getData(Settings::TPreferences * pref);

	// Pass action's list to dialog
	/* void setActionsList(QStringList l); */

protected:
	virtual void createHelp();

	void createMouseCombos();

	void setLeftClickFunction(QString f);
	QString leftClickFunction();

	void setRightClickFunction(QString f);
	QString rightClickFunction();

	void setDoubleClickFunction(QString f);
	QString doubleClickFunction();

	void setMiddleClickFunction(QString f);
	QString middleClickFunction();

	void setXButton1ClickFunction(QString f);
	QString xButton1ClickFunction();

	void setXButton2ClickFunction(QString f);
	QString xButton2ClickFunction();

	void setWheelFunction(int function);
	int wheelFunction();

	void setWheelFunctionCycle(Settings::TPreferences::WheelFunctions flags);
	Settings::TPreferences::WheelFunctions wheelFunctionCycle();

	void setWheelFunctionSeekingReverse(bool b);
	bool wheelFunctionSeekingReverse();

protected:
	virtual void retranslateStrings();
};

} // namespace Pref

#endif // _PREF_INPUT_H_