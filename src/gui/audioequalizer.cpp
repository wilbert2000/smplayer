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

#include "gui/audioequalizer.h"
#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>

#include "images.h"
#include "settings/preferences.h"
#include "gui/eqslider.h"
#include "wzdebug.h"


using namespace Settings;

namespace Gui {

TAudioEqualizer::TAudioEqualizer(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f) {

    createPresets();

    QBoxLayout *bl = new QHBoxLayout; //(0, 4, 2);

    for (int n = 0; n < 10; n++) {
        TEqSlider* slider = new TEqSlider(this);
        slider->setIcon(QPixmap());
        slider->sliderWidget()->setRange(-120, 120);
        slider->sliderWidget()->setTracking(false);
        eq[n] = slider;

        connect(slider, &TEqSlider::valueChanged,
                this, &TAudioEqualizer::updatePresetCombo);
        bl->addWidget(slider);
    }

    presets_combo = new QComboBox(this);
    connect(presets_combo, SIGNAL(activated(int)),
            this, SLOT(presetChanged(int)));

    presets_label = new QLabel("&Preset", this);
    presets_label->setBuddy(presets_combo);

    reset_button = new QPushButton("&Reset", this);
    connect(reset_button, &QPushButton::clicked,
            this, &TAudioEqualizer::reset);

    set_default_button = new QPushButton("&Set as default values", this);
    connect(set_default_button, &QPushButton::clicked,
            this, &TAudioEqualizer::setDefaults);

    apply_button = new QPushButton("&Apply", this);
    connect(apply_button, &QPushButton::clicked,
            this, &TAudioEqualizer::applyButtonClicked);

    QBoxLayout *button_layout = new QHBoxLayout; //(0, 4, 2);
    button_layout->addWidget(presets_label);
    button_layout->addWidget(presets_combo);
    button_layout->addStretch();
    button_layout->addWidget(apply_button);
    button_layout->addWidget(reset_button);
    button_layout->addWidget(set_default_button);

    QBoxLayout *layout = new QVBoxLayout(this); //, 4, 2);
    layout->addLayout(bl);
    layout->addLayout(button_layout);

    retranslateStrings();

    adjustSize();
    //setFixedSize(sizeHint());
}

void TAudioEqualizer::createPresets() {
    preset_list.clear();
    TAudioEqualizerList preset;

    // Classical
    preset.clear();
    preset << 0 << 0 << 0 << 0 << 0 << 0 << -41 << -41 << -41 << -53;
    preset_list[Classical] = preset;

    // Club
    preset.clear();
    preset << 0 << 0 << 47 << 29 << 29 << 29 << 17 << 0 << 0 << 0;
    preset_list[Club] = preset;

    // Dance
    preset.clear();
    preset << 53 << 41 << 11 << 0 << 0 << -29 << -41 << -41 << 0 << 0;
    preset_list[Dance] = preset;

    // Flat
    preset.clear();
    preset << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0 << 0;
    preset_list[Flat] = preset;

    // Fullbass
    preset.clear();
    preset << 53 << 53 << 53 << 29 << 5 << -23 << -47 << -59 << -65 << -65;
    preset_list[Fullbass] = preset;

    // FullbassTreble
    preset.clear();
    preset << 41 << 29 << 0 << -41 << -23 << 5 << 47 << 65 << 71 << 71;
    preset_list[FullbassTreble] = preset;

    // Fulltreble
    preset.clear();
    preset << -53 << -53 << -53 << -23 << 11 << 65 << 95 << 95 << 95 << 95;
    preset_list[Fulltreble] = preset;

    // Headphones
    preset.clear();
    preset << 23 << 65 << 29 << -17 << -11 << 5 << 23 << 53 << 71 << 83;
    preset_list[Headphones] = preset;

    // LargeHall
    preset.clear();
    preset << 59 << 59 << 29 << 29 << 0 << -23 << -23 << -23 << 0 << 0;
    preset_list[LargeHall] = preset;

    // Live
    preset.clear();
    preset << -23 << 0 << 23 << 29 << 29 << 29 << 23 << 11 << 11 << 11;
    preset_list[Live] = preset;

    // Party
    preset.clear();
    preset << 41 << 41 << 0 << 0 << 0 << 0 << 0 << 0 << 41 << 41;
    preset_list[Party] = preset;

    // Pop
    preset.clear();
    preset << -5 << 23 << 41 << 47 << 29 << 0 << -11 << -11 << -5 << -5;
    preset_list[Pop] = preset;

    // Reggae
    preset.clear();
    preset << 0 << 0 << 0 << -29 << 0 << 35 << 35 << 0 << 0 << 0;
    preset_list[Reggae] = preset;

    // Rock
    preset.clear();
    preset << 47 << 23 << 29 << -47 << -17 << 23 << 47 << 65 << 65 << 65;
    preset_list[Rock] = preset;

    // Ska
    preset.clear();
    preset << -11 << -23 << -23 << 0 << 23 << 29 << 47 << 53 << 65 << 53;
    preset_list[Ska] = preset;

    // Soft
    preset.clear();
    preset << 23 << 5 << 0 << -11 << 0 << 23 << 47 << 53 << 65 << 71;
    preset_list[Soft] = preset;

    // SoftRock
    preset.clear();
    preset << 23 << 23 << 11 << 0 << -23 << -29 << -17 << 0 << 11 << 47;
    preset_list[SoftRock] = preset;

    // Techno
    preset.clear();
    preset << 47 << 29 << 0 << -29 << -23 << 0 << 47 << 53 << 53 << 47;
    preset_list[Techno] = preset;
}


void TAudioEqualizer::retranslateStrings() {
    setWindowTitle(tr("Audio Equalizer"));
    setWindowIcon(Images::icon("logo"));

    eq[0]->setLabel(tr("%1 Hz").arg("31.25"));
    eq[1]->setLabel(tr("%1 Hz").arg("62.50"));
    eq[2]->setLabel(tr("%1 Hz").arg("125"));
    eq[3]->setLabel(tr("%1 Hz").arg("250"));
    eq[4]->setLabel(tr("%1 Hz").arg("500"));
    eq[5]->setLabel(tr("%1 kHz").arg("1"));
    eq[6]->setLabel(tr("%1 kHz").arg("2"));
    eq[7]->setLabel(tr("%1 kHz").arg("4"));
    eq[8]->setLabel(tr("%1 kHz").arg("8"));
    eq[9]->setLabel(tr("%1 kHz").arg("16"));

    presets_label->setText(tr("&Preset"));
    apply_button->setText(tr("&Apply"));
    reset_button->setText(tr("&Reset"));
    set_default_button->setText(tr("&Set as default values"));

    int presets_combo_index = presets_combo->currentIndex();
    if (presets_combo_index < 0) presets_combo_index = 0;
    presets_combo->clear();
    presets_combo->addItem(tr("Flat"), Flat);
    presets_combo->addItem(tr("Classical"), Classical);
    presets_combo->addItem(tr("Club"), Club);
    presets_combo->addItem(tr("Dance"), Dance);
    presets_combo->addItem(tr("Full bass"), Fullbass);
    presets_combo->addItem(tr("Full bass and treble"), FullbassTreble);
    presets_combo->addItem(tr("Full treble"), Fulltreble);
    presets_combo->addItem(tr("Headphones"), Headphones);
    presets_combo->addItem(tr("Large hall"), LargeHall);
    presets_combo->addItem(tr("Live"), Live);
    presets_combo->addItem(tr("Party"), Party);
    presets_combo->addItem(tr("Pop"), Pop);
    presets_combo->addItem(tr("Reggae"), Reggae);
    presets_combo->addItem(tr("Rock"), Rock);
    presets_combo->addItem(tr("Ska"), Ska);
    presets_combo->addItem(tr("Soft"), Soft);
    presets_combo->addItem(tr("Soft rock"), SoftRock);
    presets_combo->addItem(tr("Techno"), Techno);
    presets_combo->addItem(tr("Custom"), User_defined);
    presets_combo->setCurrentIndex(presets_combo_index);

    // What's this help:
    set_default_button->setWhatsThis(
            tr("Use the current values as default values for new videos."));

    reset_button->setWhatsThis(tr("Set all controls to zero."));

}

void TAudioEqualizer::reset() {
    setValues(preset_list[Flat]);
    presets_combo->setCurrentIndex(presets_combo->findData(Flat));
}

void TAudioEqualizer::setDefaults() {
    TAudioEqualizerList l;
    for (int n = 0; n < 10; n++) {
        l << eq[n]->value();
    }
    pref->initial_audio_equalizer = l;

    QMessageBox::information(this, tr("Information"), 
                             tr("The current values have been stored to be "
                                "used as default."));
}

void TAudioEqualizer::setEqualizer(const TAudioEqualizerList& l) {

    int p = findPreset(l);
    int index = presets_combo->findData(p);
    if (index >= 0) {
        presets_combo->setCurrentIndex(index);
    } else {
        WZWARN("preset not found");
    }
    setValues(l, false);
}

void TAudioEqualizer::setValues(const TAudioEqualizerList& eqList,
                                bool emitValuesChanged) {
    WZTRACE("");

    for (int n = 0; n < 10; n++) {
        eq[n]->blockSignals(true);
        eq[n]->setValue(eqList[n].toInt());
        eq[n]->blockSignals(false);
    }

    if (emitValuesChanged)
        emit valuesChanged(eqList);
}

void TAudioEqualizer::presetChanged(int index) {
    WZDEBUG(QString::number(index));

    int p = presets_combo->itemData(index).toInt();
    if (p != User_defined) {
        setValues(preset_list[p]);
    }
}

int TAudioEqualizer::findPreset(const TAudioEqualizerList& l) {

    QMap<int,TAudioEqualizerList>::iterator i;
    for (i = preset_list.begin(); i != preset_list.end(); ++i) {
        if (l == i.value())
            return i.key();
    }
    return User_defined;
}

void TAudioEqualizer::applyButtonClicked() {

    TAudioEqualizerList l;
    for (int n = 0; n < 10; n++) {
        l << eq[n]->value();
    }
    emit applyClicked(l);
}

void TAudioEqualizer::updatePresetCombo() {
    WZDEBUG("");

    TAudioEqualizerList l;
    for (int n = 0; n < 10; n++) {
        l << eq[n]->value();
    }

    int p = findPreset(l);
    int index = presets_combo->findData(p);
    if (index != 1) {
        presets_combo->setCurrentIndex(index);
    }
}

void TAudioEqualizer::hideEvent(QHideEvent*) {
    emit visibilityChanged(false);
}

void TAudioEqualizer::showEvent(QShowEvent*) {
    emit visibilityChanged(true);
}

// Language change stuff
void TAudioEqualizer::changeEvent(QEvent *e) {
    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QWidget::changeEvent(e);
    }
}

} // namespace Gui

#include "moc_audioequalizer.cpp"
