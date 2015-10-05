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


#include "gui/pref/performance.h"
#include "images.h"
#include "settings/preferences.h"

using namespace Settings;

namespace Gui { namespace Pref {

TPerformance::TPerformance(QWidget * parent, Qt::WindowFlags f)
	: TWidget(parent, f )
{
	setupUi(this);

	hwdec_combo->addItem(tr("None"), "no");
	hwdec_combo->addItem(tr("Auto"), "auto");
	#ifdef Q_OS_LINUX
	hwdec_combo->addItem("vdpau", "vdpau");
	hwdec_combo->addItem("vaapi", "vaapi");
	hwdec_combo->addItem("vaapi-copy", "vaapi-copy");
	#endif
	#ifdef Q_OS_OSX
	hwdec_combo->addItem("vda", "vda");
	#endif
	#ifdef Q_OS_WIN
	hwdec_combo->addItem("dxva2-copy", "dxva2-copy");
	#endif

	// Priority is only for windows, so we disable for other systems
#ifndef Q_OS_WIN
	priority_group->hide();
#endif

	retranslateStrings();
}

TPerformance::~TPerformance()
{
}

QString TPerformance::sectionName() {
	return tr("Performance");
}

QPixmap TPerformance::sectionIcon() {
    return Images::icon("pref_performance", 22);
}


void TPerformance::retranslateStrings() {
	int priority = priority_combo->currentIndex();
	int loop_filter = loopfilter_combo->currentIndex();

	retranslateUi(this);

	loopfilter_combo->clear();
	loopfilter_combo->addItem( tr("Enabled"), TPreferences::LoopEnabled );
	loopfilter_combo->addItem( tr("Skip (always)"), TPreferences::LoopDisabled );
	loopfilter_combo->addItem( tr("Skip only on HD videos"), TPreferences::LoopDisabledOnHD );

	priority_combo->setCurrentIndex(priority);
	loopfilter_combo->setCurrentIndex(loop_filter);

	createHelp();
}

void TPerformance::setData(TPreferences * pref) {
	setCacheForFiles( pref->cache_for_files );
	setCacheForStreams( pref->cache_for_streams );
	setCacheForDVDs( pref->cache_for_dvds );
	setCacheForAudioCDs( pref->cache_for_audiocds );
	setCacheForVCDs( pref->cache_for_vcds );
	setCacheForTV( pref->cache_for_tv );

	setPriority( pref->priority );
	setFrameDrop( pref->frame_drop );
	setHardFrameDrop( pref->hard_frame_drop );
	setCoreavcUsage( pref->coreavc );
	setSkipLoop( pref->h264_skip_loop_filter );
	setThreads( pref->threads );
	setHwdec( pref->hwdec );
}

void TPerformance::getData(TPreferences * pref) {
	requires_restart = false;

	TEST_AND_SET(pref->cache_for_files, cacheForFiles());
	TEST_AND_SET(pref->cache_for_streams, cacheForStreams());
	TEST_AND_SET(pref->cache_for_dvds, cacheForDVDs());
	TEST_AND_SET(pref->cache_for_audiocds, cacheForAudioCDs());
	TEST_AND_SET(pref->cache_for_vcds, cacheForVCDs());
	TEST_AND_SET(pref->cache_for_tv, cacheForTV());

	TEST_AND_SET(pref->priority, priority());
	TEST_AND_SET(pref->frame_drop, frameDrop());
	TEST_AND_SET(pref->hard_frame_drop, hardFrameDrop());
	TEST_AND_SET(pref->coreavc, coreavcUsage())
	TEST_AND_SET(pref->h264_skip_loop_filter, skipLoop());
	TEST_AND_SET(pref->threads, threads());
	TEST_AND_SET(pref->hwdec, hwdec());
}

void TPerformance::setCacheForFiles(int n) {
	cache_files_spin->setValue(n);
}

int TPerformance::cacheForFiles() {
	return cache_files_spin->value();
}

void TPerformance::setCacheForStreams(int n) {
	cache_streams_spin->setValue(n);
}

int TPerformance::cacheForStreams() {
	return cache_streams_spin->value();
}

void TPerformance::setCacheForDVDs(int n) {
	cache_dvds_spin->setValue(n);
}

int TPerformance::cacheForDVDs() {
	return cache_dvds_spin->value();
}

void TPerformance::setCacheForAudioCDs(int n) {
	cache_cds_spin->setValue(n);
}

int TPerformance::cacheForAudioCDs() {
	return cache_cds_spin->value();
}

void TPerformance::setCacheForVCDs(int n) {
	cache_vcds_spin->setValue(n);
}

int TPerformance::cacheForVCDs() {
	return cache_vcds_spin->value();
}

void TPerformance::setCacheForTV(int n) {
	cache_tv_spin->setValue(n);
}

int TPerformance::cacheForTV() {
	return cache_tv_spin->value();
}

void TPerformance::setPriority(int n) {
	priority_combo->setCurrentIndex(n);
}

int TPerformance::priority() {
	return priority_combo->currentIndex();
}

void TPerformance::setFrameDrop(bool b) {
	framedrop_check->setChecked(b);
}

bool TPerformance::frameDrop() {
	return framedrop_check->isChecked();
}

void TPerformance::setHardFrameDrop(bool b) {
	hardframedrop_check->setChecked(b);
}

bool TPerformance::hardFrameDrop() {
	return hardframedrop_check->isChecked();
}

void TPerformance::setCoreavcUsage(bool b) {
	coreavc_check->setChecked(b);
}

bool TPerformance::coreavcUsage() {
	return coreavc_check->isChecked();
}

void TPerformance::setSkipLoop(TPreferences::H264LoopFilter value) {
	loopfilter_combo->setCurrentIndex(loopfilter_combo->findData(value));
}

TPreferences::H264LoopFilter TPerformance::skipLoop() {
	return (TPreferences::H264LoopFilter) loopfilter_combo->itemData(loopfilter_combo->currentIndex()).toInt();
}

void TPerformance::setThreads(int v) {
	threads_spin->setValue(v);
}

int TPerformance::threads() {
	return threads_spin->value();
}

void TPerformance::setHwdec(const QString & v) {
	int idx = hwdec_combo->findData(v);
	if (idx < 0) idx = 0;
	hwdec_combo->setCurrentIndex(idx);
}

QString TPerformance::hwdec() {
	int idx = hwdec_combo->currentIndex();
	return hwdec_combo->itemData(idx).toString();
}

void TPerformance::createHelp() {
	clearHelp();

	addSectionTitle(tr("Performance"));
	
	// Performance tab
#ifdef Q_OS_WIN
	setWhatsThis(priority_combo, tr("Priority"), 
		tr("Set process priority for mplayer according to the predefined "
           "priorities available under Windows.<br>"
           "<b>Warning:</b> Using realtime priority can cause system lockup."));
#endif

	setWhatsThis(framedrop_check, tr("Allow frame drop"),
		tr("Skip displaying some frames to maintain A/V sync on slow systems." ) );

	setWhatsThis(hardframedrop_check, tr("Allow hard frame drop"),
		tr("More intense frame dropping (breaks decoding). "
           "Leads to image distortion!") );

	setWhatsThis(threads_spin, tr("Threads for decoding"),
		tr("Sets the number of threads to use for decoding. Only for "
           "MPEG-1/2 and H.264") );

	setWhatsThis(hwdec_combo, tr("Hardware decoding"),
		tr("Sets the hardware video decoding API. "
		   "If hardware decoding is not possible, software decoding will be used instead.") + " " +
		tr("Available options:") +
			"<ul>"
			"<li>" + tr("None: only software decoding will be used.") + "</li>"
			"<li>" + tr("Auto: it tries to automatically enable hardware decoding using the first available method.") + "</li>"
			#ifdef Q_OS_LINUX
			"<li>" + tr("vdpau: for the vdpau and opengl video outputs.") + "</li>"
			"<li>" + tr("vaapi: for the opengl and vaapi video outputs. For Intel GPUs only.") + "</li>"
			"<li>" + tr("vaapi-copy: it copies video back into system RAM. For Intel GPUs only.") + "</li>"
			#endif
			#ifdef Q_OS_WIN
			"<li>" + tr("dxva2-copy: it copies video back to system RAM. Experimental.") + "</li>"
			#endif
			"</ul>" +
		tr("This option only works with mpv."));

	setWhatsThis(loopfilter_combo, tr("Skip loop filter"),
		tr("This option allows to skips the loop filter (AKA deblocking) "
           "during H.264 decoding. "
           "Since the filtered frame is supposed to be used as reference "
           "for decoding dependent frames this has a worse effect on quality "
           "than not doing deblocking on e.g. MPEG-2 video. But at least for "
           "high bitrate HDTV this provides a big speedup with no visible "
           "quality loss.") +"<br>"+
           tr("Possible values:") +"<br>" +
           tr("<b>Enabled</b>: the loop filter is not skipped")+"<br>"+
           tr("<b>Skip (always)</b>: the loop filter is skipped no matter the "
           "resolution of the video")+"<br>"+
           tr("<b>Skip only on HD videos</b>: the loop filter will be "
           "skipped only on videos which height is %1 or "
           "greater.").arg(pref->HD_height) +"<br>" );

	setWhatsThis(coreavc_check, tr("Use CoreAVC if no other codec specified"),
		tr("Try to use non-free CoreAVC codec when no other codec is specified "
           "and non-VDPAU video output selected. Requires MPlayer build with CoreAVC support."));


	addSectionTitle(tr("Cache"));

	setWhatsThis(cache_files_spin, tr("Cache for files"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a file.") );

	setWhatsThis(cache_streams_spin, tr("Cache for streams"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a URL.") );

	setWhatsThis(cache_dvds_spin, tr("Cache for DVDs"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a DVD.<br><b>Warning:</b> Seeking might not work "
           "properly (including chapter switching) when using a cache for DVDs.") );

	setWhatsThis(cache_cds_spin, tr("Cache for audio CDs"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching an audio CD.") );

	setWhatsThis(cache_vcds_spin, tr("Cache for VCDs"), 
		tr("This option specifies how much memory (in kBytes) to use when "
           "precaching a VCD.") );
}

}} // namespace Gui::Pref

#include "moc_performance.cpp"