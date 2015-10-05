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

#include "gui/pref/widget.h"
#include <QEvent>

namespace Gui { namespace Pref {

TWidget::TWidget(QWidget * parent, Qt::WindowFlags f )
	: QWidget(parent, f)
{
	requires_restart = false;
	help_message = "";
}

TWidget::~TWidget() {
}

QString TWidget::sectionName() {
	return QString();
}

QPixmap TWidget::sectionIcon() {
	return QPixmap();
}

void TWidget::addSectionTitle(const QString & title) {
	help_message += "<h2>"+title+"</h2>";
}

void TWidget::setWhatsThis( QWidget *w, const QString & title,
                               const QString & text)
{
	w->setWhatsThis(text);
	help_message += "<b>"+title+"</b><br>"+text+"<br><br>";

	w->setToolTip( "<qt>"+ text +"</qt>" );
}

void TWidget::clearHelp() {
	help_message = "<h1>" + sectionName() + "</h1>";
}

void TWidget::createHelp() {
}

// Language change stuff
void TWidget::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

void TWidget::retranslateStrings() {
}

}} // namespace Gui::Pref
