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

#include "player/process/mpvprocess.h"

#include <QDir>
#include <QRegExp>
#include <QStringList>
#include <QApplication>

#include "config.h"
#include "player/process/exitmsg.h"
#include "player/process/playerprocess.h"
#include "settings/preferences.h"
#include "colorutils.h"
#include "player/info/playerinfo.h"
#include "mediadata.h"


namespace Player {
namespace Process {

TMPVProcess::TMPVProcess(QObject* parent, TMediaData* mdata) :
    TPlayerProcess(parent, mdata),
    debug(logger()) {
}

TMPVProcess::~TMPVProcess() {
}

bool TMPVProcess::startPlayer() {

	received_buffering = false;

	selected_title = -1;
	received_title_not_found = false;
	title_swictched = false;
	quit_at_end_of_title = false;

    request_bit_rate_info = !md->image;

	zoom = 1;
	pan_x = 0;
	pan_y = 0;

	return TPlayerProcess::startPlayer();
}

bool TMPVProcess::parseVideoTrack(int id, QString name, bool selected) {

	// Note lang "". Track info has lang.
	if (md->videos.updateTrack(id, "", name, selected)) {
		if (notified_player_is_running)
			emit receivedVideoTracks();
		return true;
	}
	return false;
}

bool TMPVProcess::parseAudioTrack(int id,
								  const QString& lang,
								  QString name,
								  bool selected) {

	if (md->audios.updateTrack(id, lang, name, selected)) {
		if (notified_player_is_running) {
			emit receivedAudioTracks();
		}
		return true;
	}
	return false;
}

bool TMPVProcess::parseSubtitleTrack(int id,
									const QString &lang,
									QString name,
									QString type,
									bool selected) {

	if (type.startsWith("*) (")) {
		type = type.mid(4);
	}
	if (name.isEmpty() && !type.isEmpty()) {
		name = type;
	}

	SubData::Type sub_type;
	QString filename;
	if (type.contains("external", Qt::CaseInsensitive)) {
		sub_type = SubData::File;
		filename = sub_file;
	} else {
		sub_type = SubData::Sub;
	}

	SubData::Type sec_type = md->subs.selectedSecondaryType();
	int sec_ID = md->subs.selectedSecondaryID();
	bool sec_selected = sec_type != SubData::None && sec_ID >= 0;

	if (sub_type == sec_type && id == sec_ID) {
        logger()->debug("TMPVProcess::parseSubtitleTrack: found secondary"
                        " subtitle track");
		// Secondary sub, don't select the primary sub
		selected = false;
	}

	if (md->subs.update(sub_type, id, sec_type, sec_ID,
						lang, name, filename,
						selected, sec_selected)) {
		if (notified_player_is_running)
			emit receivedSubtitleTracks();
		return true;
	}

	return false;
}

bool TMPVProcess::parseProperty(const QString& name, const QString& value) {

/*
	if (name == "TRACKS_COUNT") {
		int tracks = value.toInt();
        logger()->debug("parseProperty: requesting track inf%1for %1 tracks", tracks);
		for (int n = 0; n < tracks; n++) {
			writeToStdin(QString("print_text \"TRACK_INFO_%1="
				"${track-list/%1/type} "
				"${track-list/%1/id} "
				"${track-list/%1/selected} "
				"'${track-list/%1/lang:}' "
				"'${track-list/%1/title:}'\"").arg(n));
		}
		return true;
	}
*/

	if (name == "TITLES") {
		int n_titles = value.toInt();
        logger()->debug("parseProperty: creating %1 titles", n_titles);
		for (int idx = 0; idx < n_titles; idx++) {
			md->titles.addID(idx + 1);
			writeToStdin(QString("print_text \"INFO_TITLE_LENGTH=%1 ${=disc-title-list/%1/length:-1}\"").arg(idx));
		}
		waiting_for_answers += n_titles;
		return true;
	}

	if (name == "TITLE_LENGTH") {
		static QRegExp rx_title_length("^(\\d+) (.*)");
		if (rx_title_length.indexIn(value) >= 0) {
			int idx = rx_title_length.cap(1).toInt();
			// if "" player does not know or support prop
			if (!rx_title_length.cap(2).isEmpty()) {
				double length = rx_title_length.cap(2).toDouble();
				md->titles.addDuration(idx + 1, length);
			}
		}
		waiting_for_answers--;
		return true;
	}

	if (name == "CHAPTERS") {
		int n_chapters = value.toInt();
        logger()->debug("parseProperty: requesting start and titel of %1 chapter(s)", n_chapters);
		for (int n = 0; n < n_chapters; n++) {
			writeToStdin(QString("print_text \"CHAPTER_%1=${=chapter-list/%1/time:} '${chapter-list/%1/title:}'\"").arg(n));
		}
		waiting_for_answers += n_chapters;
		return true;
	}

	if (name == "MEDIA_TITLE") {
        // TODO: set if <> filename
        if (!md->image) {
            md->title = value.simplified();
            logger()->debug("parseProperty: title set to '%1'", md->title);
        }
		return true;
	}

	return TPlayerProcess::parseProperty(name, value);
}

bool TMPVProcess::parseMetaDataList(QString list) {

	// TODO: no idea how MPV escapes a ", so for now this will
	// prob. break if the meta data contains a "
	static QRegExp rx("\\{\"key\"\\:\"([^\"]*)\",\"value\"\\:\"([^\"]*)\"\\}");

	while (rx.indexIn(list) >= 0) {
		QString key = rx.cap(1);
		QString value = rx.cap(2);
		parseMetaDataProperty(key, value);
		list = list.mid(22 + key.length() + value.length());
	}

	return true;
}

bool TMPVProcess::parseChapter(int id, double start, QString title) {

	waiting_for_answers--;
	md->chapters.addChapter(id, title, start);
    logger()->debug("parseChapter: added chapter id %1 starting at %2 with"
                    " title %3", QString::number(id), QString::number(start),
                    title);
	return true;
}

void TMPVProcess::requestChapterInfo() {
	writeToStdin("print_text \"INFO_CHAPTERS=${=chapters:}\"");
}

void TMPVProcess::fixTitle() {

	// Note: getting prop with writeToStdin("print_text XXX=${=disc-title:}");
	// valid by now.

	int title = md->disc.title;
	if (title == 0)
		title = 1;

	// Accept the requested title as the selected title, if we did not receive
	// a title not found. First and upmost this handles titles being reported
	// as VTS by DVDNAV, but it also makes it possible to sequentially play all
	// titles, needed because MPV does not support menus.
	if (!received_title_not_found) {
        logger()->debug("fixTitle: requested title %1, player reports it is"
                        " playing VTS %2", title, selected_title);
		notifyTitleTrackChanged(title);
		return;
	}

    logger()->warn("fixTitle: requested title %1 not found", title);

	// Let the playlist try the next title if a valid title was requested and
	// there is more than 1 title.
	if (title <= md->titles.count() && md->titles.count() > 1) {
		// Need to notify the requested title, otherwise the playlist will select
		// the second title instead of the title after this one.
		notifyTitleTrackChanged(title);
		// Pass eof to trigger playNext() in playlist
		received_end_of_file = true;
		// Ask player to quit
		quit(0);
		return;
	}

	// Accept defeat
	quit(TExitMsg::ERR_TITLE_NOT_FOUND);
}

void TMPVProcess::checkTime(double sec) {

	if (title_swictched && sec >= title_switch_time) {
		title_swictched = false;
        logger()->debug("checkTime: sending track changed");
		notifyTitleTrackChanged(selected_title);
	}
}

bool TMPVProcess::parseTitleSwitched(QString disc_type, int title) {

	// MPV uses dvdnav to play DVDs, but without support for menus
	if (disc_type == "dvdnav") {
		md->detected_type = TMediaData::TYPE_DVD;
	} else {
		md->detected_type = md->stringToType(disc_type);
	}

	// Due to caching it still can take a while before the previous title
	// really ends, so store the title and the time to swicth and let
	// checkTime() do the swithing when the moment arrives.
	selected_title = title;

	if (disc_type == "cdda" || disc_type == "vcd") {
		if (notified_player_is_running) {
			int chapter = title - md->titles.firstID() + md->chapters.firstID();
			title_switch_time = md->chapters[chapter].getStart();
			if (title_switch_time <= md->time_sec + 0.5) {
                logger()->debug("parseTitleSwitched: switched to track %1",
                                title);
				notifyTitleTrackChanged(title);
			} else {
				// Switch when the time comes
				title_swictched = true;
				title_switch_time -= 0.4;
                logger()->debug("parseTitleSwitched: saved track changed to %1"
                                " at %2", QString::number(title),
                                QString::number(title_switch_time));
			}
		} else {
			notifyTitleTrackChanged(title);
		}
	} else {
		// When a title ends and hits a menu MPV can go haywire on invalid
		// video time stamps. By setting quit_at_end_of_title, parseLine() will
		// release it from its suffering when the title ends by sending a quit
		// and fake an eof, so the playlist can play the next item.
		if (notified_player_is_running && !quit_at_end_of_title) {
			quit_at_end_of_title = true;
			// Set ms to wait before quitting. Cannnot rely on timestamp video,
			// because it can switch before the end of the title is reached.
			// A note on margins:
			// - Current md->time_sec can be behind
			// - Menus tend to be triggered on the last second of video
			// - Quit needs time to arrive
			quit_at_end_of_title_ms = (int) ((md->duration - md->time_sec) * 1000);
			// Quit right away if less than 400 ms to go.
			if (quit_at_end_of_title_ms <= 400) {
                logger()->debug("parseTitleSwitched: quitting at end of title");
				received_end_of_file =  true;
				quit(0);
			} else {
				// Quit when quit_at_end_of_title_ms elapsed
				quit_at_end_of_title_ms -= 400;
				quit_at_end_of_title_time.start();
                logger()->debug("parseTitleSwitched: marked title to quit in"
                                " %1 ms", quit_at_end_of_title_ms);
			}
		}
	}

	return true;
}

bool TMPVProcess::parseTitleNotFound(const QString&) {

	// Requested title means the original title. The currently selected title
    // seems still valid and is the last selected title during its search
    // through the disc.
	received_title_not_found = true;
	return true;
}

void TMPVProcess::save() {
}

void TMPVProcess::convertChaptersToTitles() {

	// Just for safety...
	if (md->titles.count() > 0) {
        logger()->warn("convertChaptersToTitles: found unexpected titles");
		return;
	}
	if (md->chapters.count() == 1) {
		// Playing a single track
		int firstChapter = md->chapters.firstID();
		md->titles.addTrack(md->titles.getSelectedID(),
							md->chapters[firstChapter].getName(),
							md->duration);
		md->chapters.setSelectedID(firstChapter);
	} else {
		// Playing all tracks
		Maps::TChapters::TChapterIterator i = md->chapters.getIterator();
		if (i.hasNext()) {
			i.next();
			Maps::TChapterData prev_chapter = i.value();
			while (i.hasNext()) {
				i.next();
				Maps::TChapterData chapter = i.value();
				double duration = chapter.getStart() - prev_chapter.getStart();
				md->titles.addTrack(prev_chapter.getID() + 1,
									prev_chapter.getName(),
									duration);
				prev_chapter = chapter;
			}
			md->titles.addTrack(prev_chapter.getID() + 1,
								prev_chapter.getName(),
								md->duration - prev_chapter.getStart());
		}
	}

    logger()->debug("convertChaptersToTitles: created %1 titles",
                    QString::number(md->titles.count()));
}

void TMPVProcess::playingStarted() {
    logger()->debug("playingStarted");

	// MPV can give negative times for TS without giving a start time.
	// Correct them by setting the start time.
	if (!md->start_sec_set && md->time_sec < 0) {
        logger()->debug("playingStarted: setting negative start time %1",
                        QString::number(md->time_sec));
		md->start_sec = md->time_sec;
		// No longer need rollover protection (though not set for MPV anyway).
		md->mpegts = false;
        notifyTime(md->time_sec);
	}

	if (TMediaData::isCD(md->detected_type)) {
		// Convert chapters to titles for CD
		convertChaptersToTitles();
	} else if (md->detectedDisc()) {
		// Workaround titles being reported as VTS
		fixTitle();
	}

	TPlayerProcess::playingStarted();
}

void TMPVProcess::requestBitrateInfo() {
	writeToStdin("print_text VIDEO_BITRATE=${=video-bitrate}");
	writeToStdin("print_text AUDIO_BITRATE=${=audio-bitrate}");
}

bool TMPVProcess::parseStatusLine(const QRegExp& rx) {
    // Parse custom status line
    // STATUS: ${=time-pos} / ${=duration:${=length:0}} P: ${=pause} B: ${=paused-for-cache} I: ${=core-idle}

    paused = rx.cap(3) == "yes";

    notifyDuration(rx.cap(2).toDouble());
    notifyTime(rx.cap(1).toDouble());

    // Any pending questions?
    if (waitForAnswers()) {
        return true;
    }

    if (!notified_player_is_running) {
        // First and only run of state playing or paused
        // Base class sets notified_player_is_running to true
        playingStarted();

        if (paused) {
            emit receivedPause();
        }
        return true;
    }

    if (paused) {
        // Don't emit signal receivedPause(): it is racy and not needed for MPV
        return true;
    }

    // Parse status flags
    bool buffering = rx.cap(4) == "yes";
    bool idle = rx.cap(5) == "yes";

    if (buffering or idle) {
        //logger()->debug("parseStatusLine: buffering");
        received_buffering = true;
        emit receivedBuffering();
        return true;
    }

    if (received_buffering) {
        received_buffering = false;
        emit receivedBufferingEnded();
    }

    if (request_bit_rate_info && md->time_sec > 11) {
        request_bit_rate_info = false;
        requestBitrateInfo();
    }

    return true;
}

bool TMPVProcess::parseLine(QString& line) {

	// Custom status line. Make sure it matches!
    static QRegExp rx_status("^STATUS: ([0-9\\.-]*) / ([0-9\\.-]+) P: (yes|no) B: (yes|no) I: (yes|no)");

	// Tracks:
	static QRegExp rx_video_track("^(.*)Video\\s+--vid=(\\d+)(.*)");
	static QRegExp rx_audio_track("^(.*)Audio\\s+--aid=(\\d+)(\\s+--alang=([a-zA-Z]+))?(.*)");
	static QRegExp rx_subtitle_track("^(.*)Subs\\s+--sid=(\\d+)(\\s+--slang=([a-zA-Z]+))?(\\s+'(.*)')?(\\s+\\((.*)\\))?");

	static QRegExp rx_ao("^AO: \\[(.*)\\]");

	static QRegExp rx_video_codec("^VIDEO_CODEC=\\s*(.*) \\[(.*)\\]");
	static QRegExp rx_video_property("^VIDEO_([A-Z]+)=\\s*(.*)");
	static QRegExp rx_audio_codec("^AUDIO_CODEC=\\s*(.*) \\[(.*)\\]");
	static QRegExp rx_audio_property("^AUDIO_([A-Z]+)=\\s*(.*)");

	static QRegExp rx_meta_data("^METADATA_LIST=(.*)");

	static QRegExp rx_chapter("^CHAPTER_(\\d+)=([0-9\\.-]+) '(.*)'");

	static QRegExp rx_title_switch("^\\[(cdda|vcd|dvd|dvdnav|br)\\] .*switched to (track|title):?\\s+(-?\\d+)",
								   Qt::CaseInsensitive);
	static QRegExp rx_title_not_found("^\\[(cdda|vcd|dvd|dvdnav|br)\\] .*(track|title) not found",
								   Qt::CaseInsensitive);

	static QRegExp rx_stream_title("icy-title: (.*)");

	static QRegExp rx_property("^INFO_([A-Z_]+)=\\s*(.*)");

	// Messages to show in statusline
	static QRegExp rx_message("^(Playing:|\\[ytdl_hook\\])");

	// Errors
	static QRegExp rx_file_open("^\\[file\\] Cannot open file '.*': (.*)");
	static QRegExp rx_failed_open("^Failed to open (.*)\\.$");
	static QRegExp rx_failed_format("^Failed to recognize file format");
	static QRegExp rx_error_http_403("HTTP error 403 ");
	static QRegExp rx_error_http_404("HTTP error 404 ");

	static QRegExp rx_verbose("^\\[(statusline|term-msg|cplayer)\\] (.*)");


	// Check to see if a DVD title needs to be terminated
	if (quit_at_end_of_title && !quit_send
		&& quit_at_end_of_title_time.elapsed() >= quit_at_end_of_title_ms) {
        logger()->debug("parseline: %1 ms elapsed, quitting title",
			   quit_at_end_of_title_ms);
		quit_at_end_of_title = false;
		received_end_of_file =  true;
		quit(0);
		return true;
	}

	// Remove sender when using verbose
	if (rx_verbose.indexIn(line) >= 0) {
		line = rx_verbose.cap(2);
	}

	// Parse custom status line
	if (rx_status.indexIn(line) >= 0) {
        return parseStatusLine(rx_status);
	}

	// Let parent have a look at it
	if (TPlayerProcess::parseLine(line))
		return true;

	if (rx_message.indexIn(line) >= 0) {
        logger()->info("parseLine: '%1'", line);
		emit receivedMessage(line);
		return true;
	}

	// Video id, codec, name and selected
	// If enabled, track info does give lang
	if (rx_video_track.indexIn(line) >= 0) {
		return parseVideoTrack(rx_video_track.cap(2).toInt(),
							   rx_video_track.cap(3).trimmed(),
                               !rx_video_track.cap(1).trimmed().isEmpty());
	}

	// Audio id, lang, codec, name and selected
	if (rx_audio_track.indexIn(line) >= 0) {
		return parseAudioTrack(rx_audio_track.cap(2).toInt(),
							   rx_audio_track.cap(4),
							   rx_audio_track.cap(5).trimmed(),
                               !rx_audio_track.cap(1).trimmed().isEmpty());
	}

	// Subtitles id, lang, name, type and selected
	if (rx_subtitle_track.indexIn(line) >= 0) {
		return parseSubtitleTrack(rx_subtitle_track.cap(2).toInt(),
								  rx_subtitle_track.cap(4),
								  rx_subtitle_track.cap(6).trimmed(),
								  rx_subtitle_track.cap(8),
                                  !rx_subtitle_track.cap(1).trimmed().isEmpty());
	}

	// AO
	if (rx_ao.indexIn(line) >= 0) {
		md->ao = rx_ao.cap(1);
        logger()->debug("parseLine: audio driver '%1'", md->ao);
		return true;
	}

	// Video codec best match.
	// Fall back to generic VIDEO_CODEC if match fails.
	if (rx_video_codec.indexIn(line) >= 0) {
		md->video_codec = rx_video_codec.cap(2);
        logger()->debug("parseLine: video_codec set to '%1'", md->video_codec);
		return true;
	}

	// Video property VIDEO_name and value
	if (rx_video_property.indexIn(line) >= 0) {
		return parseVideoProperty(rx_video_property.cap(1),
								  rx_video_property.cap(2));
	}

	// Audio codec best match
	// Fall back to generic AUDIO_CODEC if match fails.
	if (rx_audio_codec.indexIn(line) >= 0) {
		md->audio_codec = rx_audio_codec.cap(2);
        logger()->debug("parseLine: audio_codec set to '%1'", md->audio_codec);
		return true;
	}

	// Audio property AUDIO_name and value
	if (rx_audio_property.indexIn(line) >= 0) {
		return parseAudioProperty(rx_audio_property.cap(1),
								  rx_audio_property.cap(2));
	}

	// Chapter id, time and title
	if (rx_chapter.indexIn(line) >= 0) {
		return parseChapter(rx_chapter.cap(1).toInt(),
							rx_chapter.cap(2).toDouble(),
							rx_chapter.cap(3).trimmed());
	}

	// Property INFO_name and value
	if (rx_property.indexIn(line) >= 0) {
		return parseProperty(rx_property.cap(1), rx_property.cap(2));
	}

	// Meta data METADATA_name and value
	if (rx_meta_data.indexIn(line) >= 0) {
		return parseMetaDataList(rx_meta_data.cap(1));
	}

	// Switch title
	if (rx_title_switch.indexIn(line) >= 0) {
		return parseTitleSwitched(rx_title_switch.cap(1).toLower(),
								  rx_title_switch.cap(3).toInt());
	}

	// Title not found
	if (rx_title_not_found.indexIn(line) >= 0) {
		return parseTitleNotFound(rx_title_not_found.cap(1));
	}

    if (rx_stream_title.indexIn(line) >= 0) {
        md->detected_type = TMediaData::TYPE_STREAM;
		QString s = rx_stream_title.cap(1);
        md->title = s;
        logger()->debug("parseLine: title '%1'", md->title);
		emit receivedStreamTitle();
		return true;
	}

	// Errors
	if (rx_file_open.indexIn(line) >= 0) {
		logger()->debug("MVPProcess::parseLine: stored file open failed");
		exit_code_override = TExitMsg::ERR_FILE_OPEN;
		TExitMsg::setExitCodeMsg(rx_file_open.cap(1));
		return true;
	}
	if (rx_failed_open.indexIn(line) >= 0) {
		if (exit_code_override == 0 && rx_failed_open.cap(1) == md->filename) {
			logger()->debug("MVPProcess::parseLine: stored open failed");
			exit_code_override = TExitMsg::ERR_OPEN;
		} else {
			logger()->debug("MVPProcess::parseLine: skipped open failed");
		}
	}
	if (rx_failed_format.indexIn(line) >= 0) {
		logger()->debug("MVPProcess::parseLine: stored unrecognized file format");
		exit_code_override = TExitMsg::ERR_FILE_FORMAT;
	}
	if (rx_error_http_403.indexIn(line) >= 0) {
		logger()->debug("MVPProcess::parseLine: stored HTTP 403");
		exit_code_override = TExitMsg::ERR_HTTP_403;
		return true;
	}
	if (rx_error_http_404.indexIn(line) >= 0) {
		logger()->debug("MVPProcess::parseLine: stored HTTP 404");
		exit_code_override = TExitMsg::ERR_HTTP_404;
		return true;
	}

	return false;
}

// Start of what used to be mpvoptions.cpp and was pulled in with an include

void TMPVProcess::setMedia(const QString& media) {
    args << "--term-playing-msg="
		"VIDEO_ASPECT=${video-aspect}\n"
		"VIDEO_FPS=${=fps}\n"
//		"VIDEO_BITRATE=${=video-bitrate}\n"
		"VIDEO_FORMAT=${=video-format}\n"
		"VIDEO_CODEC=${=video-codec}\n"

//		"AUDIO_BITRATE=${=audio-bitrate}\n"
		"AUDIO_FORMAT=${=audio-codec-name}\n"
		"AUDIO_CODEC=${=audio-codec}\n"
		"AUDIO_RATE=${=audio-params/samplerate}\n"
		"AUDIO_NCH=${=audio-params/channel-count}\n"

		"INFO_START_TIME=${=time-start:}\n"
		"INFO_LENGTH=${=duration:${=length}}\n"
		"INFO_DEMUXER=${=demuxer}\n"

		"INFO_TITLES=${=disc-titles}\n"
		"INFO_CHAPTERS=${=chapters}\n"
		"INFO_ANGLE_EX=${angle}\n"
//		"INFO_TRACKS_COUNT=${=track-list/count}\n"

		"METADATA_LIST=${=metadata/list:}\n"
		"INFO_MEDIA_TITLE=${=media-title:}\n";

    args << "--term-status-msg=STATUS: ${=time-pos} / ${=duration:${=length:0}} P: ${=pause} B: ${=paused-for-cache} I: ${=core-idle}";

	// MPV interprets the ID in a DVD URL as index [0..#titles-1] instead of
	// [1..#titles]. Maybe one day they gonna fix it and this will break. Sigh.
	// When no title is given it plays the longest title it can find.
	// Need to change no title to 0, otherwise fixTitle() won't work.
	// CDs work as expected, don't know about bluray, but assuming it's the same.
	QString url = media;
	TDiscName disc(media);
	if (disc.valid
		&& (disc.protocol == "dvd" || disc.protocol == "dvdnav"
			|| disc.protocol == "br")) {
		if (disc.title > 0)
			disc.title--;
		url = disc.toString(true);
    } else if (md->image) {
        url = "mf://@" + temp_file_name;
    }

    args << url;

	capturing = false;
}

void TMPVProcess::setFixedOptions() {
    args << "--no-config";
    args << "--no-quiet";
    args << "--terminal";
    args << "--no-msg-color";
    args << "--input-file=/dev/stdin";
	//arg << "--no-osc";
	//arg << "--msg-level=vd=v";
}

void TMPVProcess::disableInput() {
    args << "--no-input-default-bindings";
    args << "--input-x11-keyboard=no";
    args << "--no-input-cursor";
    args << "--cursor-autohide=no";
}

bool TMPVProcess::isOptionAvailable(const QString& option) {

    Player::Info::TPlayerInfo* ir = Player::Info::TPlayerInfo::obj();
	ir->getInfo();
	return ir->optionList().contains(option);
}

void TMPVProcess::addVFIfAvailable(const QString& vf, const QString& value) {

    Player::Info::TPlayerInfo* ir = Player::Info::TPlayerInfo::obj();
    ir->getInfo();
    if (ir->vfList().contains(vf)) {
        QString s = "--vf-add=" + vf;
        if (!value.isEmpty()) {
            s += "=" + value;
        }
        args << s;
        logger()->debug("addVFIfAvailable: added video filter '%1'", s);
    } else {
        logger()->info("addVFIfAvailable: filter '%1' is not available", vf);
    }
}

void TMPVProcess::setOption(const QString& name, const QVariant& value) {

	// Options without translation
	if (name == "wid"
		|| name == "vo"
		|| name == "aid"
		|| name == "vid"
		|| name == "volume"
		|| name == "ass-styles"
		|| name == "ass-force-style"
		|| name == "ass-line-spacing"
		|| name == "embeddedfonts"
		|| name == "osd-scale-by-window"
		|| name == "osd-scale"
		|| name == "speed"
		|| name == "contrast"
		|| name == "brightness"
		|| name == "hue"
		|| name == "saturation"
		|| name == "gamma"
		|| name == "monitorpixelaspect"
		|| name == "monitoraspect"
		|| name == "mc"
		|| name == "framedrop"
		|| name == "hwdec"
		|| name == "autosync"
		|| name == "dvd-device"
		|| name == "cdrom-device"
		|| name == "demuxer"
		|| name == "shuffle"
		|| name == "frames"
        || name == "hwdec-codecs"
        || name == "pause"
        || name == "fps") {

		QString s = "--" + name;
		if (!value.isNull())
			s += "=" + value.toString();
        args << s;
	} else if (name == "aspect") {
		QString s = value.toString();
		if (!s.isEmpty()) {
			if (s == "0") {
                args << "--no-video-aspect";
			} else {
                args << "--video-aspect";
                args << s;
			}
		}
	} else if (name == "cache") {
		int cache = value.toInt();
		if (cache > 31) {
            args << "--cache=" + value.toString();
		} else {
            args << "--cache=no";
		}
	} else if (name == "ss") {
        args << "--start=" + value.toString();
	} else if (name == "endpos") {
        args << "--length=" + value.toString();
	} else if (name == "loop") {
		QString o = value.toString();
        if (o == "0") {
			o = "inf";
        }
        args << "--loop=" + o;
	} else if (name == "ass") {
        args << "--sub-ass";
	} else if (name == "noass") {
        args << "--no-sub-ass";
	} else if (name == "nosub") {
        args << "--no-sub";
	} else if (name == "sub-fuzziness") {
		QString v;
		switch (value.toInt()) {
			case 1: v = "fuzzy"; break;
			case 2: v = "all"; break;
			default: v = "exact";
		}
        args << "--sub-auto=" + v;
	} else if (name == "audiofile") {
        args << "--audio-file=" + value.toString();
	} else if (name == "delay") {
        args << "--audio-delay=" + value.toString();
	} else if (name == "subdelay") {
        args << "--sub-delay=" + value.toString();
	} else if (name == "sid") {
        args << "--sid=" + value.toString();
	} else if (name == "sub") {
		sub_file = value.toString();
        args << "--sub-file=" + sub_file;
	} else if (name == "subpos") {
        args << "--sub-pos=" + value.toString();
	} else if (name == "font") {
        args << "--osd-font=" + value.toString();
	} else if (name == "subcp") {
        args << "--sub-codepage=" + value.toString();
	} else if (name == "osdlevel") {
        args << "--osd-level=" + value.toString();
	} else if (name == "sws") {
        args << "--sws-scaler=lanczos";
	} else if (name == "channels") {
        args << "--audio-channels=" + value.toString();
	} else if (name == "sub-scale"
			   || name == "subfont-text-scale"
			   || name == "ass-font-scale") {
		QString scale = value.toString();
		if (scale != "1")
            args << "--sub-scale=" + scale;
	} else if (name == "correct-pts") {
		bool b = value.toBool();
        if (b) args << "--correct-pts"; else args << "--no-correct-pts";
	} else if (name == "idx") {
        args << "--index=default";
	} else if (name == "softvol") {
        args << "--softvol=yes";
	} else if (name == "softvol-max") {
		int v = value.toInt();
		if (v < 100)
			v = 100;
        args << "--volume-max=" + QString::number(v);
	} else if (name == "subfps") {
        args << "--sub-fps=" + value.toString();
	} else if (name == "forcedsubsonly") {
        args << "--sub-forced-only";
	} else if (name == "dvdangle") {
        args << "--dvd-angle=" + value.toString();
	} else if (name == "screenshot_template") {
        args << "--screenshot-template=" + value.toString();
	} else if (name == "screenshot_format") {
        args << "--screenshot-format=" + value.toString();
	} else if (name == "keepaspect" || name == "fs") {
		bool b = value.toBool();
        if (b) args << "--" + name; else args << "--no-" + name;
	} else if (name == "ao") {
		QString o = value.toString();
		if (o.startsWith("alsa:device=")) {
			QString device = o.mid(12);
			device = device.replace("=", ":").replace(".", ",");
			o = "alsa:device=[" + device + "]";
		}
        args << "--ao=" + o;
	} else if (name == "afm") {
		QString s = value.toString();
		if (s == "hwac3")
            args << "--ad=spdif:ac3,spdif:dts";
	} else if (name == "verbose") {
        args << "-v";
	} else if (name == "mute") {
        args << "--mute=yes";
	} else if (name == "vf-add") {
		if (!value.isNull())
            args << "--vf-add=" + value.toString();
	} else if (name == "af-add") {
		if (!value.isNull())
            args << "--af-add=" + value.toString();
	} else {
        logger()->debug("setOption: ignoring option name '%1' value '%2'",
                        name, value.toString());
	}
}

void TMPVProcess::addUserOption(const QString& option) {
    args << option;
}

void TMPVProcess::addVF(const QString& filter_name, const QVariant& value) {

	QString option = value.toString();

	if ((filter_name == "harddup") || (filter_name == "hue")) {
		// ignore
	} else if (filter_name == "eq2") {
        args << "--vf-add=eq";
	} else if (filter_name == "blur") {
		addVFIfAvailable("lavfi", "[unsharp=la=-1.5:ca=-1.5]");
	} else if (filter_name == "sharpen") {
		addVFIfAvailable("lavfi", "[unsharp=la=1.5:ca=1.5]");
	} else if (filter_name == "noise") {
		addVFIfAvailable("lavfi", "[noise=alls=9:allf=t]");
	} else if (filter_name == "deblock") {
		addVFIfAvailable("lavfi", "[pp=" + option +"]");
	} else if (filter_name == "dering") {
		addVFIfAvailable("lavfi", "[pp=dr]");
	} else if (filter_name == "phase") {
		addVFIfAvailable("lavfi", "[phase=" + option +"]");
	} else if (filter_name == "postprocessing") {
		addVFIfAvailable("lavfi", "[pp]");
	} else if (filter_name == "hqdn3d") {
		QString o;
		if (!option.isEmpty())
			o = "=" + option;
		addVFIfAvailable("lavfi", "[hqdn3d" + o +"]");
	} else if (filter_name == "yadif") {
		if (option == "1") {
            args << "--vf-add=yadif=field";
		} else {
            args << "--vf-add=yadif";
		}
	} else if (filter_name == "kerndeint") {
		addVFIfAvailable("lavfi", "[kerndeint=" + option +"]");
	} else if (filter_name == "lb" || filter_name == "l5") {
		addVFIfAvailable("lavfi", "[pp=" + filter_name +"]");
	} else if (filter_name == "subs_on_screenshots") {
		// Ignore
	} else if (filter_name == "screenshot") {
		if (!screenshot_dir.isEmpty() && isOptionAvailable("--screenshot-directory")) {
            args << "--screenshot-directory=" + QDir::toNativeSeparators(screenshot_dir);
		}
	} else if (filter_name == "rotate") {
        args << "--vf-add=rotate=" + option;
    } else {
		if (filter_name == "pp") {
			QString s;
			if (option.isEmpty()) {
				s = "[pp]";
			} else {
				s = "[pp=" + option + "]";
			}
			addVFIfAvailable("lavfi", s);
		} else if (filter_name == "extrastereo") {
            args << "--af-add=lavfi=[extrastereo]";
		} else if (filter_name == "karaoke") {
			/* Not supported anymore, ignore */
		} else {
			QString s = filter_name;
			if (!option.isEmpty())
				s += "=" + option;
            args << "--vf-add=" + s;
		}
	}
}

void TMPVProcess::addStereo3DFilter(const QString& in, const QString& out) {
    args << "--vf-add=stereo3d=" + in + ":" + out;
}

void TMPVProcess::addAF(const QString& filter_name, const QVariant& value) {
	QString option = value.toString();

	if (filter_name == "volnorm") {
		QString s = "drc";
		if (!option.isEmpty()) s += "=" + option;
        args << "--af-add=" + s;
	}
	else
	if (filter_name == "channels") {
        if (option == "2:2:0:1:0:0") args << "--af-add=channels=2:[0-1,0-0]";
		else
        if (option == "2:2:1:0:1:1") args << "--af-add=channels=2:[1-0,1-1]";
		else
        if (option == "2:2:0:1:1:0") args << "--af-add=channels=2:[0-1,1-0]";
	}
	else
	if (filter_name == "pan") {
		if (option == "1:0.5:0.5") {
            args << "--af-add=pan=1:[0.5,0.5]";
		}
	}
	else
	if (filter_name == "equalizer") {
		previous_eq = option;
        args << "--af-add=equalizer=" + option;
	}
	else {
		QString s = filter_name;
		if (!option.isEmpty()) s += "=" + option;
        args << "--af-add=" + s;
	}
}

void TMPVProcess::setVolume(int v) {
	writeToStdin("set volume " + QString::number(v));
}

void TMPVProcess::setOSDLevel(int level) {
	writeToStdin("osd " + QString::number(level));
}

void TMPVProcess::setAudio(int ID) {
	writeToStdin("set aid " + QString::number(ID));
}

void TMPVProcess::setVideo(int ID) {
	writeToStdin("set vid " + QString::number(ID));
}

void TMPVProcess::setSubtitle(SubData::Type type, int ID) {

	writeToStdin("set sid " + QString::number(ID));
	md->subs.setSelected(type, ID);
	emit receivedSubtitleTrackChanged();
}

void TMPVProcess::disableSubtitles() {

	writeToStdin("set sid no");
	md->subs.clearSelected();
	emit receivedSubtitleTrackChanged();
}

void TMPVProcess::setSecondarySubtitle(SubData::Type type, int ID) {

	md->subs.setSelectedSecondary(type, ID);
	writeToStdin("set secondary-sid " + QString::number(ID));
}

void TMPVProcess::disableSecondarySubtitles() {

	md->subs.setSelectedSecondary(SubData::None, -1);
	writeToStdin("set secondary-sid no");
}

void TMPVProcess::setSubtitlesVisibility(bool b) {
	writeToStdin(QString("set sub-visibility %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::seekPlayerTime(double secs, int mode, bool precise, bool currently_paused) {
	Q_UNUSED(currently_paused)

	QString s = "seek " + QString::number(secs) + " ";
	switch (mode) {
		case 0 : s += "relative "; break;
		case 1 : s += "absolute-percent "; break;
		case 2 : s += "absolute "; break;
	}
	if (precise) s += "exact"; else s += "keyframes";
	writeToStdin(s);
}

void TMPVProcess::mute(bool b) {
	writeToStdin(QString("set mute %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::setPause(bool b) {
	writeToStdin(QString("set pause %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::frameStep() {
	writeToStdin("frame_step");
}

void TMPVProcess::frameBackStep() {
	writeToStdin("frame_back_step");
}

void TMPVProcess::showOSDText(const QString& text, int duration, int level) {
	QString str = QString("show_text \"%1\" %2 %3").arg(text).arg(duration).arg(level);
	writeToStdin(str);
}

void TMPVProcess::showFilenameOnOSD() {
	writeToStdin("show_text \"${filename}\" 2000 0");
}

void TMPVProcess::showTimeOnOSD() {
	writeToStdin("show_text \"${time-pos} / ${length:0} (${percent-pos}%)\" 2000 0");
}

void TMPVProcess::setContrast(int value) {
	writeToStdin("set contrast " + QString::number(value));
}

void TMPVProcess::setBrightness(int value) {
	writeToStdin("set brightness " + QString::number(value));
}

void TMPVProcess::setHue(int value) {
	writeToStdin("set hue " + QString::number(value));
}

void TMPVProcess::setSaturation(int value) {
	writeToStdin("set saturation " + QString::number(value));
}

void TMPVProcess::setGamma(int value) {
	writeToStdin("set gamma " + QString::number(value));
}

void TMPVProcess::setChapter(int ID) {
	writeToStdin("set chapter " + QString::number(ID));
}

void TMPVProcess::nextChapter(int delta) {
	writeToStdin("add chapter " + QString::number(delta));
}

void TMPVProcess::setAngle(int ID) {
	writeToStdin("set angle " + QString::number(ID));
	writeToStdin("print_text INFO_ANGLE_EX=${angle}");
}

void TMPVProcess::nextAngle() {
	writeToStdin("cycle angle");
	writeToStdin("print_text INFO_ANGLE_EX=${angle}");
}

void TMPVProcess::setExternalSubtitleFile(const QString& filename) {

	writeToStdin("sub_add \""+ filename +"\"");
	// Remeber filename to add to subs when MPV is done with it
	sub_file = filename;
}

void TMPVProcess::setSubPos(int pos) {
	writeToStdin("set sub-pos " + QString::number(pos));
}

void TMPVProcess::setSubScale(double value) {
	writeToStdin("set sub-scale " + QString::number(value));
}

void TMPVProcess::setSubStep(int value) {
	writeToStdin("sub_step " + QString::number(value));
}

void TMPVProcess::seekSub(int value) {
	writeToStdin("sub-seek " + QString::number(value));
}

void TMPVProcess::setSubForcedOnly(bool b) {
	writeToStdin(QString("set sub-forced-only %1").arg(b ? "yes" : "no"));
}

void TMPVProcess::setSpeed(double value) {
	writeToStdin("set speed " + QString::number(value));
}

void TMPVProcess::enableKaraoke(bool) {
    logger()->warn("enableKaraoke: filter karaoke not supported");
}

void TMPVProcess::enableExtrastereo(bool b) {
	if (b)
		writeToStdin("af add lavfi=[extrastereo]");
	else
		writeToStdin("af del lavfi=[extrastereo]");
 }

void TMPVProcess::enableVolnorm(bool b, const QString& option) {
	if (b) writeToStdin("af add drc=" + option); else writeToStdin("af del drc=" + option);
}

void TMPVProcess::setAudioEqualizer(const QString& values) {
	if (values == previous_eq) return;

	if (!previous_eq.isEmpty()) {
		writeToStdin("af del equalizer=" + previous_eq);
	}
	writeToStdin("af add equalizer=" + values);
	previous_eq = values;
}

void TMPVProcess::setAudioDelay(double delay) {
	writeToStdin("set audio-delay " + QString::number(delay));
}

void TMPVProcess::setSubDelay(double delay) {
	writeToStdin("set sub-delay " + QString::number(delay));
}

void TMPVProcess::setLoop(int v) {
	QString o;
	switch (v) {
		case -1: o = "no"; break;
		case 0: o = "inf"; break;
		default: o = QString::number(v);
	}
	writeToStdin(QString("set loop %1").arg(o));
}

void TMPVProcess::takeScreenshot(ScreenshotType t, bool include_subtitles) {
	writeToStdin(QString("screenshot %1 %2").arg(include_subtitles ? "subtitles" : "video").arg(t == Single ? "single" : "each-frame"));
}

void TMPVProcess::switchCapturing() {

	if (!capture_filename.isEmpty()) {
		QString f;
		if (capturing) {
			f = "";
		} else {
			f = capture_filename;
#ifdef Q_OS_WIN
			// Escape backslash
			f = f.replace("\\", "\\\\");
#endif
		}
		writeToStdin("set stream-capture \"" + f + "\"");
		capturing = !capturing;
	}
}

void TMPVProcess::setTitle(int ID) {
	writeToStdin("set disc-title " + QString::number(ID));
}

void TMPVProcess::discSetMousePos(int, int) {

	// MPV versions later than 18 july 2015 no longer support menus

	// writeToStdin(QString("discnav mouse_move %1 %2").arg(x).arg(y));
	// mouse_move doesn't accept options :?

	// For some reason this doesn't work either...
	// So it's not possible to select options in the dvd menus just
	// because there's no way to pass the mouse position to mpv, or it
	// ignores it.
	// writeToStdin(QString("mouse %1 %2").arg(x).arg(y));
	// writeToStdin("discnav mouse_move");
}

void TMPVProcess::discButtonPressed(const QString& button_name) {
	writeToStdin("discnav " + button_name);
}

void TMPVProcess::setAspect(double aspect) {
	writeToStdin("set video-aspect " + QString::number(aspect));
}

void TMPVProcess::setZoomAndPan(double zoom, double pan_x, double pan_y, int osd_level) {

	// Wait until player is up
	if (notified_player_is_running) {
		bool clear_osd = false;
		if (zoom != this->zoom) {
			writeToStdin("set video-zoom " + QString::number(zoom - 1));
			this->zoom = zoom;
			clear_osd = true;
		}
		if (pan_x != this->pan_x) {
			writeToStdin("set video-pan-x " + QString::number(pan_x));
			this->pan_x = pan_x;
			clear_osd = true;
		}
		if (pan_y != this->pan_y) {
			writeToStdin("set video-pan-y " + QString::number(pan_y));
			this->pan_y = pan_y;
			clear_osd = true;
		}
		// Clear OSD message
		if (clear_osd) {
			writeToStdin("show_text \"\" 0 " + QString::number(osd_level));
		}
	}
}

#if PROGRAM_SWITCH
void TMPVProcess::setTSProgram(int ID) {
    logger()->debug("setTSProgram: function not supported");
}
#endif

void TMPVProcess::toggleDeinterlace() {
	writeToStdin("cycle deinterlace");
}

void TMPVProcess::setOSDScale(double value) {
	writeToStdin("set osd-scale " + QString::number(value));
}

void TMPVProcess::changeVF(const QString& filter, bool enable, const QVariant& option) {
    logger()->debug("changeVF: " + filter + QString::number(enable)
                  + option.toString());

	QString f;
	if (filter == "letterbox") {
		f = QString("expand=aspect=%1").arg(option.toDouble());
	}
	else
	if (filter == "noise") {
		f = "lavfi=[noise=alls=9:allf=t]";
	}
	else
	if (filter == "blur") {
		f = "lavfi=[unsharp=la=-1.5:ca=-1.5]";
	}
	else
	if (filter == "sharpen") {
		f = "lavfi=[unsharp=la=1.5:ca=1.5]";
	}
	else
	if (filter == "deblock") {
		f = "lavfi=[pp=" + option.toString() +"]";
	}
	else
	if (filter == "dering") {
		f = "lavfi=[pp=dr]";
	}
	else
	if (filter == "phase") {
		f = "lavfi=[phase=" + option.toString() +"]";
	}
	else
	if (filter == "postprocessing") {
		f = "lavfi=[pp]";
	}
	else
	if (filter == "hqdn3d") {
		QString o = option.toString();
		if (!o.isEmpty()) o = "=" + o;
		f = "lavfi=[hqdn3d" + o +"]";
	}
	else
	if (filter == "rotate") {
        f = "rotate=" + option.toString();
	}
	else
	if (filter == "flip" || filter == "mirror") {
		f = filter;
	}
	else
	if (filter == "scale" || filter == "gradfun") {
		f = filter;
		QString o = option.toString();
		if (!o.isEmpty()) f += "=" + o;
	}
	else
	if (filter == "lb" || filter == "l5") {
		f = "lavfi=[pp=" + filter +"]";
	}
	else
	if (filter == "yadif") {
		if (option.toString() == "1") {
			f = "yadif=field";
		} else {
			f = "yadif";
		}
	}
	else
	if (filter == "kerndeint") {
		f = "lavfi=[kerndeint=" + option.toString() +"]";
	}
	else {
        logger()->debug("changeVF: unknown filter: " + filter);
	}

	if (!f.isEmpty()) {
		writeToStdin(QString("vf %1 \"%2\"").arg(enable ? "add" : "del").arg(f));
	}
}

void TMPVProcess::changeStereo3DFilter(bool enable, const QString& in, const QString& out) {
	QString filter = "stereo3d=" + in + ":" + out;
	writeToStdin(QString("vf %1 \"%2\"").arg(enable ? "add" : "del").arg(filter));
}

void TMPVProcess::setSubStyles(const Settings::TAssStyles& styles, const QString&) {

	using namespace Settings;
	QString font = styles.fontname;
	//arg << "--sub-text-font=" + font.replace(" ", "");
    args << "--sub-text-font=" + font;
    args << "--sub-text-color=#" + TColorUtils::colorToRRGGBB(styles.primarycolor);

	if (styles.borderstyle == TAssStyles::Outline) {
        args << "--sub-text-shadow-color=#" + TColorUtils::colorToRRGGBB(styles.backcolor);
	} else {
        args << "--sub-text-back-color=#" + TColorUtils::colorToRRGGBB(styles.outlinecolor);
	}
    args << "--sub-text-border-color=#" + TColorUtils::colorToRRGGBB(styles.outlinecolor);

    args << "--sub-text-border-size=" + QString::number(styles.outline * 2.5);
    args << "--sub-text-shadow-offset=" + QString::number(styles.shadow * 2.5);

	if (isOptionAvailable("--sub-text-font-size")) {
        args << "--sub-text-font-size=" + QString::number(styles.fontsize * 2.5);
	}
	if (isOptionAvailable("--sub-text-bold")) {
        args << QString("--sub-text-bold=%1").arg(styles.bold ? "yes" : "no");
	}

	QString halign;
	switch (styles.halignment) {
		case TAssStyles::Left: halign = "left"; break;
		case TAssStyles::Right: halign = "right"; break;
	}

	QString valign;
	switch (styles.valignment) {
		case TAssStyles::VCenter: valign = "center"; break;
		case TAssStyles::Top: valign = "top"; break;
	}

	if (!halign.isEmpty() || !valign.isEmpty()) {
		if (isOptionAvailable("--sub-text-align-x")) {
            if (!halign.isEmpty()) args << "--sub-text-align-x=" + halign;
            if (!valign.isEmpty()) args << "--sub-text-align-y=" + valign;
		}
	}
}

void TMPVProcess::setChannelsFile(const QString& filename) {
    args << "--dvbin-file=" + filename;
}

} // namespace Process
} // namespace Player


#include "moc_mpvprocess.cpp"