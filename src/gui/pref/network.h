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

#ifndef PREF_NETWORK_H
#define PREF_NETWORK_H

#include "ui_network.h"
#include "settings/preferences.h"
#include "gui/pref/widget.h"
#include "config.h"


namespace Gui { namespace Pref {

class TNetwork : public TWidget, public Ui::TNetwork
{
	Q_OBJECT

public:
	TNetwork(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~TNetwork();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Settings::TPreferences* pref);

	// Apply changes
	void getData(Settings::TPreferences* pref);

protected:
	virtual void createHelp();

	void setProxyType(int type);
	int proxyType();

protected:
	virtual void retranslateStrings();
};

}} // namespace Gui::Pref

#endif // PREF_NETWORK_H
