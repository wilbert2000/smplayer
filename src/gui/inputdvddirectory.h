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

#ifndef GUI_INPUTDVDDIRECTORY_H
#define GUI_INPUTDVDDIRECTORY_H

#include "ui_inputdvddirectory.h"

namespace Gui {

class TInputDVDDirectory : public QDialog, public Ui::TInputDVDDirectory
{
    Q_OBJECT

public:
    TInputDVDDirectory(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~TInputDVDDirectory();

    void setFolder(QString folder);
    QString folder();

protected slots:
    void on_searchButton_clicked();
};

} // namespace Gui

#endif // GUI_INPUTDVDDIRECTORY_H
