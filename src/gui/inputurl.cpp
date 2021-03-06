/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/inputurl.h"
#include "gui/lineedit.h"
#include "images.h"
#include "config.h"


namespace Gui {

TInputURL::TInputURL(QWidget* parent)
    : QDialog(parent, TConfig::DIALOG_FLAGS) {

    setupUi(this);

    setMinimumSize(QSize(500,140));
    setMaximumSize(QSize(600,170));
    //layout()->setSizeConstraint(QLayout::SetFixedSize);

    url_icon->setPixmap(Images::icon("open_url_big", 48));
    url_edit->setFocus();

    TLineEdit* edit = new TLineEdit(this);
    url_edit->setLineEdit(edit);
}

void TInputURL::setURL(QString url) {
    url_edit->addItem(url);
}

QString TInputURL::url() {
    return url_edit->currentText().trimmed();
}

} // namespace Gui

#include "moc_inputurl.cpp"
