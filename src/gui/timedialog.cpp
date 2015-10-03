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

#include "gui/timedialog.h"

namespace Gui {

TTimeDialog::TTimeDialog( QWidget* parent, Qt::WindowFlags f )
	: QDialog(parent, f)
{
	setupUi(this);

	time_edit->setDisplayFormat("H:mm:ss");
}

TTimeDialog::~TTimeDialog() {
}

void TTimeDialog::setTime(int seconds) {
	QTime t(0,0);
	time_edit->setTime(t.addSecs(seconds));
}

int TTimeDialog::time() {
	QTime t(0,0);
	return t.secsTo(time_edit->time());
}

void TTimeDialog::setMaximumTime( int seconds ) {
	QTime t(0,0);
	time_edit->setMaximumTime(t.addSecs(seconds));
}

int TTimeDialog::maximumTime() {
	QTime t(0,0);
	return t.secsTo(time_edit->maximumTime());
}

void TTimeDialog::setLabel(const QString & label) {
	time_label->setText(label);
}

QString TTimeDialog::label() {
	return time_label->text();
}

} // namespace Gui

#include "moc_timedialog.cpp"