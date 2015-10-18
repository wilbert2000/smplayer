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


#include "gui/pref/interface.h"
#include "images.h"
#include "settings/preferences.h"
#include "settings/recents.h"
#include "settings/urlhistory.h"
#include "paths.h"
#include "languages.h"
#include "gui/autohidetoolbar.h"

#include <QDir>
#include <QStyleFactory>
#include <QFontDialog>

#define SINGLE_INSTANCE_TAB 2

namespace Gui { namespace Pref {

TInterface::TInterface(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f)
{
	setupUi(this);
	/* volume_icon->hide(); */

	// Style combo
	style_combo->addItem("<default>");
	style_combo->addItems(QStyleFactory::keys());

	// Icon set combo
	iconset_combo->addItem("Default");

#ifdef SKINS
	n_skins = 0;
#endif

	// User
	QDir icon_dir = Paths::configPath() + "/themes";
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	QStringList iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n=0; n < iconsets.count(); n++) {
		#ifdef SKINS
		QString css_file = Paths::configPath() + "/themes/" + iconsets[n] + "/main.css";
		bool is_skin = QFile::exists(css_file);
		//qDebug("***** %s %d", css_file.toUtf8().constData(), is_skin);
		if (is_skin) {
			skin_combo->addItem(iconsets[n]);
			n_skins++;
		}
		else
		#endif
		iconset_combo->addItem(iconsets[n]);
	}
	// Global
	icon_dir = Paths::themesPath();
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n=0; n < iconsets.count(); n++) {
		#ifdef SKINS
		QString css_file = Paths::themesPath() + "/" + iconsets[n] + "/main.css";
		bool is_skin = QFile::exists(css_file);
		//qDebug("***** %s %d", css_file.toUtf8().constData(), is_skin);
		if ((is_skin) && (skin_combo->findText(iconsets[n]) == -1)) {
			skin_combo->addItem(iconsets[n]);
			n_skins++;
		}
		else
		#endif
		if (iconset_combo->findText(iconsets[n]) == -1) {
			iconset_combo->addItem(iconsets[n]);
		}
	}
	#ifdef SKINS
	if (skin_combo->itemText(0) == "Black") {
		skin_combo->removeItem(0);
		skin_combo->addItem("Black");
	}
	#endif

#ifdef SINGLE_INSTANCE
	connect(single_instance_check, SIGNAL(toggled(bool)), 
            this, SLOT(changeInstanceImages()));
#else
	tabWidget->setTabEnabled(SINGLE_INSTANCE_TAB, false);
#endif

#ifdef SKINS
	connect(gui_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(GUIChanged(int)));
#endif

#ifndef SKINS
	skin_combo->hide();
	skin_label->hide();
	skin_sp->hide();
#endif

	retranslateStrings();
}

TInterface::~TInterface()
{
}

QString TInterface::sectionName() {
	return tr("Interface");
}

QPixmap TInterface::sectionIcon() {
    return Images::icon("pref_gui", 22);
}

void TInterface::createLanguageCombo() {
	QMap <QString,QString> m = Languages::translations();

	// Language combo
	QDir translation_dir = Paths::translationPath();
	QStringList languages = translation_dir.entryList(QStringList() << "*.qm");
	QRegExp rx_lang("smplayer_(.*)\\.qm");
	language_combo->clear();
	language_combo->addItem(tr("<Autodetect>"));
	for (int n=0; n < languages.count(); n++) {
		if (rx_lang.indexIn(languages[n]) > -1) {
			QString l = rx_lang.cap(1);
			QString text = l;
			if (m.contains(l)) text = m[l] + " ("+l+")";
			language_combo->addItem(text, l);
		}
	}
}

void TInterface::retranslateStrings() {
	int mainwindow_resize = mainwindow_resize_combo->currentIndex();
	int timeslider_pos = timeslider_behaviour_combo->currentIndex();

	retranslateUi(this);

	mainwindow_resize_combo->setCurrentIndex(mainwindow_resize);
	timeslider_behaviour_combo->setCurrentIndex(timeslider_pos);

	// Icons
	resize_window_icon->setPixmap(Images::icon("resize_window"));
	/* volume_icon->setPixmap(Images::icon("speaker")); */

#ifdef SINGLE_INSTANCE
	changeInstanceImages();
#endif

	// Seek widgets
	seek1->setLabel(tr("&Short jump"));
	seek2->setLabel(tr("&Medium jump"));
	seek3->setLabel(tr("&Long jump"));
	seek4->setLabel(tr("Mouse &wheel jump"));

	if (qApp->isLeftToRight()) {
		seek1->setIcon(Images::icon("forward10s", 32));
		seek2->setIcon(Images::icon("forward1m", 32));
		seek3->setIcon(Images::icon("forward10m", 32));
	} else {
		seek1->setIcon(Images::flippedIcon("forward10s", 32));
		seek2->setIcon(Images::flippedIcon("forward1m", 32));
		seek3->setIcon(Images::flippedIcon("forward10m", 32));
	}
	seek4->setIcon(Images::icon("mouse",32));

	// Language combo
	int language_item = language_combo->currentIndex();
	createLanguageCombo();
	language_combo->setCurrentIndex(language_item);

	// Iconset combo
	iconset_combo->setItemText(0, tr("Default"));

	style_combo->setItemText(0, tr("Default"));

	int gui_index = gui_combo->currentIndex();
	gui_combo->clear();
	gui_combo->addItem(tr("Basic GUI"), "DefaultGUI");
#ifdef SKINS
	gui_combo->addItem(tr("Skinnable GUI"), "SkinGUI");
	if (n_skins == 0) {
		QModelIndex index = gui_combo->model()->index(gui_combo->count()-1,0);
		gui_combo->model()->setData(index, QVariant(0), Qt::UserRole -1);
	}
#endif
	gui_combo->setCurrentIndex(gui_index);

	floating_width_label->setNum(floating_width_slider->value());

	createHelp();
}

void TInterface::setData(Settings::TPreferences* pref) {
	setLanguage(pref->language);
	setIconSet(pref->iconset);

	setResizeMethod(pref->resize_method);
	setSaveSize(pref->save_window_size_on_exit);

#ifdef SINGLE_INSTANCE
	setUseSingleInstance(pref->use_single_instance);
#endif
	setSeeking1(pref->seeking1);
	setSeeking2(pref->seeking2);
	setSeeking3(pref->seeking3);
	setSeeking4(pref->seeking4);

	setUpdateWhileDragging(pref->update_while_seeking);
	setRelativeSeeking(pref->relative_seeking);
	setPreciseSeeking(pref->precise_seeking);

	reset_stop_check->setChecked(pref->reset_stop);

	setDefaultFont(pref->default_font);

	setHideVideoOnAudioFiles(pref->hide_video_window_on_audio_files);

	setStyle(pref->style);

	setGUI(pref->gui);

	setFloatingWidth(pref->floating_control_width);
	floating_move_bottom_check->setChecked(pref->floating_activation_area == Gui::TAutohideToolbar::Bottom);
	floating_hide_delay_spin->setValue(pref->floating_hide_delay);

	setRecentsMaxItems(pref->history_recents.maxItems());
	setURLMaxItems(pref->history_urls.maxItems());
	setRememberDirs(pref->save_dirs);
}

void TInterface::getData(Settings::TPreferences* pref) {
	requires_restart = false;
	language_changed = false;
	iconset_changed = false;
	gui_changed = false;
	style_changed = false;
	floating_control_width_changed = false;
	recents_changed = false;

	if (pref->language != language()) {
		pref->language = language();
		language_changed = true;
		qDebug("Gui::Pref::TInterface::getData: chosen language: '%s'", pref->language.toUtf8().data());
	}

	if (pref->iconset != iconSet()) {
		pref->iconset = iconSet();
		iconset_changed = true;
	}

	if (pref->gui != GUI()) {
		pref->gui = GUI();
		gui_changed = true;
	}

	pref->resize_method = resizeMethod();
	pref->save_window_size_on_exit = saveSize();

#ifdef SINGLE_INSTANCE
	pref->use_single_instance = useSingleInstance();
#endif

	pref->seeking1 = seeking1();
	pref->seeking2 = seeking2();
	pref->seeking3 = seeking3();
	pref->seeking4 = seeking4();

	pref->update_while_seeking = updateWhileDragging();
	pref->relative_seeking= relativeSeeking();
	pref->precise_seeking = preciseSeeking();

	pref->reset_stop = reset_stop_check->isChecked();

	pref->default_font = defaultFont();

	pref->hide_video_window_on_audio_files = hideVideoOnAudioFiles();

	if (pref->style != style()) {
		pref->style = style();
		style_changed = true;
	}

	if (pref->floating_control_width != floatingWidth()) {
		floating_control_width_changed = true;
		pref->floating_control_width = floatingWidth();
	}
	pref->floating_activation_area = floating_move_bottom_check->isChecked() ? Gui::TAutohideToolbar::Bottom : Gui::TAutohideToolbar::Anywhere;
	pref->floating_hide_delay = floating_hide_delay_spin->value();

	if (pref->history_recents.maxItems() != recentsMaxItems()) {
		pref->history_recents.setMaxItems(recentsMaxItems());
		recents_changed = true;
	}

	if (pref->history_urls.maxItems() != urlMaxItems()) {
		pref->history_urls.setMaxItems(urlMaxItems());
		url_max_changed = true;
	}

	pref->save_dirs = rememberDirs();
}

void TInterface::setLanguage(QString lang) {
	if (lang.isEmpty()) {
		language_combo->setCurrentIndex(0);
	}
	else {
		int pos = language_combo->findData(lang);
		if (pos != -1) 
			language_combo->setCurrentIndex(pos);
		else
			language_combo->setCurrentText(lang);
	}
}

QString TInterface::language() {
	if (language_combo->currentIndex()==0) 
		return "";
	else 
		return language_combo->itemData(language_combo->currentIndex()).toString();
}

void TInterface::setIconSet(QString set) {
	/*
	if (set.isEmpty())
		iconset_combo->setCurrentIndex(0);
	else
		iconset_combo->setCurrentText(set);
	*/
	iconset_combo->setCurrentIndex(0);
	for (int n=0; n < iconset_combo->count(); n++) {
		if (iconset_combo->itemText(n) == set) {
			iconset_combo->setCurrentIndex(n);
			break;
		}
	}
#ifdef SKINS
	skin_combo->setCurrentIndex(0);
	for (int n=0; n < skin_combo->count(); n++) {
		if (skin_combo->itemText(n) == set) {
			skin_combo->setCurrentIndex(n);
			break;
		}
	}
#endif
}

QString TInterface::iconSet() {
#ifdef SKINS
	QString GUI = gui_combo->itemData(gui_combo->currentIndex()).toString();
	if (GUI == "SkinGUI") {
		return skin_combo->currentText();
	}
	else
#endif
	if (iconset_combo->currentIndex()==0) 
		return "";
	else
		return iconset_combo->currentText();
}

void TInterface::setResizeMethod(int v) {
	mainwindow_resize_combo->setCurrentIndex(v);
}

int TInterface::resizeMethod() {
	return mainwindow_resize_combo->currentIndex();
}

void TInterface::setSaveSize(bool b) {
	save_size_check->setChecked(b);
}

bool TInterface::saveSize() {
	return save_size_check->isChecked();
}


void TInterface::setStyle(QString style) {
	if (style.isEmpty()) 
		style_combo->setCurrentIndex(0);
	else
		style_combo->setCurrentText(style);
}

QString TInterface::style() {
	if (style_combo->currentIndex()==0)
		return "";
	else 
		return style_combo->currentText();
}

void TInterface::setGUI(QString gui_name) {
#ifdef SKINS
	if ((n_skins == 0) && (gui_name == "SkinGUI")) gui_name = "DefaultGUI";
#endif
	int i = gui_combo->findData(gui_name);
	if (i < 0) i=0;
	gui_combo->setCurrentIndex(i);
}

QString TInterface::GUI() {
	return gui_combo->itemData(gui_combo->currentIndex()).toString();
}

#ifdef SKINS
void TInterface::GUIChanged(int index) {
	if (gui_combo->itemData(index).toString() == "SkinGUI") {
		iconset_combo->hide();
		iconset_label->hide();
		iconset_sp->hide();
		skin_combo->show();
		skin_label->show();
		skin_sp->show();
	} else {
		iconset_combo->show();
		iconset_label->show();
		iconset_sp->show();
		skin_combo->hide();
		skin_label->hide();
		skin_sp->hide();
	}
}
#endif

#ifdef SINGLE_INSTANCE
void TInterface::setUseSingleInstance(bool b) {
	single_instance_check->setChecked(b);
	//singleInstanceButtonToggled(b);
}

bool TInterface::useSingleInstance() {
	return single_instance_check->isChecked();
}
#endif

void TInterface::setSeeking1(int n) {
	seek1->setTime(n);
}

int TInterface::seeking1() {
	return seek1->time();
}

void TInterface::setSeeking2(int n) {
	seek2->setTime(n);
}

int TInterface::seeking2() {
	return seek2->time();
}

void TInterface::setSeeking3(int n) {
	seek3->setTime(n);
}

int TInterface::seeking3() {
	return seek3->time();
}

void TInterface::setSeeking4(int n) {
	seek4->setTime(n);
}

int TInterface::seeking4() {
	return seek4->time();
}

void TInterface::setUpdateWhileDragging(bool b) {
	if (b) 
		timeslider_behaviour_combo->setCurrentIndex(0);
	else
		timeslider_behaviour_combo->setCurrentIndex(1);
}

bool TInterface::updateWhileDragging() {
	return (timeslider_behaviour_combo->currentIndex() == 0);
}

void TInterface::setRelativeSeeking(bool b) {
	relative_seeking_button->setChecked(b);
	absolute_seeking_button->setChecked(!b);
}

bool TInterface::relativeSeeking() {
	return relative_seeking_button->isChecked();
}

void TInterface::setPreciseSeeking(bool b) {
	precise_seeking_check->setChecked(b);
}

bool TInterface::preciseSeeking() {
	return precise_seeking_check->isChecked();
}

void TInterface::setDefaultFont(QString font_desc) {
	default_font_edit->setText(font_desc);
}

QString TInterface::defaultFont() {
	return default_font_edit->text();
}

void TInterface::on_changeFontButton_clicked() {
	QFont f = qApp->font();

	if (!default_font_edit->text().isEmpty()) {
		f.fromString(default_font_edit->text());
	}

	bool ok;
	f = QFontDialog::getFont(&ok, f, this);

	if (ok) {
		default_font_edit->setText(f.toString());
	}
}

#ifdef SINGLE_INSTANCE
void TInterface::changeInstanceImages() {
	if (single_instance_check->isChecked())
		instances_icon->setPixmap(Images::icon("instance1"));
	else
		instances_icon->setPixmap(Images::icon("instance2"));
}
#endif

void TInterface::setHideVideoOnAudioFiles(bool b) {
	hide_video_window_on_audio_check->setChecked(b);
}

bool TInterface::hideVideoOnAudioFiles() {
	return hide_video_window_on_audio_check->isChecked();
}

// Floating tab
void TInterface::setFloatingWidth(int percentage) {
	floating_width_slider->setValue(percentage);
}

int TInterface::floatingWidth() {
	return floating_width_slider->value();
}

void TInterface::setRecentsMaxItems(int n) {
	recents_max_items_spin->setValue(n);
}

int TInterface::recentsMaxItems() {
	return recents_max_items_spin->value();
}

void TInterface::setURLMaxItems(int n) {
	url_max_items_spin->setValue(n);
}

int TInterface::urlMaxItems() {
	return url_max_items_spin->value();
}

void TInterface::setRememberDirs(bool b) {
	save_dirs_check->setChecked(b);
}

bool TInterface::rememberDirs() {
	return save_dirs_check->isChecked();
}

void TInterface::createHelp() {
	clearHelp();

	addSectionTitle(tr("Interface"));

	setWhatsThis(mainwindow_resize_combo, tr("Autoresize"),
        tr("The main window can be resized automatically. Select the option "
           "you prefer."));

	setWhatsThis(save_size_check, tr("Remember position and size"),
        tr("If you check this option, the position and size of the main "
           "window will be saved and restored when you run SMPlayer again."));

	setWhatsThis(hide_video_window_on_audio_check, tr("Hide video window when playing audio files"),
        tr("If this option is enabled the video window will be hidden when playing audio files."));

	setWhatsThis(language_combo, tr("Language"),
		tr("Here you can change the language of the application."));

	setWhatsThis(gui_combo, tr("GUI"),
		tr("Select the graphic interface you prefer for the application.") +"<br>"+
		tr("The <b>Basic GUI</b> provides the traditional interface, with the "
		   "toolbar and control bar.") +" "+
		tr("The <b>Skinnable GUI</b> provides an interface where several skins are available.")
		);

	setWhatsThis(iconset_combo, tr("Icon set"),
		tr("Select the icon set you prefer for the application."));

#ifdef SKINS
	setWhatsThis(skin_combo, tr("Skin"),
        tr("Select the skin you prefer for the application. Only available with the skinnable GUI."));
#endif

	setWhatsThis(style_combo, tr("Style"),
        tr("Select the style you prefer for the application."));


	setWhatsThis(changeFontButton, tr("Default font"),
        tr("You can change here the application's font."));

	addSectionTitle(tr("Seeking"));

	setWhatsThis(seek1, tr("Short jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("short jump")));

	setWhatsThis(seek2, tr("Medium jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("medium jump")));

	setWhatsThis(seek3, tr("Long jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("long jump")));

	setWhatsThis(seek4, tr("Mouse wheel jump"),
        tr("Select the time that should be go forward or backward when you "
           "move the mouse wheel."));

	setWhatsThis(timeslider_behaviour_combo, tr("Behaviour of time slider"),
        tr("Select what to do when dragging the time slider."));

	setWhatsThis(seeking_method_group, tr("Seeking method"),
		tr("Sets the method to be used when seeking with the slider. "
           "Absolute seeking may be a little bit more accurate, while "
           "relative seeking may work better with files with a wrong length."));

	setWhatsThis(precise_seeking_check, tr("Precise seeking"),
		tr("If this option is enabled, seeks are more accurate but they "
           "can be a little bit slower. May not work with some video formats.") +"<br>"+
		tr("Note: this option only works with MPlayer2"));

	setWhatsThis(reset_stop_check, tr("Pressing the stop button once resets the time position"),
		tr("By default when the stop button is pressed the time position is remembered "
           "so if you press play button the media will resume at the same point. You need "
           "to press the stop button twice to reset the time position, but if this "
           "option is checked the time position will be set to 0 with only once "
           "press of the stop button."));

#ifdef SINGLE_INSTANCE
	addSectionTitle(tr("Instances"));

	setWhatsThis(single_instance_check, 
        tr("Use only one running instance of SMPlayer"),
        tr("Check this option if you want to use an already running instance "
           "of SMPlayer when opening other files."));
#endif

	addSectionTitle(tr("Floating control"));

	setWhatsThis(floating_width_slider, tr("Width"),
		tr("Specifies the width of the control (as a percentage)."));

	setWhatsThis(floating_move_bottom_check, tr("Show only when moving the mouse to the bottom of the screen"),
		tr("If this option is checked, the floating control will only be displayed when the mouse is moved "
           "to the bottom of the screen. Otherwise the control will appear whenever the mouse is moved, no matter "
           "its position."));

	setWhatsThis(floating_hide_delay_spin, tr("Time to hide the control"),
		tr("Sets the time (in milliseconds) to hide the control after the mouse went away from the control."));

	addSectionTitle(tr("Privacy"));

	setWhatsThis(recents_max_items_spin, tr("Recent files"),
        tr("Select the maximum number of items that will be shown in the "
           "<b>Open->Recent files</b> submenu. If you set it to 0 that "
           "menu won't be shown at all."));

	setWhatsThis(url_max_items_spin, tr("Max. URLs"),
        tr("Select the maximum number of items that the <b>Open->URL</b> "
           "dialog will remember. Set it to 0 if you don't want any URL "
           "to be stored."));

	setWhatsThis(save_dirs_check, tr("Remember last directory"),
		tr("If this option is checked, SMPlayer will remember the last folder you use to open a file."));
}

}} // namespace Gui::Pref

#include "moc_interface.cpp"
