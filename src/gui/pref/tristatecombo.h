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

/*! 
	This class is to replace some QCheckBox with a combo with three possible
    values: true, false or autodetect
*/

#ifndef _PREF_TRISTATE_COMBO_H_
#define _PREF_TRISTATE_COMBO_H_

#include <QComboBox>
#include "settings/preferences.h"

using namespace Settings;

namespace Gui { namespace Pref {

class TTristateCombo : public QComboBox
{
	Q_OBJECT

public:
	TTristateCombo(QWidget * parent = 0);
	virtual ~TTristateCombo();

	void setState(TPreferences::OptionState v);
	TPreferences::OptionState state();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent( QEvent * event );

};

}} // namespace Gui::Pref

#endif // _PREF_TRISTATE_COMBO_H_
