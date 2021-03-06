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

#include "gui/videoequalizer.h"

namespace Gui {

TVideoEqualizer::TVideoEqualizer(QWidget* parent, Qt::WindowFlags f) 
    : QWidget(parent, f)
{
    setupUi(this);

    /*
    contrast_indicator->setNum(0);
    brightness_indicator->setNum(0);
    hue_indicator->setNum(0);
    saturation_indicator->setNum(0);
    gamma_indicator->setNum(0);
    */

    connect(contrast_slider, SIGNAL(valueChanged(int)),
             contrast_indicator, SLOT(setNum(int)));

    connect(brightness_slider, SIGNAL(valueChanged(int)),
             brightness_indicator, SLOT(setNum(int)));

    connect(hue_slider, SIGNAL(valueChanged(int)),
             hue_indicator, SLOT(setNum(int)));

    connect(saturation_slider, SIGNAL(valueChanged(int)),
             saturation_indicator, SLOT(setNum(int)));

    connect(gamma_slider, SIGNAL(valueChanged(int)),
             gamma_indicator, SLOT(setNum(int)));

    // Reemit signals
    connect(contrast_slider, &QSlider::valueChanged,
             this, &TVideoEqualizer::contrastChanged);
    connect(brightness_slider, &QSlider::valueChanged,
             this, &TVideoEqualizer::brightnessChanged);
    connect(hue_slider, &QSlider::valueChanged,
             this, &TVideoEqualizer::hueChanged);
    connect(saturation_slider, &QSlider::valueChanged,
             this, &TVideoEqualizer::saturationChanged);
    connect(gamma_slider, &QSlider::valueChanged,
             this, &TVideoEqualizer::gammaChanged);

    connect(makedefault_button, &QPushButton::clicked,
             this, &TVideoEqualizer::requestToChangeDefaultValues);

    adjustSize();
}

void TVideoEqualizer::reset() {
    setContrast(0);
    setBrightness(0);
    setHue(0);
    setSaturation(0);
    setGamma(0);
}

void TVideoEqualizer::on_reset_button_clicked() {
    reset();
}

void TVideoEqualizer::on_bysoftware_check_stateChanged(int state) {
    emit bySoftwareChanged(state == Qt::Checked);
}

void TVideoEqualizer::hideEvent(QHideEvent*) {
    emit visibilityChanged(false);
}

void TVideoEqualizer::showEvent(QShowEvent*) {
    emit visibilityChanged(true);
}

void TVideoEqualizer::retranslateStrings() {
    retranslateUi(this);

    // What's this help:
    makedefault_button->setWhatsThis(
            tr("Use the current values as default values for new videos."));

    reset_button->setWhatsThis(tr("Set all controls to zero."));
}

// Language change stuff
void TVideoEqualizer::changeEvent(QEvent *e) {
    if (e->type() == QEvent::LanguageChange) {
        retranslateStrings();
    } else {
        QWidget::changeEvent(e);
    }
}

} // namespace Gui

#include "moc_videoequalizer.cpp"
