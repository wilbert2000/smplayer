/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

#ifndef _MEDIASETTINGS_H_
#define _MEDIASETTINGS_H_


/* Settings the user has set for this file, and that we need to */
/* restore the video after a restart */

#include <QString>
#include <QSize>

class QSettings;

class MediaSettings {

public:
	enum LetterboxType { NoLetterbox = 0, Letterbox_43 = 1, Letterbox_169 = 2 };
	enum Denoise { NoDenoise = 0, DenoiseNormal = 1, DenoiseSoft = 2 };
	enum Aspect { AspectAuto = 1, Aspect43 = 2, Aspect169 = 3, Aspect235 = 4,
                  Aspect43Letterbox = 5, Aspect43Panscan = 6, 
                  Aspect43To169 = 7, Aspect149 = 8, Aspect1610 = 9, 
                  Aspect54 = 10, Aspect169Letterbox = 11 };
	enum Deinterlace { NoDeinterlace = 0, L5 = 1, Yadif = 2, LB = 3, 
                       Yadif_1 = 4, Kerndeint = 5 };
	enum AudioChannels { ChDefault = 0, ChStereo = 2, ChSurround = 4, 
                         ChFull51 = 6 };
	enum StereoMode { Stereo = 0, Left = 1, Right = 2 };
	enum IDs { NoneSelected = -1000, SubNone = 90000 };

	MediaSettings();
	virtual ~MediaSettings();

	virtual void reset();

	double current_sec;
	int current_sub_id;

	int current_audio_id;

	int current_title_id;
	int current_chapter_id;
	int current_angle_id;

	LetterboxType letterbox; // Force letterbox
	int aspect_ratio_id;

	//bool fullscreen;

	int volume;
	bool mute;

	int brightness, contrast, gamma, hue, saturation;

	QString external_subtitles;
	QString external_audio; // external audio file

	int sub_delay;
	int audio_delay;

	// Subtitles position (0-100)
	int sub_pos;
	int sub_scale;

	double speed; // Speed of playback: 1.0 = normal speed

	int current_deinterlacer;
	QString panscan_filter;
	QString crop_43to169_filter;

	// Filters in menu
	bool phase_filter;
	int current_denoiser;
	bool deblock_filter;
	bool dering_filter;
	bool noise_filter;
	bool postprocessing_filter;

	bool karaoke_filter;
	bool extrastereo_filter;
	bool volnorm_filter;

	int audio_use_channels;
	int stereo_mode;

	double panscan_factor; // mplayerwindow zoom

	bool flip; //!< Flip image

	// This a property of the video and it should be
    // in mediadata, but we have to save it to preserve 
	// this data among restarts.
	double starting_time; // Some videos don't start at 0

	// Advanced settings
	QString forced_demuxer;
	QString forced_video_codec;
	QString forced_audio_codec;

	// A copy of the original values, so we can restore them.
	QString original_demuxer;
	QString original_video_codec;
	QString original_audio_codec;

	// Options to mplayer (for this file only)
	QString mplayer_additional_options;
	QString mplayer_additional_video_filters;
	QString mplayer_additional_audio_filters;

	// Some things that were before in mediadata
	// They can vary, because of filters, so better here

	//Resolution used by mplayer
    //Can be bigger that video resolution
    //because of the aspect ratio or expand filter
    int win_width;
    int win_height;
    double win_aspect();


	void list();
	void save(QSettings * set);
	void load(QSettings * set);
};

#endif
