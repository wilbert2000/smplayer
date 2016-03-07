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


#include "gui/pref/advanced.h"
#include <QColorDialog>
#include "images.h"
#include "settings/preferences.h"
#include "settings/paths.h"


namespace Gui { namespace Pref {

TAdvanced::TAdvanced(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f)
{
	setupUi(this);

	colorkey_label->hide();
	colorkey_view->hide();
	changeButton->hide();

	// Monitor aspect
	monitoraspect_combo->addItem("Auto");
	monitoraspect_combo->addItem("4:3");
	monitoraspect_combo->addItem("16:9");
	monitoraspect_combo->addItem("5:4");
	monitoraspect_combo->addItem("16:10");

	retranslateStrings();
}

TAdvanced::~TAdvanced() {
}

QString TAdvanced::sectionName() {
	return tr("Advanced");
}

QPixmap TAdvanced::sectionIcon() {
    return Images::icon("pref_advanced", 22);
}


void TAdvanced::retranslateStrings() {

	retranslateUi(this);

	monitor_aspect_icon->setPixmap(Images::icon("monitor"));

	monitoraspect_combo->setItemText(0, tr("Auto"));

	mplayer_crashes_check->setText(tr("R&eport %1 crashes").arg(pref->playerName()));
	advanced_tab->setTabText(1, tr("O&ptions for %1").arg(pref->playerName()));
	options_info_label->setText(tr("Here you can pass extra options to %1.").arg(pref->playerName()) +"<br>"+
		tr("Write them separated by spaces.") + "<br>" + tr("Example:") + " -volume 50 -fps 25");

	createHelp();
}

void TAdvanced::setData(TPreferences* pref) {
	setMonitorAspect(pref->monitor_aspect);

	setRepaintVideoBackground(pref->repaint_video_background);
	setMplayerAdditionalArguments(pref->mplayer_additional_options);
	setMplayerAdditionalVideoFilters(pref->mplayer_additional_video_filters);
	setMplayerAdditionalAudioFilters(pref->mplayer_additional_audio_filters);
	setColorKey(pref->color_key);

	setUseIdx(pref->use_idx);

	setUseLavfDemuxer(pref->use_lavf_demuxer);

	setUseCorrectPts(pref->use_correct_pts);
	setActionsToRun(pref->actions_to_run);
	setShowTagInTitle(pref->show_tag_in_window_title);

	setLogDebugEnabled(pref->log_debug_enabled);
	setLogVerbose(pref->log_verbose);
	setLogFile(pref->log_file);

	setMplayerCrashes(pref->report_mplayer_crashes);
}

void TAdvanced::getData(TPreferences* pref) {

	requires_restart = false;
	repaint_video_background_changed = false;
	monitor_aspect_changed = false;
	colorkey_changed = false;
	lavf_demuxer_changed = false;

	TEST_AND_SET(pref->use_idx, useIdx());

	if (pref->use_lavf_demuxer != useLavfDemuxer()) {
		pref->use_lavf_demuxer = useLavfDemuxer();
		lavf_demuxer_changed = true;
		requires_restart = true;
	}

	TEST_AND_SET(pref->use_correct_pts, useCorrectPts());
	pref->actions_to_run = actionsToRun();
	//TEST_AND_SET(pref->show_tag_in_window_title, showTagInTitle());
	pref->show_tag_in_window_title = showTagInTitle(); // TODO: detect change and apply

	if (pref->monitor_aspect != monitorAspect()) {
		pref->monitor_aspect = monitorAspect();
		monitor_aspect_changed = true;
		requires_restart = true;
	}

	if (pref->repaint_video_background != repaintVideoBackground()) {
		pref->repaint_video_background = repaintVideoBackground();
		repaint_video_background_changed = true;
    }

	TEST_AND_SET(pref->mplayer_additional_options, mplayerAdditionalArguments());
	TEST_AND_SET(pref->mplayer_additional_video_filters, mplayerAdditionalVideoFilters());
	TEST_AND_SET(pref->mplayer_additional_audio_filters, mplayerAdditionalAudioFilters());
	if (pref->color_key != colorKey()) {
		pref->color_key = colorKey();
		colorkey_changed = true;
		requires_restart = true;
	}

	pref->log_debug_enabled = logDebugEnabled();
	TEST_AND_SET(pref->log_verbose, logVerbose());
	pref->log_file = logFile();

	pref->report_mplayer_crashes = mplayerCrashes();
}

void TAdvanced::setMonitorAspect(QString asp) {
	if (asp.isEmpty())
		monitoraspect_combo->setCurrentIndex(0);
	else
		monitoraspect_combo->setCurrentText(asp);
		//monitoraspect_combo->setEditText(asp);
}

QString TAdvanced::monitorAspect() {
	if (monitoraspect_combo->currentIndex() == 0) 
		return "";
	else
		return monitoraspect_combo->currentText();
}

void TAdvanced::setRepaintVideoBackground(bool b) {
	repaint_video_background_check->setChecked(b);
}

bool TAdvanced::repaintVideoBackground() {
	return repaint_video_background_check->isChecked();
}

void TAdvanced::setMplayerCrashes(bool b) {
	mplayer_crashes_check->setChecked(b);
}

bool TAdvanced::mplayerCrashes() {
	return mplayer_crashes_check->isChecked();
}

void TAdvanced::setMplayerAdditionalArguments(QString args) {
	mplayer_args_edit->setText(args);
}

QString TAdvanced::mplayerAdditionalArguments() {
	return mplayer_args_edit->text();
}

void TAdvanced::setMplayerAdditionalVideoFilters(QString s) {
	mplayer_vfilters_edit->setText(s);
}

QString TAdvanced::mplayerAdditionalVideoFilters() {
	return mplayer_vfilters_edit->text();
}

void TAdvanced::setMplayerAdditionalAudioFilters(QString s) {
	mplayer_afilters_edit->setText(s);
}

QString TAdvanced::mplayerAdditionalAudioFilters() {
	return mplayer_afilters_edit->text();
}

void TAdvanced::setColorKey(unsigned int c) {
	QString color = QString::number(c, 16);
	while (color.length() < 6) color = "0" + color;
	colorkey_view->setText("#" + color);
}

unsigned int TAdvanced::colorKey() {

	QString c = colorkey_view->text();
	if (c.startsWith("#")) c = c.mid(1);

	bool ok;
	unsigned int color = c.toUInt(&ok, 16);

	if (!ok) 
		qWarning("Gui::Pref::TAdvanced::colorKey: cannot convert color to uint");

	qDebug("Gui::Pref::TAdvanced::colorKey: color: %s", QString::number(color, 16).toUtf8().data());
	return color;
}

void TAdvanced::setUseIdx(bool b) {
	idx_check->setChecked(b);
}

bool TAdvanced::useIdx() {
	return idx_check->isChecked();
}

void TAdvanced::setUseLavfDemuxer(bool b) {
	lavf_demuxer_check->setChecked(b);
}

bool TAdvanced::useLavfDemuxer() {
	return lavf_demuxer_check->isChecked();
}

void TAdvanced::setUseCorrectPts(TPreferences::TOptionState value) {
	correct_pts_combo->setState(value);
}

TPreferences::TOptionState TAdvanced::useCorrectPts() {
	return correct_pts_combo->state();
}

void TAdvanced::setActionsToRun(QString actions) {
	actions_to_run_edit->setText(actions);
}

QString TAdvanced::actionsToRun() {
	return actions_to_run_edit->text();
}

void TAdvanced::setShowTagInTitle(bool b) {
	show_tag_in_title_check->setChecked(b);
}

bool TAdvanced::showTagInTitle() {
	return show_tag_in_title_check->isChecked();
}

void TAdvanced::on_changeButton_clicked() {
	//bool ok;
	//int color = colorkey_view->text().toUInt(&ok, 16);
	QColor color(colorkey_view->text());
	QColor c = QColorDialog::getColor (color, this);
	if (c.isValid()) {
		//colorkey_view->setText(QString::number(c.rgb(), 16));
		colorkey_view->setText(c.name());
	}
}

// Log options
void TAdvanced::setLogDebugEnabled(bool b) {
	log_debug_enabled_check->setChecked(b);
}

bool TAdvanced::logDebugEnabled() {
	return log_debug_enabled_check->isChecked();
}

void TAdvanced::setLogVerbose(bool b) {
	log_verbose_check->setChecked(b);
}

bool TAdvanced::logVerbose() {
	return log_verbose_check->isChecked();
}

void TAdvanced::setLogFile(bool b) {
	log_file_check->setChecked(b);
}

bool TAdvanced::logFile() {
	return log_file_check->isChecked();
}

void TAdvanced::createHelp() {
	clearHelp();

	addSectionTitle(tr("Advanced"));

	setWhatsThis(monitoraspect_combo, tr("Monitor aspect"),
        tr("Select the aspect ratio of your monitor."));

	setWhatsThis(idx_check, tr("Rebuild index if needed"),
		tr("Rebuilds index of files if no index was found, allowing seeking. "
		   "Useful with broken/incomplete downloads, or badly created files. "
           "This option only works if the underlying media supports "
           "seeking (i.e. not with stdin, pipe, etc).<br> "
           "<b>Note:</b> the creation of the index may take some time."));

	setWhatsThis(lavf_demuxer_check, tr("Use the lavf demuxer by default"),
		tr("If this option is checked, the lavf demuxer will be used for all formats."));

	setWhatsThis(repaint_video_background_check, 
        tr("Repaint the background of the video window"),
		tr("Checking this option may reduce flickering, but it also might "
           "produce that the video won't be displayed properly."));

	setWhatsThis(mplayer_crashes_check, 
		tr("Report %1 crashes").arg(pref->playerName()),
		tr("If this option is checked, a window will appear to inform "
           "about %1 crashes. Otherwise those failures will be "
           "silently ignored.").arg(pref->playerName()));

	setWhatsThis(correct_pts_combo, tr("Correct pts"),
		tr("Switches %1 to an experimental mode where timestamps for "
           "video frames are calculated differently and video filters which "
           "add new frames or modify timestamps of existing ones are "
           "supported. The more accurate timestamps can be visible for "
           "example when playing subtitles timed to scene changes with the "
           "SSA/ASS library enabled. Without correct pts the subtitle timing "
           "will typically be off by some frames. This option does not work "
           "correctly with some demuxers and codecs.").arg(pref->playerName()));

	setWhatsThis(actions_to_run_edit, tr("Actions list"),
		tr("Here you can specify a list of <i>actions</i> which will be "
           "run every time a file is opened. You'll find all available "
           "actions in the key shortcut editor in the <b>Keyboard and mouse</b> "
           "section. The actions must be separated by spaces. Checkable "
           "actions can be followed by <i>true</i> or <i>false</i> to "
           "enable or disable the action.") +"<br>"+
		tr("Example:") +" <i>auto_zoom fullscreen true</i><br>" +
		tr("Limitation: the actions are run only when a file is opened and "
           "not when the mplayer process is restarted (e.g. you select an "
           "audio or video filter)."));

	setWhatsThis(colorkey_view, tr("Colorkey"),
        tr("If you see parts of the video over any other window, you can "
           "change the colorkey to fix it. Try to select a color close to "
           "black."));

	setWhatsThis(show_tag_in_title_check, tr("Show tag info in window title"),
		tr("If this option is enabled, information from tags will be "
		   "shown in window title. "
           "Otherwise only the filename will be shown."));

	addSectionTitle(tr("Options for %1").arg(pref->playerName()));

	setWhatsThis(mplayer_args_edit, tr("Options"),
        tr("Here you can type options for %1.").arg(pref->playerName()) +" "+
        tr("Write them separated by spaces."));

	setWhatsThis(mplayer_vfilters_edit, tr("Video filters"),
        tr("Here you can add video filters for %1.").arg(pref->playerName()) +" "+
        tr("Write them separated by commas. Don't use spaces!"));

	setWhatsThis(mplayer_afilters_edit, tr("Audio filters"),
        tr("Here you can add audio filters for %1.").arg(pref->playerName()) +" "+
        tr("Write them separated by commas. Don't use spaces!"));


	addSectionTitle(tr("Logs"));

	setWhatsThis(log_debug_enabled_check, tr("Log debug messages"),
		tr("If checked, SMPlayer will log debug messages, "
		   "which might give additional information in case of trouble. "
		   "Non-debug messages are always logged. "
		   "You can view the log with menu <b>Options -> View log</b>."));

	setWhatsThis(log_file_check, tr("Save SMPlayer log to file"),
		tr("If this option is checked, the SMPlayer log wil be recorded to %1")
          .arg("<i>"+ TPaths::configPath() + "/smplayer_log.txt</i>"));
}

}} // namespace Gui::Pref

#include "moc_advanced.cpp"
