/*  umplayer, GUI front-end for mplayer.
    Copyright (C) 2010 Ori Rejwan

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

#ifndef GUI_SKIN_VOLUMECONTROLPANEL_H
#define GUI_SKIN_VOLUMECONTROLPANEL_H

#include <QWidget>
#include "gui/skin/button.h"
#include "gui/skin/panelseeker.h"

namespace Gui {
namespace Skin {


class TVolumeControlPanel : public QWidget
{
Q_OBJECT
Q_PROPERTY( QPixmap mute READ muteIcon WRITE setMuteIcon )
Q_PROPERTY( QPixmap max READ maxIcon WRITE setMaxIcon  )
Q_PROPERTY( QPixmap fullscreen READ fullscreenIcon WRITE setFullscreenIcon)
Q_PROPERTY( QPixmap playlist READ playlistIcon WRITE setPlaylistIcon)
Q_PROPERTY( QPixmap equalizer READ equalizerIcon WRITE setEqualizerIcon)
Q_PROPERTY( QPixmap volumebar READ volumebarPix WRITE setVolumebarPix)
Q_PROPERTY( QPixmap volumebarProgress READ volumebarProgressPix WRITE setVolumebarProgressPix)
Q_PROPERTY( QPixmap volumeKnob READ volumeKnobPix WRITE setVolumeKnobPix)

private:
    TButton* muteButton;
    TButton* maxButton;
    TButton* fullscreenButton;
    TButton* playlistButton;
    TButton* equalizerButton;
    TPanelSeeker* volumeBar;
    void setButtonIcons( TButton* button, QPixmap pix);

public:
	explicit TVolumeControlPanel(QWidget *parent = 0);
	virtual ~TVolumeControlPanel() {}
    QPixmap muteIcon() { return muteButton->getIcon().pixmap(TIcon::Normal, TIcon::Off); }
    QPixmap maxIcon() { return maxButton->getIcon().pixmap(TIcon::Normal, TIcon::Off); }
    QPixmap fullscreenIcon() { return fullscreenButton->getIcon().pixmap(TIcon::Normal, TIcon::Off); }
    QPixmap playlistIcon() { return playlistButton->getIcon().pixmap(TIcon::Normal, TIcon::Off); }
    QPixmap equalizerIcon() { return equalizerButton->getIcon().pixmap(TIcon::Normal, TIcon::Off); }
    QPixmap volumebarPix() { return volumeBar->centerIcon(); }
    QPixmap volumebarProgressPix() { return volumeBar->progressIcon(); }
    QPixmap volumeKnobPix() { return volumeBar->knobIcon(); }

    void setMuteIcon( QPixmap pix) { setButtonIcons(muteButton, pix);}
    void setMaxIcon( QPixmap pix) { setButtonIcons(maxButton, pix);}
    void setFullscreenIcon( QPixmap pix) {setButtonIcons(fullscreenButton, pix);}
    void setPlaylistIcon( QPixmap pix) {setButtonIcons(playlistButton, pix);}
    void setEqualizerIcon( QPixmap pix) {setButtonIcons(equalizerButton, pix);}
    void setVolumebarPix(QPixmap pix) { volumeBar->setCenterIcon(pix);}
    void setVolumebarProgressPix(QPixmap pix) { volumeBar->setProgressIcon(pix);}
    void setVolumeKnobPix(QPixmap pix) { volumeBar->setKnobIcon(pix); }
    void setActionCollection(QList<QAction*> actions);
    /* bool eventFilter(QObject *watched, QEvent *event); */

signals:
	void volumeChanged(int);
	void volumeSliderMoved(int);

public slots:
    void setVolumeMax() { volumeBar->setValue( volumeBar->maximum()); }
    void setVolumeMin() { volumeBar->setValue( volumeBar->minimum()); }
    void setVolume(int value);

protected:
    virtual void changeEvent (QEvent * event);
    virtual void retranslateStrings();
};

} // namesapce Skin
} // namespace Gui

#endif // GUI_SKIN_VOLUMECONTROLPANEL_H
