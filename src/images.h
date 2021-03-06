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

#ifndef IMAGES_H
#define IMAGES_H

#include <QString>
#include <QPixmap>
#include <QIcon>


// TODO: rename to TImages
class Images {
public:
    static void setTheme(const QString& name);

    static QPixmap icon(const QString& name, int size=-1);
    static QPixmap flippedIcon(const QString& name, int size=-1);
    static QString iconFilename(const QString& icon_name);

    static QString styleSheet();
    static QString themesDirectory();

    static bool has_rcc;

private:
    static QPixmap resize(const QPixmap& pixmap, int size = 20);
    static QPixmap flip(const QPixmap& pixmap);

    static QString current_theme;
    static QString themes_path;
    static QString last_resource_loaded;
};

#endif
