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

#include "titletracks.h"
#include <QApplication>
#include "helper.h"

namespace Maps {

TTitleData::TTitleData() :
	TData(),
	chapters(),
	name(),
	duration(-1),
	vts(-1),
	isTrack(false) {
}

QString TTitleData::getDisplayName(bool add_duration) const {

	QString dname = QString::number(ID);

	if (name.isEmpty()) {
		if (isTrack) {
			dname = qApp->translate("Gui::TPlaylist", "Track %1").arg(dname);
		} else {
			dname = qApp->translate("Gui::TPlaylist", "Title %1").arg(dname);
		}
	} else {
		dname += " " + name;
	}

	if (add_duration && duration >= 0) {
		dname += " (" + Helper::formatTime(qRound(duration)) + ")";
	}

	return dname;
}


TTitleTracks::TTitleTracks() : selectedVTS(-1), vtsCount(0) {
}

TTitleTracks::~TTitleTracks() {
}

void TTitleTracks::addName(int ID, const QString &name) {
	TTitleData& title = (*this)[ID];
	title.setID(ID);
	title.setName(name);
}

void TTitleTracks::addDuration(int ID, double duration, bool is_track) {
	TTitleData& title = (*this)[ID];
	title.setID(ID);
	title.setDuration(duration);
	title.setType(is_track);
}

void TTitleTracks::addTrack(int ID, const QString &name, double duration) {
	(*this)[ID].setTrack(ID, name, duration);
}

void TTitleTracks::setVTSTitle(int title) {
	(*this)[title].setVTS(selectedVTS);
	selectedID = title;
}

void TTitleTracks::list() const {

	qDebug("Maps::TTitleTracks::list: VTS count: %d", vtsCount);
	qDebug("Maps::TTitleTracks::list: selected VTS: %d", selectedVTS);
	qDebug("Maps::TTitleTracks::list: selected title ID: %d", selectedID);
	TTitleTrackIterator i(*this);
	while (i.hasNext()) {
		i.next();
		TTitleData d = i.value();
		qDebug("Maps::TTitleTracks::list: ID: %d name: '%s' duration %f chapters: %d",
			   d.getID(), d.getName().toUtf8().constData(), d.getDuration(),
			   d.chapters.count());
	}
}

} // namespace Maps
