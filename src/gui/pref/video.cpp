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

#include "gui/pref/video.h"
#include <QDebug>
#include "images.h"
#include "settings/preferences.h"
#include "settings/mediasettings.h"
#include "gui/pref/vdpauproperties.h"


using namespace Settings;

namespace Gui {
namespace Pref {

TVideo::TVideo(QWidget* parent, InfoList vol)
	: TWidget(parent, 0)
	, vo_list(vol) {

	setupUi(this);

#if USE_XV_ADAPTORS
	xv_adaptors = TDeviceInfo::xvAdaptors();
#endif

	// Hardware decoding combo
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

#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
	vdpau_button->hide();
#endif

	connect(vo_combo, SIGNAL(currentIndexChanged(int)),
			this, SLOT(vo_combo_changed(int)));

	// Monitor aspect
	monitoraspect_combo->addItem("Auto");
	monitoraspect_combo->addItem("4:3");
	monitoraspect_combo->addItem("16:9");
	monitoraspect_combo->addItem("5:4");
	monitoraspect_combo->addItem("16:10");

	retranslateStrings();
}

TVideo::~TVideo() {
}

QString TVideo::sectionName() {
	return tr("Video");
}

QPixmap TVideo::sectionIcon() {
	return Images::icon("pref_video", icon_size);
}

void TVideo::retranslateStrings() {

	retranslateUi(this);

	video_icon_label->setPixmap(Images::icon("pref_video"));

	updateDriverCombo(true);

	int index = deinterlace_combo->currentIndex();
	deinterlace_combo->clear();
	deinterlace_combo->addItem(tr("None"), TMediaSettings::NoDeinterlace);
	deinterlace_combo->addItem(tr("Lowpass5"), TMediaSettings::L5);
	deinterlace_combo->addItem(tr("Yadif (normal)"), TMediaSettings::Yadif);
	deinterlace_combo->addItem(tr("Yadif (double framerate)"), TMediaSettings::Yadif_1);
	deinterlace_combo->addItem(tr("Linear Blend"), TMediaSettings::LB);
	deinterlace_combo->addItem(tr("Kerndeint"), TMediaSettings::Kerndeint);
	deinterlace_combo->setCurrentIndex(index);

	index = deinterlace_tv_combo->currentIndex();
	deinterlace_tv_combo->clear();
	deinterlace_tv_combo->addItem(tr("None"), TMediaSettings::NoDeinterlace);
	deinterlace_tv_combo->addItem(tr("Lowpass5"), TMediaSettings::L5);
	deinterlace_tv_combo->addItem(tr("Yadif (normal)"), TMediaSettings::Yadif);
	deinterlace_tv_combo->addItem(tr("Yadif (double framerate)"), TMediaSettings::Yadif_1);
	deinterlace_tv_combo->addItem(tr("Linear Blend"), TMediaSettings::LB);
	deinterlace_tv_combo->addItem(tr("Kerndeint"), TMediaSettings::Kerndeint);
	deinterlace_tv_combo->setCurrentIndex(index);

	// Monitor
	monitor_aspect_icon->setPixmap(Images::icon("monitor"));
	monitoraspect_combo->setItemText(0, tr("Auto"));

	createHelp();
}

void TVideo::setData(Settings::TPreferences* pref) {

	// Video out driver
	setVO(pref->vo, true);

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	vdpau = pref->vdpau;
#endif

	setHwdec(pref->hwdec);
	setSoftwareVideoEqualizer(pref->use_soft_video_eq);

	setFrameDrop(pref->frame_drop);
	setHardFrameDrop(pref->hard_frame_drop);
	correct_pts_combo->setState(pref->use_correct_pts);

	setInitialPostprocessing(pref->initial_postprocessing);
	setPostprocessingQuality(pref->postprocessing_quality);
	setInitialDeinterlace(pref->initial_deinterlace);
	setInitialDeinterlaceTV(pref->initial_tv_deinterlace);
	setInitialZoom(pref->initial_zoom_factor);

	// Monitor
	setMonitorAspect(pref->monitor_aspect);
}

void TVideo::getData(Settings::TPreferences* pref) {

	restartIfStringChanged(pref->vo, VO());

#if !defined(Q_OS_WIN) && !defined(Q_OS_OS2)
	pref->vdpau = vdpau;
#endif

	restartIfStringChanged(pref->hwdec, hwdec());
	restartIfBoolChanged(pref->use_soft_video_eq, softwareVideoEqualizer());

	restartIfBoolChanged(pref->frame_drop, frameDrop());
	restartIfBoolChanged(pref->hard_frame_drop, hardFrameDrop());
	TPreferences::TOptionState pts = correct_pts_combo->state();
	if (pts != pref->use_correct_pts) {
		pref->use_correct_pts = pts;
		requires_restart = true;
	}

	pref->initial_postprocessing = initialPostprocessing();
	restartIfIntChanged(pref->postprocessing_quality, postprocessingQuality());
	pref->initial_deinterlace = initialDeinterlace();
	pref->initial_tv_deinterlace = initialDeinterlaceTV();
	pref->initial_zoom_factor = initialZoom();

	// Monitor
	restartIfStringChanged(pref->monitor_aspect, monitorAspect());
}

void TVideo::updateDriverCombo(bool allow_user_defined_vo) {

	QString current_vo = VO();
	vo_combo->clear();
	vo_combo->addItem(tr("Default"), "");

	QString vo;
	for (int n = 0; n < vo_list.count(); n++) {
		vo = vo_list[n].name();

#ifdef Q_OS_WIN
		if (vo == "directx") {
			vo_combo->addItem("directx (" + tr("fast") + ")", "directx");
			vo_combo->addItem("directx (" + tr("slow") + ")", "directx:noaccel");
		}
#else
#ifdef Q_OS_OS2
		if (vo == "kva") {
			vo_combo->addItem("kva (" + tr("fast") + ")", "kva");
			vo_combo->addItem("kva (" + tr("snap mode") + ")", "kva:snap");
			vo_combo->addItem("kva (" + tr("slower dive mode") + ")", "kva:dive");
		}
#else
		/*
		if (vo == "xv") vo_combo->addItem("xv (" + tr("fastest") + ")", vo);
		*/
#if USE_XV_ADAPTORS
		if (vo == "xv" && !xv_adaptors.isEmpty()) {
			vo_combo->addItem(vo, vo);
			for (int n = 0; n < xv_adaptors.count(); n++) {
				vo_combo->addItem("xv (" + xv_adaptors[n].ID().toString()
								  + " - " + xv_adaptors[n].desc() + ")",
								  "xv:adaptor=" + xv_adaptors[n].ID().toString());
			}
		}
#endif // USE_XV_ADAPTORS
#endif
#endif

		else if (vo == "x11") {
			vo_combo->addItem("x11 (" + tr("slow") + ")", vo);
		} else if (vo == "gl") {
			vo_combo->addItem(vo, vo);
			vo_combo->addItem("gl (" + tr("fast") + ")", "gl:yuv=2:force-pbo");
			vo_combo->addItem("gl (" + tr("fast - ATI cards") + ")", "gl:yuv=2:force-pbo:ati-hack");
			vo_combo->addItem("gl (yuv)", "gl:yuv=3");
		} else if (vo == "gl2") {
			vo_combo->addItem(vo, vo);
			vo_combo->addItem("gl2 (yuv)", "gl2:yuv=3");
		} else if (vo == "gl_tiled") {
			vo_combo->addItem(vo, vo);
			vo_combo->addItem("gl_tiled (yuv)", "gl_tiled:yuv=3");
		} else if (vo == "null"
				   || vo == "png"
				   || vo == "jpeg"
				   || vo == "gif89a"
				   || vo == "tga"
				   || vo == "pnm"
				   || vo == "md5sum") {
			// Nothing to do
		} else {
			vo_combo->addItem(vo, vo);
		}
	}
	vo_combo->addItem(tr("User defined..."), "user_defined");

	setVO(current_vo, allow_user_defined_vo);
}

void TVideo::setVO(const QString& vo_driver, bool allow_user_defined) {

	int idx = vo_combo->findData(vo_driver);
	if (idx >= 0) {
		vo_combo->setCurrentIndex(idx);
	} else if (allow_user_defined && !vo_driver.isEmpty()) {
		vo_combo->setCurrentIndex(vo_combo->findData("user_defined"));
		vo_user_defined_edit->setText(vo_driver);
	} else {
		vo_combo->setCurrentIndex(0);
	}
	vo_combo_changed(vo_combo->currentIndex());
}

QString TVideo::VO() {

	QString vo = vo_combo->itemData(vo_combo->currentIndex()).toString();
	if (vo == "user_defined") {
		vo = vo_user_defined_edit->text();
	}
	return vo;
}

void TVideo::setHwdec(const QString& v) {

	int idx = hwdec_combo->findData(v);
	if (idx < 0)
		idx = 0;
	hwdec_combo->setCurrentIndex(idx);
}

QString TVideo::hwdec() {

	int idx = hwdec_combo->currentIndex();
	return hwdec_combo->itemData(idx).toString();
}

void TVideo::setFrameDrop(bool b) {
	framedrop_check->setChecked(b);
}

bool TVideo::frameDrop() {
	return framedrop_check->isChecked();
}

void TVideo::setHardFrameDrop(bool b) {
	hardframedrop_check->setChecked(b);
}

bool TVideo::hardFrameDrop() {
	return hardframedrop_check->isChecked();
}

void TVideo::setSoftwareVideoEqualizer(bool b) {
	software_video_equalizer_check->setChecked(b);
}

bool TVideo::softwareVideoEqualizer() {
	return software_video_equalizer_check->isChecked();
}

void TVideo::setInitialPostprocessing(bool b) {
	postprocessing_check->setChecked(b);
}

bool TVideo::initialPostprocessing() {
	return postprocessing_check->isChecked();
}

void TVideo::setInitialDeinterlace(int ID) {

	int pos = deinterlace_combo->findData(ID);
	if (pos < 0) {
		qWarning("Gui::Pref::TVideo::setInitialDeinterlace: ID: %d not found in combo", ID);
		pos = 0;
	}
	deinterlace_combo->setCurrentIndex(pos);
}

int TVideo::initialDeinterlace() {

	if (deinterlace_combo->currentIndex() >= 0) {
		return deinterlace_combo->itemData(deinterlace_combo->currentIndex()).toInt();
	}

	qWarning("Gui::Pref::TVideo::initialDeinterlace: no item selected");
	return 0;
}
void TVideo::setInitialDeinterlaceTV(int ID) {

	int i = deinterlace_tv_combo->findData(ID);
	if (i < 0) {
		i = 0;
		qWarning("Gui::Pref::TTV::setInitialDeinterlaceTV: ID: %d not found in combo", ID);
	}
	deinterlace_tv_combo->setCurrentIndex(i);
}

int TVideo::initialDeinterlaceTV() {

	int i = deinterlace_tv_combo->currentIndex();
	if (i < 0)
		i = 0;
	return deinterlace_tv_combo->itemData(i).toInt();
}

void TVideo::setInitialZoom(double v) {
	zoom_spin->setValue(v);
}

double TVideo::initialZoom() {
	return zoom_spin->value();
}

void TVideo::setPostprocessingQuality(int n) {
	postprocessing_quality_spin->setValue(n);
}

int TVideo::postprocessingQuality() {
	return postprocessing_quality_spin->value();
}

void TVideo::vo_combo_changed(int idx) {
	//qDebug("Gui::Pref::TVideo::vo_combo_changed: %d", idx);

	bool visible = (vo_combo->itemData(idx).toString() == "user_defined");
	vo_user_defined_edit->setVisible(visible);
	vo_user_defined_edit->setFocus();

#ifndef Q_OS_WIN
	bool vdpau_button_visible = (vo_combo->itemData(idx).toString() == "vdpau");
	vdpau_button->setVisible(vdpau_button_visible);
#endif
}

#ifndef Q_OS_WIN
void TVideo::on_vdpau_button_clicked() {
	qDebug("Gui::Pref::TVideo::on_vdpau_button_clicked");

	TVDPAUProperties d(this);

	d.setffh264vdpau(vdpau.ffh264vdpau);
	d.setffmpeg12vdpau(vdpau.ffmpeg12vdpau);
	d.setffwmv3vdpau(vdpau.ffwmv3vdpau);
	d.setffvc1vdpau(vdpau.ffvc1vdpau);
	d.setffodivxvdpau(vdpau.ffodivxvdpau);

	d.setDisableFilters(vdpau.disable_video_filters);

	if (d.exec() == QDialog::Accepted) {
		restartIfBoolChanged(vdpau.ffh264vdpau, d.ffh264vdpau());
		restartIfBoolChanged(vdpau.ffmpeg12vdpau, d.ffmpeg12vdpau());
		restartIfBoolChanged(vdpau.ffwmv3vdpau, d.ffwmv3vdpau());
		restartIfBoolChanged(vdpau.ffvc1vdpau, d.ffvc1vdpau());
		restartIfBoolChanged(vdpau.ffodivxvdpau, d.ffodivxvdpau());

		restartIfBoolChanged(vdpau.disable_video_filters, d.disableFilters());
	}
}
#endif

void TVideo::setMonitorAspect(const QString& asp) {

	if (asp.isEmpty())
		monitoraspect_combo->setCurrentIndex(0);
	else
		monitoraspect_combo->setCurrentText(asp);
}

QString TVideo::monitorAspect() {

	if (monitoraspect_combo->currentIndex() <= 0)
		return "";
	else
		return monitoraspect_combo->currentText();
}

void TVideo::createHelp() {

	clearHelp();

	addSectionTitle(tr("Video"));
	addSectionGroup(tr("Output"));

	setWhatsThis(vo_combo, tr("Video output driver"),
		tr("Select the video output driver. %1 provides the best performance.")
#ifdef Q_OS_WIN
		  .arg("<b><i>directx</i></b>")
#else
#ifdef Q_OS_OS2
		  .arg("<b><i>kva</i></b>")
#else
		  .arg("<b><i>xv</i></b>")
#endif
#endif
		);

	setWhatsThis(hwdec_combo, tr("Hardware decoding"),
		tr("Sets the hardware video decoding API."
		   "If hardware decoding is not possible, software decoding will be used instead.")
			+ " "
			+ tr("Available options:")
			+ "<ul>"
			"<li>" + tr("None: only software decoding will be used.") + "</li>"
			"<li>" + tr("Auto: tries to automatically enable hardware decoding.") + "</li>"

#ifdef Q_OS_LINUX
			"<li>" + tr("vdpau: for the vdpau and opengl video outputs.") + "</li>"
			"<li>" + tr("vaapi: for the opengl and vaapi video outputs. For Intel GPUs only.") + "</li>"
			"<li>" + tr("vaapi-copy: it copies video back into system RAM. For Intel GPUs only.") + "</li>"
#endif

#ifdef Q_OS_WIN
			"<li>" + tr("dxva2-copy: copies video back to system RAM.") + "</li>"
#endif

			"</ul>"
			+ tr("This option only works with mpv."));


	setWhatsThis(software_video_equalizer_check, tr("Software video equalizer"),
		tr("You can check this option if video equalizing is not supported by "
		   "your graphic card or the selected video output driver.<br>"
		   "<b>Note:</b> this option can be incompatible with some video "
		   "output drivers."));

	addSectionGroup(tr("Synchronization"));

	setWhatsThis(framedrop_check, tr("Allow frame drop"),
		tr("Skip displaying some frames to maintain A/V sync on slow systems."));

	setWhatsThis(hardframedrop_check, tr("Allow hard frame drop"),
		tr("More intense frame dropping (breaks decoding). "
		   "Leads to image distortion!"));

	setWhatsThis(correct_pts_combo, tr("Correct PTS"),
		tr("Switches the player to an experimental mode with more accurate"
		   " timestamps and supporting video filters which add new frames or"
		   " modify timestamps. The more accurate timestamps can be visible for"
		   " example when playing subtitles timed to scene changes with the"
		   " SSA/ASS library enabled. Without correct PTS the subtitle timing"
		   " will typically be off by some frames. This option does not work"
		   " correctly with some demuxers and codecs."));


	addSectionGroup(tr("Defaults"));

	setWhatsThis(postprocessing_check, tr("Enable postprocessing by default"),
		tr("Postprocessing will be used by default on new opened files."));

	setWhatsThis(postprocessing_quality_spin, tr("Postprocessing quality"),
		tr("Dynamically changes the level of postprocessing depending on the "
		   "available spare CPU time. The number you specify will be the "
		   "maximum level used. Usually you can use some big number."));

	setWhatsThis(deinterlace_combo, tr("Deinterlace"),
		tr("Select the deinterlace filter that you want to be used for new "
		   "videos opened.") +" "+
		tr("<b>Note:</b> This option won't be used for TV channels."));

	setWhatsThis(deinterlace_tv_combo, tr("Deinterlace for TV"),
		tr("Select the deinterlace filter that you want to be used for TV channels."));

	setWhatsThis(zoom_spin, tr("Zoom"),
		tr("This option sets the default zoom used for new videos."));

	addSectionTitle("Monitor");

	setWhatsThis(monitoraspect_combo, tr("Monitor aspect"),
		tr("Select or enter the aspect ratio of your monitor."
		   "Accepts w:h and floating point notations."));
}

}} // namespace Gui::Pref

#include "moc_video.cpp"
