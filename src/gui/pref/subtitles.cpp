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


#include "gui/pref/subtitles.h"
#include <QDebug>
#include <QInputDialog>
#include "images.h"
#include "settings/preferences.h"
#include "settings/paths.h"
#include "settings/assstyles.h"
#include "filedialog.h"
#include "languages.h"


namespace Gui {
namespace Pref {

TSubtitles::TSubtitles(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f) {

	setupUi(this);

	ass_subs->setEnabled(false);

	connect(ass_custom_check, SIGNAL(toggled(bool)),
            ass_subs, SLOT(setEnabled(bool)));

	/*
	connect(use_ass_check, SIGNAL(toggled(bool)),
            tab2, SLOT(setEnabled(bool)));
	*/

	connect(style_border_style_combo, SIGNAL(currentIndexChanged(int)),
             this, SLOT(checkBorderStyleCombo(int)));

#ifndef Q_OS_WIN
	windowsfontdir_check->hide();
#endif

	retranslateStrings();
}

TSubtitles::~TSubtitles() {
}

QString TSubtitles::sectionName() {
	return tr("Subtitles");
}

QPixmap TSubtitles::sectionIcon() {
    return Images::icon("pref_subtitles", 22);
}

void TSubtitles::retranslateStrings() {

	int autoload = autoload_combo->currentIndex();
	retranslateUi(this);
	autoload_combo->setCurrentIndex(autoload);

	// Subtitle encoding language
	QString current = encaLang();
	enca_lang_combo->clear();
	QMap<QString,QString> l = Languages::list();
	QMapIterator<QString, QString> i(l);
	while (i.hasNext()) {
		i.next();
		enca_lang_combo->addItem(i.value() + " (" + i.key() + ")", i.key());
	}
	enca_lang_combo->model()->sort(0);
	enca_lang_combo->insertItem(0, tr("Auto detect"), "");
	setEncaLang(current);

	// Fallback encoding code page
	current = encodingFallback();
	encoding_fallback_combo->clear();
	l = Languages::encodings();
	i = l;
	while (i.hasNext()) {
		i.next();
		encoding_fallback_combo->addItem(i.value() + " (" + i.key() + ")", i.key());
	}
	encoding_fallback_combo->model()->sort(0);
	encoding_fallback_combo->insertItem(0, tr("Auto detect"), "");
	setEncodingFallback(current);

	// Ass styles
	using namespace Settings;
	int current_idx = style_alignment_combo->currentIndex();
	style_alignment_combo->clear();
	style_alignment_combo->addItem(tr("Left", "horizontal alignment"), TAssStyles::Left);
	style_alignment_combo->addItem(tr("Centered", "horizontal alignment"), TAssStyles::HCenter);
	style_alignment_combo->addItem(tr("Right", "horizontal alignment"), TAssStyles::Right);
	style_alignment_combo->setCurrentIndex(current_idx);

	current_idx = style_valignment_combo->currentIndex();
	style_valignment_combo->clear();
	style_valignment_combo->addItem(tr("Bottom", "vertical alignment"), TAssStyles::Bottom);
	style_valignment_combo->addItem(tr("Middle", "vertical alignment"), TAssStyles::VCenter);
	style_valignment_combo->addItem(tr("Top", "vertical alignment"), TAssStyles::Top);
	style_valignment_combo->setCurrentIndex(current_idx);

	current_idx = style_border_style_combo->currentIndex();
	style_border_style_combo->clear();
	style_border_style_combo->addItem(tr("Outline", "border style"), TAssStyles::Outline);
	style_border_style_combo->addItem(tr("Opaque box", "border style"), TAssStyles::Opaque);
	style_border_style_combo->setCurrentIndex(current_idx);

	createHelp();
}

void TSubtitles::setData(Settings::TPreferences* pref) {

	setFuzziness(pref->subtitle_fuzziness);
	setSubtitleLanguage(pref->subtitle_language);
	setSelectFirstSubtitle(pref->select_first_subtitle);

	setEncaLang(pref->subtitle_enca_language);
	setEncodingFallback(pref->subtitle_encoding_fallback);

	setAssFontScale(pref->initial_sub_scale_ass);
	setAssLineSpacing(pref->ass_line_spacing);
	setSubtitlesOnScreenshots(pref->subtitles_on_screenshots);
	setFreetypeSupport(pref->freetype_support);
	use_ass_check->setChecked(pref->use_ass_subtitles);

	// Load ass styles
	style_font_combo->setCurrentText(pref->ass_styles.fontname);
	style_size_spin->setValue(pref->ass_styles.fontsize);
	style_text_color_button->setColor(pref->ass_styles.primarycolor);
	style_border_color_button->setColor(pref->ass_styles.outlinecolor);
	style_shadow_color_button->setColor(pref->ass_styles.backcolor);
	style_bold_check->setChecked(pref->ass_styles.bold);
	style_italic_check->setChecked(pref->ass_styles.italic);
	style_alignment_combo->setCurrentIndex(style_alignment_combo->findData(pref->ass_styles.halignment));
	style_valignment_combo->setCurrentIndex(pref->ass_styles.valignment);
	style_border_style_combo->setCurrentIndex(style_border_style_combo->findData(pref->ass_styles.borderstyle));
	style_outline_spin->setValue(pref->ass_styles.outline);
	style_shadow_spin->setValue(pref->ass_styles.shadow);
	style_marginl_spin->setValue(pref->ass_styles.marginl);
	style_marginr_spin->setValue(pref->ass_styles.marginr);
	style_marginv_spin->setValue(pref->ass_styles.marginv);

	setForceAssStyles(pref->force_ass_styles);
	setCustomizedAssStyle(pref->user_forced_ass_style);

	ass_custom_check->setChecked(pref->enable_ass_styles);

#ifdef Q_OS_WIN
	windowsfontdir_check->setChecked(pref->use_windowsfontdir);
	if (!windowsfontdir_check->isChecked()) on_windowsfontdir_check_toggled(false);
#endif
}

void TSubtitles::getData(Settings::TPreferences* pref) {
	requires_restart = false;

	restartIfIntChanged(pref->subtitle_fuzziness, fuzziness());
	pref->subtitle_language = subtitleLanguage();
	restartIfBoolChanged(pref->select_first_subtitle, selectFirstSubtitle());

	restartIfStringChanged(pref->subtitle_enca_language, encaLang());
	restartIfStringChanged(pref->subtitle_encoding_fallback, encodingFallback());

	pref->initial_sub_scale_ass = assFontScale();
	restartIfIntChanged(pref->ass_line_spacing, assLineSpacing());
	restartIfBoolChanged(pref->subtitles_on_screenshots, subtitlesOnScreenshots());
	restartIfBoolChanged(pref->freetype_support, freetypeSupport());
	restartIfBoolChanged(pref->use_ass_subtitles, use_ass_check->isChecked());

	// Save ass styles
	restartIfStringChanged(pref->ass_styles.fontname, style_font_combo->currentText());
	restartIfIntChanged(pref->ass_styles.fontsize, style_size_spin->value());
	restartIfUIntChanged(pref->ass_styles.primarycolor, style_text_color_button->color().rgb());
	restartIfUIntChanged(pref->ass_styles.outlinecolor, style_border_color_button->color().rgb());
	restartIfUIntChanged(pref->ass_styles.backcolor, style_shadow_color_button->color().rgb());
	restartIfBoolChanged(pref->ass_styles.bold, style_bold_check->isChecked());
	restartIfBoolChanged(pref->ass_styles.italic, style_italic_check->isChecked());
	restartIfIntChanged(pref->ass_styles.halignment, style_alignment_combo->itemData(style_alignment_combo->currentIndex()).toInt());
	restartIfIntChanged(pref->ass_styles.valignment, style_valignment_combo->currentIndex());
	restartIfIntChanged(pref->ass_styles.borderstyle, style_border_style_combo->itemData(style_border_style_combo->currentIndex()).toInt());
	restartIfDoubleChanged(pref->ass_styles.outline, style_outline_spin->value());
	restartIfDoubleChanged(pref->ass_styles.shadow, style_shadow_spin->value());
	restartIfIntChanged(pref->ass_styles.marginl, style_marginl_spin->value());
	restartIfIntChanged(pref->ass_styles.marginr, style_marginr_spin->value());
	restartIfIntChanged(pref->ass_styles.marginv, style_marginv_spin->value());

	pref->ass_styles.exportStyles(Settings::TPaths::subtitleStyleFile());

	restartIfBoolChanged(pref->force_ass_styles, forceAssStyles());
	restartIfStringChanged(pref->user_forced_ass_style, customizedAssStyle());

	restartIfBoolChanged(pref->enable_ass_styles, ass_custom_check->isChecked());

#ifdef Q_OS_WIN
	pref->use_windowsfontdir = windowsfontdir_check->isChecked();
#endif
}

void TSubtitles::checkBorderStyleCombo(int index) {
	bool b = (index == 0);
	style_outline_spin->setEnabled(b);
	style_shadow_spin->setEnabled(b);
	style_outline_label->setEnabled(b);
	style_shadow_label->setEnabled(b);
}

void TSubtitles::setFuzziness(int n) {
	autoload_combo->setCurrentIndex(n);
}

int TSubtitles::fuzziness() {
	return autoload_combo->currentIndex();
}

void TSubtitles::setSubtitleLanguage(const QString& lang) {
	language_edit->setText(lang);
}

QString TSubtitles::subtitleLanguage() {
	return language_edit->text();
}

void TSubtitles::setSelectFirstSubtitle(bool v) {
	select_first_subtitle_check->setChecked(v);
}

bool TSubtitles::selectFirstSubtitle() {
	return select_first_subtitle_check->isChecked();
}

void TSubtitles::setEncaLang(const QString& s) {

	int i = enca_lang_combo->findData(s);
	if (i < 0) {
		qWarning() << "Gui::Pref::TSubtitles::setEncaLang: encoding lannguage"
				   << s << "not found, falling back to auto detect";
		i = enca_lang_combo->findData("");
	}
	enca_lang_combo->setCurrentIndex(i);
}

QString TSubtitles::encaLang() {
	int index = enca_lang_combo->currentIndex();
	return enca_lang_combo->itemData(index).toString();
}

void TSubtitles::setEncodingFallback(const QString& s) {

	int i = encoding_fallback_combo->findData(s);
	if (i < 0) {
		qWarning() << "Gui::Pref::TSubtitles::setEncodingFallback: encoding"
				   << s << "not found, falling back to auto detect";
		i = encoding_fallback_combo->findData("");
	}
	encoding_fallback_combo->setCurrentIndex(i);
}

QString TSubtitles::encodingFallback() {

	int index = encoding_fallback_combo->currentIndex();
	return encoding_fallback_combo->itemData(index).toString();
}

void TSubtitles::setSubtitlesOnScreenshots(bool b) {
	subtitles_on_screeshots_check->setChecked(b);
}

bool TSubtitles::subtitlesOnScreenshots() {
	return subtitles_on_screeshots_check->isChecked();
}

void TSubtitles::setAssFontScale(double n) {
	ass_font_scale_spin->setValue(n);
}

double TSubtitles::assFontScale() {
	return ass_font_scale_spin->value();
}

void TSubtitles::setAssLineSpacing(int spacing) {
	ass_line_spacing_spin->setValue(spacing);
}

int TSubtitles::assLineSpacing() {
	return ass_line_spacing_spin->value();
}

void TSubtitles::setForceAssStyles(bool b) {
	force_ass_styles->setChecked(b);
}

bool TSubtitles::forceAssStyles() {
	return force_ass_styles->isChecked();
}

/*
void TSubtitles::on_ass_subs_button_toggled(bool b) {
	if (b)
		stackedWidget->setCurrentIndex(1);
	 else 
		stackedWidget->setCurrentIndex(0);
}
*/

void TSubtitles::on_ass_customize_button_clicked() {
	bool ok;

	QString edit = forced_ass_style;

	// A copy with the current values in the dialog
	Settings::TAssStyles ass_styles;
	ass_styles.fontname = style_font_combo->currentText();
	ass_styles.fontsize = style_size_spin->value();
	ass_styles.primarycolor = style_text_color_button->color().rgb();
	ass_styles.outlinecolor = style_border_color_button->color().rgb();
	ass_styles.backcolor = style_shadow_color_button->color().rgb();
	ass_styles.bold = style_bold_check->isChecked();
	ass_styles.italic = style_italic_check->isChecked();
	ass_styles.halignment = style_alignment_combo->itemData(style_alignment_combo->currentIndex()).toInt();
	ass_styles.valignment = style_valignment_combo->currentIndex();
	ass_styles.borderstyle = style_border_style_combo->itemData(style_border_style_combo->currentIndex()).toInt();
	ass_styles.outline = style_outline_spin->value();
	ass_styles.shadow = style_shadow_spin->value();
	ass_styles.marginl = style_marginl_spin->value();
	ass_styles.marginr = style_marginr_spin->value();
	ass_styles.marginv = style_marginv_spin->value();

	if (edit.isEmpty()) {
		edit = ass_styles.toString();
	}

	QString s = QInputDialog::getText(this, tr("Customize SSA/ASS style"),
                                      tr("Here you can enter your customized SSA/ASS style.") +"<br>"+
                                      tr("Clear the edit line to disable the customized style."), 
                                      QLineEdit::Normal, 
                                      edit, &ok);
	if (ok) {
		if (s == ass_styles.toString()) s.clear(); // Clear string if it wasn't changed by the user
		setCustomizedAssStyle(s);
	}
}

void TSubtitles::setFreetypeSupport(bool b) {
	freetype_check->setChecked(b);
}

bool TSubtitles::freetypeSupport() {
	return freetype_check->isChecked();
}

void TSubtitles::on_freetype_check_toggled(bool b) {
	qDebug("Gui::Pref::TSubtitles:on_freetype_check_toggled: %d", b);
}

void TSubtitles::on_windowsfontdir_check_toggled(bool b) {
	qDebug("Gui::Pref::TSubtitles::on_windowsfontdir_check_toggled: %d", b);

#ifdef Q_OS_WIN
	if (b) {
		style_font_combo->setFontsFromDir(QString::null);
	} else {
		QString fontdir = TPaths::fontPath();
		//QString fontdir = "/tmp/fonts/";
		style_font_combo->setFontsFromDir(fontdir);

		// Calling setFontsFromDir resets the fonts in other comboboxes!
		// So the font list is copied from the previous combobox
		/*
		QString current_text = fontCombo->currentText();
		fontCombo->clear();
		for (int n=0; n < style_font_combo->count(); n++) {
			fontCombo->addItem(style_font_combo->itemText(n));
		}
		fontCombo->setCurrentText(current_text);
		*/
	}
#endif
}

void TSubtitles::createHelp() {
	clearHelp();

	addSectionTitle(tr("Subtitles"));

	setWhatsThis(autoload_combo, tr("Autoload"),
        tr("Select the subtitle autoload method."));

	setWhatsThis(language_edit, tr("Preferred subtitle language"),
		tr("Here you can type your preferred language for the subtitle stream. "
		   "When a media with multiple subtitle streams is found, SMPlayer will "
		   "try to use your preferred language.<br>"
		   "This only will work with media that offer info about the language "
		   "of the subtitle streams, like DVDs or mkv files.<br>"
		   "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
		   "will select the subtitle stream if it matches with <i>es</i>, "
		   "<i>esp</i> or <i>spa</i>."));

	setWhatsThis(select_first_subtitle_check, tr("Select first available subtitle"),
        tr("If there are one or more subtitle tracks available, one of them "
           "will be automatically selected, usually the first one, although if "
           "one of them matches the user's preferred language that one will "
           "be used instead."));

	// TODO:
	setWhatsThis(enca_lang_combo, tr("Subtitle language"),
		tr("Select the language for which you want the encoding to be guessed "
		   "automatically."));

	// TODO:
	setWhatsThis(encoding_fallback_combo, tr("Default subtitle encoding"),
        tr("Select the encoding which will be used for subtitle files "
           "by default."));

	setWhatsThis(subtitles_on_screeshots_check, 
        tr("Include subtitles on screenshots"), 
        tr("If this option is checked, the subtitles will appear in the "
           "screenshots. <b>Note:</b> it may cause some troubles sometimes."));

	setWhatsThis(use_ass_check, tr("Use the ASS library"),
		tr("This option enables the ASS library, which allows to display "
           "subtitles with multiple colors, fonts..."));

	setWhatsThis(freetype_check, tr("Freetype support"), 
		tr("You should normally not disable this option. Do it only if your "
           "MPlayer is compiled without freetype support. "
           "<b>Disabling this option could make subtitles not to work "
           "at all!</b>"));

#ifdef Q_OS_WIN
	setWhatsThis(windowsfontdir_check, tr("Enable Windows fonts"), 
		tr("If this option is enabled the Windows system fonts will be "
           "available for subtitles. There's an inconvenience: a font cache have "
           "to be created which can take some time.") +"<br>"+
		tr("If this option is not checked then only a few fonts bundled with SMPlayer "
           "can be used, but this is faster."));
#endif

	addSectionTitle(tr("Font"));

	QString scale_note = tr("This option does NOT change the size of the "
           "subtitles in the current video. To do so, use the options "
           "<i>Size+</i> and <i>Size-</i> in the subtitles menu.");

	setWhatsThis(ass_font_scale_spin, tr("Default scale"),
		tr("This option specifies the default font scale for SSA/ASS "
           "subtitles which will be used for new opened files.") +"<br>"+
		scale_note);

	setWhatsThis(ass_line_spacing_spin, tr("Line spacing"),
		tr("This specifies the spacing that will be used to separate "
           "multiple lines. It can have negative values."));

	setWhatsThis(styles_container, tr("SSA/ASS style"), 
		tr("The following options allows you to define the style to "
           "be used for non-styled subtitles (srt, sub...)."));
       
	setWhatsThis(style_font_combo, tr("Font"), 
		tr("Select the font for the subtitles."));

	setWhatsThis(style_size_spin, tr("Size"), 
		tr("The size in pixels."));

	setWhatsThis(style_bold_check, tr("Bold"), 
		tr("If checked, the text will be displayed in <b>bold</b>.")); 

	setWhatsThis(style_italic_check, tr("Italic"), 
		tr("If checked, the text will be displayed in <i>italic</i>.")); 

	setWhatsThis(style_text_color_button, tr("Text color"), 
        tr("Select the color for the text of the subtitles."));

	setWhatsThis(style_border_color_button, tr("Border color"), 
        tr("Select the color for the border of the subtitles."));

	setWhatsThis(style_shadow_color_button, tr("Shadow color"), 
        tr("This color will be used for the shadow of the subtitles."));

	setWhatsThis(style_marginl_spin, tr("Left margin"), 
        tr("Specifies the left margin in pixels."));

	setWhatsThis(style_marginr_spin, tr("Right margin"), 
        tr("Specifies the right margin in pixels."));

	setWhatsThis(style_marginv_spin, tr("Vertical margin"), 
        tr("Specifies the vertical margin in pixels."));

	setWhatsThis(style_alignment_combo, tr("Horizontal alignment"), 
        tr("Specifies the horizontal alignment. Possible values are "
           "left, centered and right."));

	setWhatsThis(style_valignment_combo, tr("Vertical alignment"), 
        tr("Specifies the vertical alignment. Possible values: "
           "bottom, middle and top."));

	setWhatsThis(style_border_style_combo, tr("Border style"), 
        tr("Specifies the border style. Possible values: outline "
           "and opaque box."));

	setWhatsThis(style_outline_spin, tr("Outline"), 
        tr("If border style is set to <i>outline</i>, this option specifies "
           "the width of the outline around the text in pixels."));

	setWhatsThis(style_shadow_spin, tr("Shadow"), 
        tr("If border style is set to <i>outline</i>, this option specifies "
           "the depth of the drop shadow behind the text in pixels."));

	setWhatsThis(force_ass_styles, tr("Apply style to ASS files too"), 
        tr("If this option is checked, the style defined above will be "
           "applied to ass subtitles too."));
}

}} // namespace Gui::Pref

#include "moc_subtitles.cpp"
