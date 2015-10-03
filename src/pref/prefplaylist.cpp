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

#include "pref/prefplaylist.h"
#include "settings/preferences.h"
#include "images.h"

namespace Pref {

TPrefPlaylist::TPrefPlaylist(QWidget * parent, Qt::WindowFlags f)
	: TWidget(parent, f )
{
	setupUi(this);
	retranslateStrings();
}

TPrefPlaylist::~TPrefPlaylist()
{
}

QString TPrefPlaylist::sectionName() {
	return tr("Playlist");
}

QPixmap TPrefPlaylist::sectionIcon() {
    return Images::icon("playlist", 22);
}

void TPrefPlaylist::retranslateStrings() {
	retranslateUi(this);

	int index = media_to_add_combo->currentIndex();
	media_to_add_combo->clear();
	media_to_add_combo->addItem(tr("None"), Settings::TPreferences::NoFiles);
	media_to_add_combo->addItem(tr("Video files"), Settings::TPreferences::VideoFiles);
	media_to_add_combo->addItem(tr("Audio files"), Settings::TPreferences::AudioFiles);
	media_to_add_combo->addItem(tr("Video and audio files"), Settings::TPreferences::MultimediaFiles);
	media_to_add_combo->addItem(tr("Consecutive files"), Settings::TPreferences::ConsecutiveFiles);
	media_to_add_combo->setCurrentIndex(index);

	createHelp();
}

void TPrefPlaylist::setData(Settings::TPreferences * pref) {
	setAutoAddFilesToPlaylist( pref->auto_add_to_playlist );
	setMediaToAdd( pref->media_to_add_to_playlist );
}

void TPrefPlaylist::getData(Settings::TPreferences * pref) {
	requires_restart = false;

	pref->auto_add_to_playlist = autoAddFilesToPlaylist();
	pref->media_to_add_to_playlist = (Settings::TPreferences::AutoAddToPlaylistFilter) mediaToAdd();
}

void TPrefPlaylist::setAutoAddFilesToPlaylist(bool b) {
	auto_add_to_playlist_check->setChecked(b);
}

bool TPrefPlaylist::autoAddFilesToPlaylist() {
	return auto_add_to_playlist_check->isChecked();
}

void TPrefPlaylist::setMediaToAdd(int type) {
	int i = media_to_add_combo->findData(type);
	if (i < 0) i = 0;
	media_to_add_combo->setCurrentIndex(i);
}

int TPrefPlaylist::mediaToAdd() {
	return media_to_add_combo->itemData( media_to_add_combo->currentIndex() ).toInt();
}

void TPrefPlaylist::setDirectoryRecursion(bool b) {
	recursive_check->setChecked(b);
}

bool TPrefPlaylist::directoryRecursion() {
	return recursive_check->isChecked();
}

void TPrefPlaylist::setAutoGetInfo(bool b) {
	getinfo_check->setChecked(b);
}

bool TPrefPlaylist::autoGetInfo() {
	return getinfo_check->isChecked();
}

void TPrefPlaylist::setSavePlaylistOnExit(bool b) {
	autosave_on_exit_check->setChecked(b);
}

bool TPrefPlaylist::savePlaylistOnExit() {
	return autosave_on_exit_check->isChecked();
}

void TPrefPlaylist::setPlayFilesFromStart(bool b) {
	play_from_start_check->setChecked(b);
}

bool TPrefPlaylist::playFilesFromStart() {
	return play_from_start_check->isChecked();
}

void TPrefPlaylist::createHelp() {
	clearHelp();

	setWhatsThis(auto_add_to_playlist_check, tr("Automatically add files to playlist"),
		tr("If this option is enabled, every time a file is opened, SMPlayer "
           "will first clear the playlist and then add the file to it. In "
           "case of DVDs, CDs and VCDs, all titles in the disc will be added "
           "to the playlist.") );

	setWhatsThis(media_to_add_combo, tr("Add files from folder"),
		tr("This option allows to add files automatically to the playlist:") +"<br>"+
		tr("<b>None</b>: no files will be added") +"<br>"+
		tr("<b>Video files</b>: all video files found in the folder will be added") +"<br>"+
		tr("<b>Audio files</b>: all audio files found in the folder will be added") +"<br>"+
		tr("<b>Video and audio files</b>: all video and audio files found in the folder will be added") +"<br>"+
		tr("<b>Consecutive files</b>: consecutive files (like video_1.avi, video_2.avi) will be added") );

	setWhatsThis(play_from_start_check, tr("Play files from start"),
		tr("If this option is enabled, all files from the playlist will "
           "start to play from the beginning instead of resuming from a "
           "previous playback.") );

	setWhatsThis(recursive_check, tr("Add files in directories recursively"),
		tr("Check this option if you want that adding a directory will also "
        "add the files in subdirectories recursively. Otherwise only the "
        "files in the selected directory will be added."));

	setWhatsThis(getinfo_check, tr("Get info automatically about files added"), 
		tr("Check this option to inquire the files to be added to the playlist "
        "for some info. That allows to show the title name (if available) and "
        "length of the files. Otherwise this info won't be available until "
        "the file is actually played. Beware: this option can be slow, "
        "specially if you add many files."));

	setWhatsThis(autosave_on_exit_check, tr("Save copy of playlist on exit"), 
		tr("If this option is checked, a copy of the playlist will be saved "
           "in the smplayer configuration when smplayer is closed, and it will "
           "reloaded automatically when smplayer is run again."));
}

} // namespace Pref

#include "moc_prefplaylist.cpp"