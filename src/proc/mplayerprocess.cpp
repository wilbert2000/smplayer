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

#include "mplayerprocess.h"

//#include <cmath>
#include <QRegExp>
#include <QStringList>
#include <QApplication>
#include <QDebug>

#include "global.h"
#include "preferences.h"
#include "mplayerversion.h"
#include "colorutils.h"
#include "subtracks.h"
#include "maps/titletracks.h"

using namespace Global;

namespace Proc {

MplayerProcess::MplayerProcess(MediaData *mdata)
	: PlayerProcess(PlayerID::MPLAYER, mdata),
	svn_version(-1) {
}

MplayerProcess::~MplayerProcess() {
}

void MplayerProcess::clearSubSources() {

	sub_source = -1;
	sub_file = false;
	sub_vob = false;
	sub_demux = false;
	sub_file_id = -1;
}

bool MplayerProcess::startPlayer() {

	clearSubSources();
	return PlayerProcess::startPlayer();
}

void MplayerProcess::getSelectedSub() {
	qDebug("MplayerProcess::getSelectedSub");

	if (md->subs.count() > 0) {
		writeToStdin("get_property sub_source");
		if (sub_file)
			writeToStdin("get_property sub_file");
		if (sub_vob)
			writeToStdin("get_property sub_vob");
		if (sub_demux)
			writeToStdin("get_property sub_demux");
	}
}

void MplayerProcess::getSelectedTracks() {
	qDebug("MplayerProcess::getSelectedTracks");

	if (md->videos.count() > 0)
		writeToStdin("get_property switch_video");
	if (md->audios.count() > 0)
		writeToStdin("get_property switch_audio");
	getSelectedSub();
}

bool MplayerProcess::parseVideoProperty(const QString &name, const QString &value) {

	if (name == "ID") {
		int id = value.toInt();
		if (md->videos.contains(id)) {
			qDebug("parseVideoProperty: found video track id %d", id);
			get_selected_video_track = true;
		} else {
			md->videos.addID(id);
			video_tracks_changed = true;
			qDebug("parseVideoProperty: added video track id %d", id);
		}
		return true;
	}

	return PlayerProcess::parseVideoProperty(name, value);
}

bool MplayerProcess::parseAudioProperty(const QString &name, const QString &value) {

	// Audio ID
	if (name == "ID") {
		int id = value.toInt();
		if (md->audios.contains(id)) {
			qDebug("parseAudioProperty: found audio track id %d", id);
			get_selected_audio_track = true;
		} else {
			md->audios.addID(id);
			audio_tracks_changed = true;
			qDebug("parseAudioProperty: added audio track id %d", id);
		}
		return true;
	}

	return PlayerProcess::parseAudioProperty(name, value);
}

bool MplayerProcess::parseSubID(const QString &type, int id) {

	// Add new id or a sub got selected

	SubData::Type sub_type;
	if (type == "FILE_SUB") {
		sub_type = SubData::File;
		sub_file = true;
		// Remember id in case there is a filename comming
		sub_file_id = id;
	} else if (type == "VOBSUB") {
		sub_type = SubData::Vob;
		sub_vob = true;
	} else {
		sub_type = SubData::Sub;
		sub_demux = true;
	}

	if (md->subs.find(sub_type, id) < 0) {
		md->subs.add(sub_type, id);
		subtitles_changed = true;
		qDebug() << "MplayerProcess::parseSubID: created subtitle id"
				 << id << "type" << type;
	} else {
		qDebug() << "MplayerProcess::parseSubID: found subtitle id"
				 << id << "type" << type;
		get_selected_subtitle = true;
	}

	return true;
}

bool MplayerProcess::parseSubTrack(const QString &type, int id, const QString &name, const QString &value) {

	SubData::Type sub_type;
	if (type == "VSID")	{
		sub_type = SubData::Vob;
		sub_vob = true;
	} else {
		sub_type = SubData::Sub;
		sub_demux = true;
	}

	if (md->subs.find(sub_type, id) < 0) {
		qDebug("MplayerProcess::parseSubTrack: adding new subtitle id %d", id);
		md->subs.add(sub_type, id);
	}

	if (name == "NAME")	md->subs.changeName(sub_type, id, value);
	else md->subs.changeLang(sub_type, id, value);
	subtitles_changed = true;

	qDebug() << "MplayerProcess::parseSubTrack: updated subtitle id" << id
			 << "type" << type << "field" << name << "to" << value;
	return true;
}

bool MplayerProcess::parseAnswer(const QString &name, const QString &value) {

	if (name == "LENGTH") {
		notifyDuration(value.toDouble());
		return true;
	}

	int i = value.toInt();

	// Video track
	if (name == "SWITCH_VIDEO") {
		if (i != md->videos.getSelectedID()) {
			qDebug("MplayerProcess::parseAnswer: selected video track id %d", i);
			md->videos.setSelectedID(i);
			emit receivedVideoTrackChanged(i);
		} else {
			qDebug("MplayerProcess::parseAnswer: video track id %d already selected", i);
		}
		return true;
	}

	// Audio track
	if (name == "SWITCH_AUDIO") {
		if (i != md->audios.getSelectedID()) {
			qDebug("MplayerProcess::parseAnswer: selected audio track id %d", i);
			md->audios.setSelectedID(i);
			emit receivedAudioTrackChanged(i);
		} else {
			qDebug("MplayerProcess::parseAnswer: audio track id %d already selected", i);
		}
		return true;
	}

	// Subtitle track
	if (name == "SUB_SOURCE") {
		qDebug("MplayerProcess::parseAnswer: subtitle source set to %d", i);
		sub_source = i;
		if (i < 0 && md->subs.selectedID() >= 0) {
			md->subs.clearSelected();
			emit receivedSubtitleTrackChanged();
		}
		return true;
	}

	if (name == "SUB_DEMUX") {
		if (sub_source == SubData::Sub) {
			qDebug("MplayerProcess::parseAnswer: selected subtitle track id %d from demuxer", i);
			md->subs.setSelected(SubData::Sub, i);
			emit receivedSubtitleTrackChanged();
		} else {
			qDebug("MplayerProcess::parseAnswer: did not select subtitles from demuxer");
		}
		return true;
	}

	if (name == "SUB_VOB") {
		if (sub_source == SubData::Vob) {
			qDebug("MplayerProcess::parseAnswer: selected VOB subtitle track id %d", i);
			md->subs.setSelected(SubData::Vob, i);
			emit receivedSubtitleTrackChanged();
		} else {
			qDebug("MplayerProcess::parseAnswer: did not select VOB subtitles");
		}
		return true;
	}

	if (name == "SUB_FILE") {
		if (sub_source == SubData::File) {
			qDebug("MplayerProcess::parseAnswer: selected subtitle track id %d from external file", i);
			md->subs.setSelected(SubData::File, i);
			emit receivedSubtitleTrackChanged();
		} else {
			qDebug("MplayerProcess::parseAnswer: did not select external subtitles");
		}
		return true;
	}

	qWarning() << "MplayerProcess::parseAnswer: unexpected answer"
			   << name << "=" << value;

	return false;
}

void MplayerProcess::clearTime() {
	qDebug("MplayerProcess::clearTime");

	// Reset start time. Assuming we get ID_START_TIME if title has one,
	// though never seen it, except for the first title at startup.
	md->start_sec = 0;
	// Start time status is not reliable, so can't set start_sec_set to false
	md->start_sec_set = true;
	md->start_sec_prop_set = false;
	notifyTime(0, "");
}

bool MplayerProcess::titleChanged(MediaData::Type type, int title) {
	qDebug("MplayerProcess::titleChanged: title %d", title);

	md->detected_type = type;
	notifyTitleTrackChanged(title);

	if (type == MediaData::TYPE_DVDNAV) {
		// Set time to 0
		clearTime();

		// Set duration
		Maps::TTitleData& t = md->titles[title];
		notifyDuration(t.getDuration());

		if (notified_player_is_running) {
			// Videos

			// Audios
			md->audios = Maps::TTracks();
			audio_tracks_changed = true;

			// Subs
			// Subs mostly get announced at the start of title, but not always.
			// By not clearing them they are at least accesible, working or not.
			// clearSubSources();
			// md->subs.clear();
			// subtitles_changed = true;
		}

		// Chapters
		md->chapters = t.chapters;
		if (notified_player_is_running) {
			emit receivedChapterInfo();
		}
	}

	return true;
}

bool MplayerProcess::parseProperty(const QString &name, const QString &value) {

	// Track changed
	if (name == "CDDA_TRACK") {
		return titleChanged(MediaData::TYPE_CDDA, value.toInt());
	}
	if (name == "VCD_TRACK") {
		return titleChanged(MediaData::TYPE_VCD, value.toInt());
	}

	// Title changed. DVDNAV uses its own reg expr
	if (name == "DVD_CURRENT_TITLE") {
		return titleChanged(MediaData::TYPE_DVD, value.toInt());
	}
	if (name == "BLURAY_CURRENT_TITLE") {
		return titleChanged(MediaData::TYPE_BLURAY, value.toInt());
	}

	// Subtitle filename
	if (name == "FILE_SUB_FILENAME") {
		if (sub_file_id >= 0) {
			qDebug() << "MplayerProcess::parseProperty: set filename sub id"
					 << sub_file_id << "to" << value;
			md->subs.changeFilename(SubData::File, sub_file_id, value);
			subtitles_changed = true;
			return true;
		}
		qWarning() << "MplayerProcess::parseProperty: unexpected subtitle filename"
				   << value;
		return false;
	}

	// DVD disc id (DVD_VOLUME_ID is not the same)
	if (name == "DVD_DISC_ID") {
		md->dvd_id = value;
		qDebug("MplayerProcess::parseProperty: DVD id set to '%s'", md->dvd_id.toUtf8().data());
		return true;
	}

	return PlayerProcess::parseProperty(name, value);
}

bool MplayerProcess::parseChapter(int id, const QString &type, const QString &value) {

	if(type == "START") {
		double time = value.toDouble()/1000;
		md->chapters.addStart(id, time);
		qDebug("MplayerProcess::parseChapter: Chapter ID %d starts at: %g", id, time);
	} else if(type == "END") {
		double time = value.toDouble()/1000;
		md->chapters.addEnd(id, time);
		qDebug("MplayerProcess::parseChapter: Chapter ID %d ends at: %g", id, time);
	} else {
		md->chapters.addName(id, value);
		qDebug("MplayerProcess::parseChapter: Chapter ID %d name: '%s'", id, value.toUtf8().data());
	}

	return true;
}

bool MplayerProcess::parseCDTrack(const QString &type, int id, const QString &length) {

	static QRegExp rx_length("(\\d+):(\\d+):(\\d+)");

	double duration = 0;
	if (rx_length.indexIn(length) >= 0) {
		duration = rx_length.cap(1).toInt() * 60;
		duration += rx_length.cap(2).toInt();
		// MSF is 1/75 of second
		duration += ((double) rx_length.cap(3).toInt())/75;
	}

	md->titles.addDuration(id, duration, true);

	qDebug() << "MplayerProcess::parseCDTrack: added" << type << "track with duration" << duration;
	return true;
}

bool MplayerProcess::parseTitle(int id, const QString &field, const QString &value) {

	// DVD/Bluray titles. Chapters handled by parseTitleChapters.
	if (field == "LENGTH") {
		md->titles.addDuration(id, value.toDouble());
	} else {
		md->titles.addAngles(id, value.toInt());
	}

	qDebug() << "MplayerProcess::parseTitle:" << field
			 << "for title" << id
			 << "set to" << value;
	return true;
}

bool MplayerProcess::parseTitleChapters(Maps::TChapters& chapters, const QString& chaps) {

	static QRegExp rx_time("(\\d\\d):(\\d\\d):(\\d\\d)(.(\\d\\d\\d))?");

	int i;
	int idx = 0;
	int from = 0;
	while ((i = chaps.indexOf(",", from)) > 0) {
		QString s = chaps.mid(from, i - from);
		if (rx_time.indexIn(s) >= 0) {
			double time = rx_time.cap(1).toInt() * 3600
						  + rx_time.cap(2).toInt() * 60
						  + rx_time.cap(3).toInt()
						  + rx_time.cap(5).toDouble()/1000;
			chapters.addStart(idx, time);
		}
		from = i + 1;
		idx++;
	}

	qDebug("MplayerProcess::parseTitleChapters: added %d chapters", chapters.count());
	return true;
}

bool MplayerProcess::parseVO(const QString &driver, int w, int h) {

	md->video_out_width = w;
	md->video_out_height = h;

	qDebug("MplayerProcess::parseVO: video out size set to %d x %d", w, h);
	emit receivedVO(driver);

	return true;
}

bool MplayerProcess::parsePause() {

	qDebug("MplayerProcess::parsePause: emit receivedPause()");
	emit receivedPause();
	return true;
}

void MplayerProcess::convertTitlesToChapters() {

	// Just for safety, don't overwrite
	if (md->chapters.count() > 0)
		return;

	int first_title_id = md->titles.firstID();

	Maps::TTitleTracks::TTitleTrackIterator i = md->titles.getIterator();
	double start = 0;
	while (i.hasNext()) {
		i.next();
		Maps::TTitleData title = i.value();
		md->chapters.addChapter(title.getID() - first_title_id, title.getName(), start);
		start += title.getDuration();
	}

	qDebug("MplayerProcess::convertTitlesToChapters: added %d chapers",
		   md->chapters.count());
}

void MplayerProcess::checkTime(double sec) {
	Q_UNUSED(sec)
}

int MplayerProcess::getFrame(double sec, const QString &line) {
	Q_UNUSED(sec)

	// Check for frame in status line
	static QRegExp rx_frame("^[AV]:.* (\\d+)\\/.\\d+");
	if (rx_frame.indexIn(line) >= 0) {
		return rx_frame.cap(1).toInt();
	}

	// Status line timestamp resolution is only 0.1, so can be off by 10%
	// return qRound(sec * fps);
	return 0;
}

void MplayerProcess::notifyChanges() {

	if (video_tracks_changed) {
		video_tracks_changed = false;
		qDebug("MplayerProcess::notifyChanges: emit videoTrackInfoChanged");
		emit receivedVideoTrackInfo();
		get_selected_video_track = true;
	}
	if (get_selected_video_track) {
		get_selected_video_track = false;
		writeToStdin("get_property switch_video");
	}
	if (audio_tracks_changed) {
		audio_tracks_changed = false;
		qDebug("MplayerProcess::notifyChanges: emit audioTrackInfoChanged");
		emit receivedAudioTrackInfo();
		get_selected_audio_track = true;
	}
	if (get_selected_audio_track) {
		get_selected_audio_track = false;
		writeToStdin("get_property switch_audio");
	}
	if (subtitles_changed) {
		subtitles_changed = false;
		qDebug("MplayerProcess::notifyChanges: emit receivedSubtitleTrackInfo");
		emit receivedSubtitleTrackInfo();
		get_selected_subtitle = true;
	}
	if (get_selected_subtitle) {
		get_selected_subtitle = false;
		getSelectedSub();
	}
}

void MplayerProcess::playingStarted() {
	qDebug("MplayerProcess::playingStarted");

	// ...
	want_pause = false;

	// Clear notifications
	video_tracks_changed = false;
	get_selected_video_track = false;
	audio_tracks_changed = false;
	get_selected_audio_track = false;
	subtitles_changed = false;
	get_selected_subtitle = false;

	// Reset the check duration timer
	check_duration_time = md->time_sec;
	if (md->detectedDisc()) {
		// Don't check disc
		check_duration_time_diff = 360000;
	} else {
		check_duration_time_diff = 1;
	}

	if (md->title_is_menu) {
		// See if the menu has a length
		writeToStdin("get_property length");
	} else if (md->duration == 0) {
		// Use duration from selected title if duration 0
		int title = md->titles.getSelectedID();
		if (title >= 0)
			notifyDuration(md->titles[title].getDuration());

		// See if the duration is known by now
		writeToStdin("get_property length");
	}

	// Get selected video, audio and subtitle tracks
	getSelectedTracks();

	// Create chapters from titles for vcd or audio CD
	if (MediaData::isCD(md->detected_type)) {
		convertTitlesToChapters();
	}

	// Get the GUI going
	PlayerProcess::playingStarted();
}

bool MplayerProcess::parseStatusLine(double time_sec, double duration, QRegExp &rx, QString &line) {

	if (PlayerProcess::parseStatusLine(time_sec, duration, rx, line))
		return true;

	if (notified_player_is_running) {
		// Normal way to go, playing, except for first frame
		notifyChanges();

		// Check for changes in duration once in a while.
		// Abs, to protect against time wrappers like TS.
		if (qAbs(time_sec - check_duration_time) > check_duration_time_diff) {
			// Ask for length
			writeToStdin("get_property length");
			// Wait another while
			check_duration_time = time_sec;
			// Just a little longer
			check_duration_time_diff *= 4;
		}
		return true;
	}

	// First and only run of state playing
	playingStarted();

	return true;
}

bool MplayerProcess::parseLine(QString &line) {

	// Status line
	static QRegExp rx_av("^[AV]: *([0-9,:.-]+)");

	// Answers to queries
	static QRegExp rx_answer("^ANS_(.+)=(.*)");

	// Video driver and resolution after filters and aspect applied
	static QRegExp rx_vo("^VO: \\[(.*)\\] \\d+x\\d+ => (\\d+)x(\\d+)");
	// Audio driver
	static QRegExp rx_ao("^AO: \\[(.*)\\]");
	// Video and audio tracks
	static QRegExp rx_video_track("^ID_VID_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");
	static QRegExp rx_audio_track("^ID_AID_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");
	static QRegExp rx_audio_track_alt("^audio stream: \\d+ format: (.*) language: (.*) aid: (\\d+)");
	// Video and audio properties
	static QRegExp rx_video_prop("^ID_VIDEO_([A-Z]+)\\s*=\\s*(.*)");
	static QRegExp rx_audio_prop("^ID_AUDIO_([A-Z]+)\\s*=\\s*(.*)");

	// Subtitles
	static QRegExp rx_sub_id("^ID_(SUBTITLE|FILE_SUB|VOBSUB)_ID=(\\d+)");
	static QRegExp rx_sub_track("^ID_(SID|VSID)_(\\d+)_(LANG|NAME)\\s*=\\s*(.*)");

	// Chapters
	static QRegExp rx_chapters("^ID_CHAPTER_(\\d+)_(START|END|NAME)=(.*)");
	static QRegExp rx_mkvchapters("\\[mkv\\] Chapter (\\d+) from");

	// CD tracks
	static QRegExp rx_cd_track("^ID_(CDDA|VCD)_TRACK_(\\d+)_MSF=(.*)");

	// DVD/BLURAY titles
	static QRegExp rx_title("^ID_(DVD|BLURAY)_TITLE_(\\d+)_(LENGTH|ANGLES)=(.*)");
	// DVD/BLURAY chapters
	static QRegExp rx_title_chapters("^CHAPTERS: (.*)");

	// DVDNAV chapters for every title
	static QRegExp rx_dvdnav_chapters("^TITLE (\\d+), CHAPTERS: (.*)");
	static QRegExp rx_dvdnav_switch_title("^DVDNAV, switched to title: (\\d+)");
	static QRegExp rx_dvdnav_title_is_menu("^DVDNAV_TITLE_IS_MENU");
	static QRegExp rx_dvdnav_title_is_movie("^DVDNAV_TITLE_IS_MOVIE");

	// Stream title and url
	static QRegExp rx_stream_title("^.*StreamTitle='(.*)';");
	static QRegExp rx_stream_title_and_url("^.*StreamTitle='(.*)';StreamUrl='(.*)';");

	static QRegExp rx_screenshot("^\\*\\*\\* screenshot '(.*)'");

	// Program switch
#if PROGRAM_SWITCH
	static QRegExp rx_program("^PROGRAM_ID=(\\d+)");
#endif

	// Catch all props
	static QRegExp rx_prop("^ID_([A-Z_]+)\\s*=\\s*(.*)");

	// Meta data
	static QRegExp rx_meta_data("^(name|"
								"title|"
								"artist|"
								"author|"
								"album|"
								"genre|"
								"track|"
								"copyright|"
								"comment|"
								"software|"
								"creation date|"
								"year):(.*)", Qt::CaseInsensitive);

	// Messages with side effects
	static QRegExp rx_cache_empty("^Cache empty.*|^Cache not filling.*");
	static QRegExp rx_create_index("^Generating Index:.*");
	static QRegExp rx_connecting("^Connecting to .*");
	static QRegExp rx_resolving("^Resolving .*");
	static QRegExp rx_fontcache("^\\[ass\\] Updating font cache|^\\[ass\\] Init");
	static QRegExp rx_forbidden("Server returned 403: Forbidden");

	// General messages to pass on
	static QRegExp rx_message("^(Playing |Cache fill:|Scanning file)");


	// Parse A: V: status line
	if (rx_av.indexIn(line) >=0) {
		return parseStatusLine(rx_av.cap(1).toDouble(), 0, rx_av, line);
	}

	if (PlayerProcess::parseLine(line))
		return true;

	// Pause
	if (line == "ID_PAUSED") {
		return parsePause();
	}

	// Answers ANS_name=value
	if (rx_answer.indexIn(line) >= 0) {
		return parseAnswer(rx_answer.cap(1).toUpper(), rx_answer.cap(2));
	}

	// VO driver and resolution after aspect and filters applied
	if (rx_vo.indexIn(line) >= 0) {
		return parseVO(rx_vo.cap(1), rx_vo.cap(2).toInt(),
					   rx_vo.cap(3).toInt());
	}

	// AO driver
	if (rx_ao.indexIn(line) >= 0) {
		qDebug("MplayerProcess::parseLine: emit receivedAO()");
		emit receivedAO(rx_ao.cap(1));
		return true;
	}

	// Video track ID, (NAME|LANG), value
	if (rx_video_track.indexIn(line) >= 0) {
		bool changed = md->videos.updateTrack(rx_video_track.cap(1).toInt(),
											  rx_video_track.cap(2),
											  rx_video_track.cap(3));
		if (changed) video_tracks_changed = true;
		return changed;
	}

	// Audio track ID, (NAME|LANG), value
	if (rx_audio_track.indexIn(line) >= 0) {
		bool changed = md->audios.updateTrack(rx_audio_track.cap(1).toInt(),
											  rx_audio_track.cap(2),
											  rx_audio_track.cap(3));
		if (changed) audio_tracks_changed = true;
		return changed;
	}

	// Audio track alt ID, lang and format
	if (rx_audio_track_alt.indexIn(line) >= 0) {
		int id = rx_audio_track_alt.cap(3).toInt();
		bool selected = md->audios.getSelectedID() == id;
		bool changed = md->audios.updateTrack(id, rx_audio_track_alt.cap(2),
											  rx_audio_track_alt.cap(1),
											  selected);
		if (changed) audio_tracks_changed = true;
		return changed;
	}

	// Subtitle ID
	if (rx_sub_id.indexIn(line) >= 0) {
		return parseSubID(rx_sub_id.cap(1), rx_sub_id.cap(2).toInt());
	}

	// Subtitle track (SID|VSID), id, (LANG|NAME) and value
	if (rx_sub_track.indexIn(line) >= 0) {
		return parseSubTrack(rx_sub_track.cap(1), rx_sub_track.cap(2).toInt(),
							 rx_sub_track.cap(3), rx_sub_track.cap(4));
	}

	// Video property ID_VIDEO_name and value
	if (rx_video_prop.indexIn(line) >= 0) {
		return parseVideoProperty(rx_video_prop.cap(1),rx_video_prop.cap(2));
	}

	// Audio property ID_AUDIO_name and value
	if (rx_audio_prop.indexIn(line) >= 0) {
		return parseAudioProperty(rx_audio_prop.cap(1), rx_audio_prop.cap(2));
	}

	// Chapters
	if (rx_chapters.indexIn(line) >= 0) {
		return parseChapter(rx_chapters.cap(1).toInt(),
							rx_chapters.cap(2),
							rx_chapters.cap(3).trimmed());
	}

	// Matroshka chapters
	if (rx_mkvchapters.indexIn(line) >= 0) {
		int c = rx_mkvchapters.cap(1).toInt();
		qDebug("MplayerProcess::parseLine: adding mkv chapter %d", c);
		md->chapters.addID(c);
		return true;
	}

	// Audio/Video CD tracks
	if (rx_cd_track.indexIn(line) >= 0 ) {
		return parseCDTrack(rx_cd_track.cap(1),
							rx_cd_track.cap(2).toInt(),
							rx_cd_track.cap(3));
	}

	// DVD/Bluray titles
	if (rx_title.indexIn(line) >= 0) {
		return parseTitle(rx_title.cap(2).toInt(),
						  rx_title.cap(3),
						  rx_title.cap(4));
	}

	// DVD/Bluray title chapters
	if (rx_title_chapters.indexIn(line) >= 0) {
		return parseTitleChapters(md->chapters, rx_title_chapters.cap(1));
	}

	// DVDNAV
	if (rx_dvdnav_chapters.indexIn(line) >= 0) {
		int title = rx_dvdnav_chapters.cap(1).toInt();
		if (md->titles.contains(title))
			return parseTitleChapters(md->titles[title].chapters,
									  rx_dvdnav_chapters.cap(2));
		qWarning("MplayerProcess::parseLine: unexpected title %d", title);
		return false;
	}
	if (rx_dvdnav_switch_title.indexIn(line) >= 0) {
		return titleChanged(MediaData::TYPE_DVDNAV,
							rx_dvdnav_switch_title.cap(1).toInt());
	}
	if (rx_dvdnav_title_is_menu.indexIn(line) >= 0) {
		qDebug("MplayerProcess::parseLine: title is menu");
		md->title_is_menu = true;
		// Menus can have a length. If the menu has no length we get:
		// 'Failed to get value of property 'length'.'
		if (notified_player_is_running)
			writeToStdin("get_property length");
		emit receivedTitleIsMenu();
		return true;
	}
	if (line == "Failed to get value of property 'length'.") {
		if (md->title_is_menu) {
			qDebug("MplayerProcess::parseLine: this menu has no length, cleaning time");
			clearTime();
			notifyDuration(0);
			return true;
		}
		return false;
	}
	if (rx_dvdnav_title_is_movie.indexIn(line) >= 0) {
		qDebug("MplayerProcess::parseLine: title is movie");
		md->title_is_menu = false;
		emit receivedTitleIsMovie();
		return true;
	}

	// Stream title
	if (rx_stream_title_and_url.indexIn(line) >= 0) {
		QString s = rx_stream_title_and_url.cap(1);
		QString url = rx_stream_title_and_url.cap(2);
		qDebug("MplayerProcess::parseLine: stream_title: '%s'", s.toUtf8().data());
		qDebug("MplayerProcess::parseLine: stream_url: '%s'", url.toUtf8().data());
		md->detected_type = MediaData::TYPE_STREAM;
		md->stream_title = s;
		md->stream_url = url;
		emit receivedStreamTitleAndUrl( s, url );
		return true;
	}

	if (rx_stream_title.indexIn(line) >= 0) {
		QString s = rx_stream_title.cap(1);
		qDebug("MplayerProcess::parseLine: stream_title: '%s'", s.toUtf8().data());
		md->detected_type = MediaData::TYPE_STREAM;
		md->stream_title = s;
		emit receivedStreamTitle( s );
		return true;
	}

	// Screenshot
	if (rx_screenshot.indexIn(line) >= 0) {
		QString shot = rx_screenshot.cap(1);
		qDebug("MplayerProcess::parseLine: screenshot: '%s'", shot.toUtf8().data());
		emit receivedScreenshot( shot );
		return true;
	}

#if PROGRAM_SWITCH
	// Program switch
	if (rx_program.indexIn(line) >= 0) {
		int ID = rx_program.cap(1).toInt();
		md->programs.addID( ID );
		qDebug("MplayerProcess::parseLine: Added program: ID: %d", ID);
		return true;
	}
#endif

	// Catch all property ID_name = value
	if (rx_prop.indexIn(line) >= 0) {
		return parseProperty(rx_prop.cap(1), rx_prop.cap(2));
	}

	// Meta data
	if (rx_meta_data.indexIn(line) >= 0) {
		return parseMetaDataProperty(rx_meta_data.cap(1),
									 rx_meta_data.cap(2));
	}

/*
	if (svn_version == -1 && (line.startsWith("MPlayer ") || line.startsWith("MPlayer2 ", Qt::CaseInsensitive))) {
		svn_version = MplayerVersion::mplayerVersion(line);
		qDebug("MplayerProcess::parseLine: MPlayer SVN: %d", svn_version);
		if (svn_version <= 0) {
			qWarning("MplayerProcess::parseLine: couldn't parse mplayer version!");
			emit failedToParseMplayerVersion(line);
			svn_version = 0;
		}
		return true;
	}
*/

	// Catch cache messages
	if (rx_cache_empty.indexIn(line) >= 0) {
		emit receivedCacheEmptyMessage(line);
		return true;
	}
	// Creating index
	if (rx_create_index.indexIn(line) >= 0) {
		emit receivedCreatingIndex(line);
		return true;
	}
	// Catch connecting message
	if (rx_connecting.indexIn(line) >= 0) {
		emit receivedConnectingToMessage(line);
		return true;
	}
	// Catch resolving message
	if (rx_resolving.indexIn(line) >= 0) {
		emit receivedResolvingMessage(line);
		return true;
	}
	if (rx_fontcache.indexIn(line) >= 0) {
		qDebug("MplayerProcess::parseLine: emit receivedUpdatingFontCache");
		emit receivedUpdatingFontCache();
		return true;
	}
	if (rx_forbidden.indexIn(line) >= 0) {
		qDebug("MplayerProcess::parseLine: 403 forbidden");
		emit receivedForbiddenText();
		return true;
	}

	// Messages to display
	if (rx_message.indexIn(line) >= 0) {
		emit receivedMessage(line);
		return true;
	}

	return false;
}


// Start of what used to be mplayeroptions.cpp

void MplayerProcess::setMedia(const QString & media, bool is_playlist) {
	if (is_playlist) arg << "-playlist";
	arg << media;
}

void MplayerProcess::setFixedOptions() {
	arg << "-noquiet" << "-slave" << "-identify";
}

void MplayerProcess::disableInput() {
	arg << "-nomouseinput";

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	arg << "-input" << "nodefault-bindings:conf=/dev/null";
#endif
}

void MplayerProcess::setOption(const QString & option_name, const QVariant & value) {
	if (option_name == "cache") {
		int cache = value.toInt();
		if (cache > 31) {
			arg << "-cache" << value.toString();
		} else {
			arg << "-nocache";
		}
	}
	else
	if (option_name == "stop-xscreensaver") {
		bool stop_ss = value.toBool();
		if (stop_ss) arg << "-stop-xscreensaver"; else arg << "-nostop-xscreensaver";
	}
	else
	if (option_name == "correct-pts") {
		bool b = value.toBool();
		if (b) arg << "-correct-pts"; else arg << "-nocorrect-pts";
	}
	else
	if (option_name == "framedrop") {
		QString o = value.toString();
		if (o.contains("vo"))  arg << "-framedrop";
		if (o.contains("decoder")) arg << "-hardframedrop";
	}
	else
	if (option_name == "osd-scale") {
		QString scale = value.toString();
		if (scale != "6")
			arg << "-subfont-osd-scale" << scale;
	}
	else
	if (option_name == "verbose") {
		arg << "-v";
	}
	else
	if (option_name == "screenshot_template") {
		// Not supported
	}
	else
	if (option_name == "enable_streaming_sites_support") {
		// Not supported
	}
	else
	if (option_name == "hwdec") {
		// Not supported
	}
	else
	if (option_name == "fontconfig") {
		bool b = value.toBool();
		if (b) arg << "-fontconfig"; else arg << "-nofontconfig";
	}
	else
	if (option_name == "keepaspect" ||
		option_name == "dr" || option_name == "double" ||
		option_name == "fs" || option_name == "slices" ||
		option_name == "flip-hebrew")
	{
		bool b = value.toBool();
		if (b) arg << "-" + option_name; else arg << "-no" + option_name;
	}
	else {
		arg << "-" + option_name;
		if (!value.isNull()) arg << value.toString();
	}
}

void MplayerProcess::addUserOption(const QString & option) {
	arg << option;
}

void MplayerProcess::addVF(const QString & filter_name, const QVariant & value) {
	QString option = value.toString();

	if (filter_name == "blur" || filter_name == "sharpen") {
		arg << "-vf-add" << "unsharp=" + option;
	}
	else
	if (filter_name == "deblock") {
		arg << "-vf-add" << "pp=" + option;
	}
	else
	if (filter_name == "dering") {
		arg << "-vf-add" << "pp=dr";
	}
	else
	if (filter_name == "postprocessing") {
		arg << "-vf-add" << "pp";
	}
	else
	if (filter_name == "lb" || filter_name == "l5") {
		arg << "-vf-add" << "pp=" + filter_name;
	}
	else
	if (filter_name == "subs_on_screenshots") {
		if (option == "ass") {
			arg << "-vf-add" << "ass";
		} else {
			arg << "-vf-add" << "expand=osd=1";
		}
	}
	else
	if (filter_name == "flip") {
		// expand + flip doesn't work well, a workaround is to add another
		// filter between them, so that's why harddup is here
		arg << "-vf-add" << "harddup,flip";
	}
	else
	if (filter_name == "expand") {
		arg << "-vf-add" << "expand=" + option + ",harddup";
		// Note: on some videos (h264 for instance) the subtitles doesn't disappear,
		// appearing the new ones on top of the old ones. It seems adding another
		// filter after expand fixes the problem. I chose harddup 'cos I think
		// it will be harmless in mplayer.
	}
	else {
		QString s = filter_name;
		QString option = value.toString();
		if (!option.isEmpty()) s += "=" + option;
		arg << "-vf-add" << s;
	}
}

void MplayerProcess::addStereo3DFilter(const QString & in, const QString & out) {
	QString filter = "stereo3d=" + in + ":" + out;
	filter += ",scale"; // In my PC it doesn't work without scale :?
	arg << "-vf-add" << filter;
}

void MplayerProcess::addAF(const QString & filter_name, const QVariant & value) {
	QString s = filter_name;
	if (!value.isNull()) s += "=" + value.toString();
	arg << "-af-add" << s;
}

void MplayerProcess::setVolume(int v) {
	writeToStdin("volume " + QString::number(v) + " 1");
}

void MplayerProcess::setOSDLevel(int level) {
	writeToStdin("pausing_keep osd " + QString::number(level));
}

void MplayerProcess::setAudio(int ID) {
	writeToStdin("switch_audio " + QString::number(ID));
}

void MplayerProcess::setVideo(int ID) {
	writeToStdin("set_property switch_video " + QString::number(ID));
}

void MplayerProcess::setSubtitle(SubData::Type type, int ID) {

	switch (type) {
		case SubData::Vob:
			writeToStdin( "sub_vob " + QString::number(ID) );
			break;
		case SubData::Sub:
			writeToStdin( "sub_demux " + QString::number(ID) );
			break;
		case SubData::File:
			writeToStdin( "sub_file " + QString::number(ID) );
			break;
		default: {
			qWarning("MplayerProcess::setSubtitle: unknown type!");
			return;
		}
	}

	md->subs.setSelected(type, ID);
	emit receivedSubtitleTrackChanged();
}

void MplayerProcess::disableSubtitles() {

	writeToStdin("sub_source -1");

	md->subs.clearSelected();
	emit receivedSubtitleTrackChanged();
}

void MplayerProcess::setSubtitlesVisibility(bool b) {
	writeToStdin(QString("sub_visibility %1").arg(b ? 1 : 0));
}

void MplayerProcess::seekPlayerTime(double secs, int mode, bool precise, bool currently_paused) {

	QString s = QString("seek %1 %2").arg(secs).arg(mode);
	if (precise) s += " 1"; else s += " -1";

	// pausing_keep does strange things with seek, so need to use pausing instead,
	// hence the leakage of currently_paused.
	if (currently_paused)
		s = "pausing " + s;
	want_pause = currently_paused;

	writeToStdin(s);
}

void MplayerProcess::mute(bool b) {
	writeToStdin("pausing_keep_force mute " + QString::number(b ? 1 : 0));
}

void MplayerProcess::setPause(bool pause) {

	want_pause = pause;
	if (pause) writeToStdin("pausing pause"); // pauses
	else writeToStdin("pause"); // pauses / unpauses
}

void MplayerProcess::frameStep() {
	writeToStdin("frame_step");
}

void MplayerProcess::frameBackStep() {
	// TODO: use seek()
	qDebug("MplayerProcess::frameBackStep: function not supported in mplayer");
}

void MplayerProcess::showOSDText(const QString & text, int duration, int level) {

	QString s = "pausing_keep_force osd_show_text \"" + text + "\" "
			+ QString::number(duration) + " " + QString::number(level);

	writeToStdin(s);
}

void MplayerProcess::showFilenameOnOSD() {
	writeToStdin("osd_show_property_text \"${filename}\" 2000 0");
}

void MplayerProcess::showTimeOnOSD() {
	writeToStdin("osd_show_property_text \"${time_pos} / ${length} (${percent_pos}%)\" 2000 0");
}

void MplayerProcess::setContrast(int value) {
	writeToStdin("pausing_keep contrast " + QString::number(value) + " 1");
}

void MplayerProcess::setBrightness(int value) {
	writeToStdin("pausing_keep brightness " + QString::number(value) + " 1");
}

void MplayerProcess::setHue(int value) {
	writeToStdin("pausing_keep hue " + QString::number(value) + " 1");
}

void MplayerProcess::setSaturation(int value) {
	writeToStdin("pausing_keep saturation " + QString::number(value) + " 1");
}

void MplayerProcess::setGamma(int value) {
	writeToStdin("pausing_keep gamma " + QString::number(value) + " 1");
}

void MplayerProcess::setChapter(int ID) {
	writeToStdin("seek_chapter " + QString::number(ID) +" 1");
}

void MplayerProcess::setExternalSubtitleFile(const QString & filename) {

	// Load it
	writeToStdin("sub_load \""+ filename +"\"");
	// Select files as sub source
	writeToStdin("sub_source 0");
}

void MplayerProcess::setSubPos(int pos) {
	writeToStdin("sub_pos " + QString::number(pos) + " 1");
}

void MplayerProcess::setSubScale(double value) {
	writeToStdin("sub_scale " + QString::number(value) + " 1");
}

void MplayerProcess::setSubStep(int value) {
	writeToStdin("sub_step " + QString::number(value));
}

void MplayerProcess::setSubForcedOnly(bool b) {
	writeToStdin(QString("forced_subs_only %1").arg(b ? 1 : 0));
}

void MplayerProcess::setSpeed(double value) {
	writeToStdin("speed_set " + QString::number(value));
}

void MplayerProcess::enableKaraoke(bool b) {
	if (b) writeToStdin("af_add karaoke"); else writeToStdin("af_del karaoke");
}

void MplayerProcess::enableExtrastereo(bool b) {
	if (b) writeToStdin("af_add extrastereo"); else writeToStdin("af_del extrastereo");
}

void MplayerProcess::enableVolnorm(bool b, const QString & option) {
	if (b) writeToStdin("af_add volnorm=" + option); else writeToStdin("af_del volnorm");
}

void MplayerProcess::setAudioEqualizer(const QString & values) {
	writeToStdin("af_cmdline equalizer " + values);
}

void MplayerProcess::setAudioDelay(double delay) {
	writeToStdin("pausing_keep_force audio_delay " + QString::number(delay) +" 1");
}

void MplayerProcess::setSubDelay(double delay) {
	writeToStdin("pausing_keep_force sub_delay " + QString::number(delay) +" 1");
}

void MplayerProcess::setLoop(int v) {
	writeToStdin(QString("loop %1 1").arg(v));
}

void MplayerProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
	Q_UNUSED(include_subtitles)

	if (t == Single) {
		writeToStdin("pausing_keep_force screenshot 0");
	} else {
		writeToStdin("screenshot 1");
	}
}

void MplayerProcess::setTitle(int ID) {
	writeToStdin("switch_title " + QString::number(ID));
}

void MplayerProcess::discSetMousePos(int x, int y) {
	writeToStdin(QString("set_mouse_pos %1 %2").arg(x).arg(y), false);
}

void MplayerProcess::discButtonPressed(const QString & button_name) {
	writeToStdin("dvdnav " + button_name);
}

void MplayerProcess::setAspect(double aspect) {
	writeToStdin("switch_ratio " + QString::number(aspect));
}

void MplayerProcess::setFullscreen(bool b) {
	writeToStdin(QString("vo_fullscreen %1").arg(b ? "1" : "0"));
}

#if PROGRAM_SWITCH
void MplayerProcess::setTSProgram(int ID) {
	writeToStdin("set_property switch_program " + QString::number(ID) );
	writeToStdin("get_property switch_audio");
	writeToStdin("get_property switch_video");
}
#endif

void MplayerProcess::toggleDeinterlace() {
	writeToStdin("step_property deinterlace");
}

void MplayerProcess::setOSDPos(const QPoint &) {
	// not supported
}

void MplayerProcess::setOSDScale(double) {
	// not supported
	//writeToStdin("set_property subfont-osd-scale " + QString::number(value));
}

void MplayerProcess::changeVF(const QString &, bool, const QVariant &) {
	// not supported
}

void MplayerProcess::changeStereo3DFilter(bool, const QString &, const QString &) {
	// not supported
}

void MplayerProcess::setSubStyles(const AssStyles & styles, const QString & assStylesFile) {
	if (assStylesFile.isEmpty()) {
		qWarning("MplayerProcess::setSubStyles: assStylesFile is invalid");
		return;
	}

	// Load the styles.ass file
	if (!QFile::exists(assStylesFile)) {
		// If file doesn't exist, create it
		styles.exportStyles(assStylesFile);
	}
	if (QFile::exists(assStylesFile)) {
		setOption("ass-styles", assStylesFile);
	} else {
		qWarning("MplayerProcess::setSubStyles: '%s' doesn't exist", assStylesFile.toUtf8().constData());
	}
}

} // namespace Proc

#include "moc_mplayerprocess.cpp"