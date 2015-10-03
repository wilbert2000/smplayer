/*  Mpcgui for SMPlayer.
    Copyright (C) 2008 matt_ <matt@endboss.org>

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

#include "gui/mpc/mpc.h"

#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QLayout>
#include <QApplication>
#include <QTimer>

#include "gui/action.h"
#include "gui/widgetactions.h"
#include "gui/autohidewidget.h"

#include "styles.h"
#include "mplayerwindow.h"
#include "helper.h"
#include "desktopinfo.h"
#include "colorutils.h"


using namespace Settings;

namespace Gui {

TMpc::TMpc( QWidget * parent, Qt::WindowFlags flags )
	: TBasePlus( parent, flags )
{
	createActions();
	createControlWidget();
	createStatusBar();
	createFloatingControl();
}

TMpc::~TMpc() {
}

void TMpc::createActions() {
	timeslider_action = createTimeSliderAction(this);
	timeslider_action->disable();
    timeslider_action->setCustomStyle( new Mpc::TimeSlideStyle() );

#if USE_VOLUME_BAR
	volumeslider_action = createVolumeSliderAction(this);
	volumeslider_action->disable();
    volumeslider_action->setCustomStyle( new Mpc::VolumeSlideStyle() );
    volumeslider_action->setFixedSize( QSize(50,18) );
	volumeslider_action->setTickPosition( QSlider::NoTicks );
#endif

	time_label_action = new TTimeLabelAction(this);
	time_label_action->setObjectName("timelabel_action");

	connect( this, SIGNAL(timeChanged(QString)),
             time_label_action, SLOT(setText(QString)) );
}


void TMpc::createControlWidget() {
	controlwidget = new QToolBar( this );
	controlwidget->setObjectName("controlwidget");
	controlwidget->setLayoutDirection(Qt::LeftToRight);

	controlwidget->setMovable(false);
	controlwidget->setAllowedAreas(Qt::BottomToolBarArea);
	controlwidget->addAction(playAct);
	controlwidget->addAction(pauseAct);
	controlwidget->addAction(stopAct);
	controlwidget->addSeparator();
	controlwidget->addAction(rewind3Act);
	controlwidget->addAction(rewind1Act);
	controlwidget->addAction(forward1Act);
	controlwidget->addAction(forward3Act);
	controlwidget->addSeparator();
	controlwidget->addAction(frameStepAct);
	controlwidget->addSeparator();

	QLabel* pLabel = new QLabel(this);
	pLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	controlwidget->addWidget(pLabel);

	controlwidget->addAction(muteAct);
	controlwidget->addAction(volumeslider_action);

	timeslidewidget = new QToolBar( this );
	timeslidewidget->setObjectName("timeslidewidget");
	timeslidewidget->setLayoutDirection(Qt::LeftToRight);
	timeslidewidget->addAction(timeslider_action);
	timeslidewidget->setMovable(false);

	/*
	QColor SliderColor = palette().color(QPalette::Window);
	QColor SliderBorderColor = palette().color(QPalette::Dark);
	*/
	setIconSize( QSize( 16 , 16 ) );

	addToolBar(Qt::BottomToolBarArea, controlwidget);
	addToolBarBreak(Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, timeslidewidget);

	controlwidget->setStyle(new Mpc::ToolbarStyle() );
	timeslidewidget->setStyle(new Mpc::ToolbarStyle() );

	statusBar()->show();
}

void TMpc::createFloatingControl() {
	// Floating control
	floating_control = new TAutohideWidget(panel, mplayerwindow);
	floating_control->setAutoHide(true);
	floating_control->hide();
	spacer = new QSpacerItem(10,10);

	floating_control_time = new QLabel(floating_control);
	floating_control_time->setAlignment(Qt::AlignRight);
	floating_control_time->setAutoFillBackground(true);
	ColorUtils::setBackgroundColor( floating_control_time, QColor(0,0,0) );
	ColorUtils::setForegroundColor( floating_control_time, QColor(255,255,255) );

}

void TMpc::retranslateStrings() {
	qDebug("Gui::TMpc::retranslateStrings");

	TBasePlus::retranslateStrings();

	controlwidget->setWindowTitle( tr("Control bar") );
	timeslidewidget->setWindowTitle( tr("Seek bar") );

    setupIcons();
}

#if AUTODISABLE_ACTIONS
void TMpc::enableActionsOnPlaying() {
	TBasePlus::enableActionsOnPlaying();

	timeslider_action->enable();
#if USE_VOLUME_BAR
	volumeslider_action->enable();
#endif
}

void TMpc::disableActionsOnStop() {
	TBasePlus::disableActionsOnStop();

	timeslider_action->disable();
#if USE_VOLUME_BAR
	volumeslider_action->disable();
#endif
}
#endif // AUTODISABLE_ACTIONS

void TMpc::aboutToEnterFullscreen() {
	TBasePlus::aboutToEnterFullscreen();

	// Show floating_control
	// Move controls to the floating_control layout
	removeToolBarBreak(controlwidget);
	removeToolBar(controlwidget);
	removeToolBar(timeslidewidget);
	floating_control->layout()->addWidget(timeslidewidget);
	floating_control->layout()->addItem(spacer);
	floating_control->layout()->addWidget(controlwidget);
	floating_control->layout()->addWidget(floating_control_time);
	controlwidget->show();
	timeslidewidget->show();
	floating_control->adjustSize();

	floating_control->setMargin(pref->floating_control_margin);
	floating_control->setPercWidth(pref->floating_control_width);
	floating_control->setAnimated(pref->floating_control_animated);
	floating_control->setActivationArea( (TAutohideWidget::Activation) pref->floating_activation_area);
	floating_control->setHideDelay(pref->floating_hide_delay);
	QTimer::singleShot(100, floating_control, SLOT(activate()));


	if (!pref->compact_mode) {
		//controlwidget->hide();
		//timeslidewidget->hide();
		statusBar()->hide();
	}
}

void TMpc::aboutToExitFullscreen() {
	TBasePlus::aboutToExitFullscreen();

	// Remove controls from the floating_control and put them back to the mainwindow
	floating_control->deactivate();
	floating_control->layout()->removeWidget(controlwidget);
	floating_control->layout()->removeWidget(timeslidewidget);
	floating_control->layout()->removeItem(spacer);
	floating_control->layout()->removeWidget(floating_control_time);
	addToolBar(Qt::BottomToolBarArea, controlwidget);
	addToolBarBreak(Qt::BottomToolBarArea);
	addToolBar(Qt::BottomToolBarArea, timeslidewidget);

	if (!pref->compact_mode) {
		controlwidget->show();
		statusBar()->show();
		timeslidewidget->show();
	} else {
		controlwidget->hide();
		timeslidewidget->hide();
	}
}

void TMpc::aboutToEnterCompactMode() {
	TBasePlus::aboutToEnterCompactMode();

	controlwidget->hide();
	timeslidewidget->hide();
	statusBar()->hide();
}

void TMpc::aboutToExitCompactMode() {
	TBasePlus::aboutToExitCompactMode();

	statusBar()->show();
	controlwidget->show();
	timeslidewidget->show();
}

#if USE_mpcMUMSIZE
QSize TMpc::mpcmumSizeHint() const {
	return QSize(controlwidget->sizeHint().width(), 0);
}
#endif


void TMpc::saveConfig(const QString &group) {
	Q_UNUSED(group)

	TBasePlus::saveConfig("mpc_gui");
}

void TMpc::loadConfig(const QString &group) {
	Q_UNUSED(group)

	TBasePlus::loadConfig("mpc_gui");

	if (pref->compact_mode) {
		controlwidget->hide();
		timeslidewidget->hide();
	}
}

void TMpc::setupIcons() {
    playAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(0,0,16,16) );
    playOrPauseAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(0,0,16,16) );
    pauseAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(16,0,16,16) );
    stopAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(32,0,16,16) );
    rewind3Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(64,0,16,16) );
    rewind2Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(80,0,16,16) );
    rewind1Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(80,0,16,16) );
    forward1Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(96,0,16,16) );
    forward2Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(96,0,16,16) );
    forward3Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(112,0,16,16) );
    frameStepAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(144,0,16,16) );
    muteAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(192,0,16,16) );

    pauseAct->setCheckable(true);
    playAct->setCheckable(true);
    stopAct->setCheckable(true);
	connect( muteAct, SIGNAL(toggled(bool)),
             this, SLOT(muteIconChange(bool)) );

	connect( core , SIGNAL(mediaInfoChanged()),
             this, SLOT(updateAudioChannels()) );

    connect( core , SIGNAL(stateChanged(Core::State)),
             this, SLOT(iconChange(Core::State)) );
}

void TMpc::iconChange(Core::State state) {
    playAct->blockSignals(true);
    pauseAct->blockSignals(true);
    stopAct->blockSignals(true);

    if( state == Core::Paused )
    {
        playAct->setChecked(false);
        pauseAct->setChecked(true);
        stopAct->setChecked(false);
    }
    if( state == Core::Playing )
    {
        playAct->setChecked(true);
        pauseAct->setChecked(false);
        stopAct->setChecked(false);
    }
    if( state == Core::Stopped )
    {
        playAct->setChecked(false);
        pauseAct->setChecked(false);
        stopAct->setChecked(false);
    }

    playAct->blockSignals(false);
    pauseAct->blockSignals(false);
    stopAct->blockSignals(false);
}

void TMpc::muteIconChange(bool b) {
    if( sender() == muteAct )
    {
        if(!b) {
            muteAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(192,0,16,16) );
        } else {
            muteAct->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(208,0,16,16) );
        }
    }

}


void TMpc::createStatusBar() {

    // remove frames around statusbar items
    statusBar()->setStyleSheet("QStatusBar::item { border: 0px solid black }; ");

    // emulate mono/stereo display from mpc
    audiochannel_display = new QLabel( statusBar() );
    audiochannel_display->setContentsMargins(0,0,0,0);
    audiochannel_display->setAlignment(Qt::AlignRight);
    audiochannel_display->setPixmap( QPixmap(":/gui/mpc/mpc_stereo.png") );
    audiochannel_display->setMinimumSize(audiochannel_display->sizeHint());
    audiochannel_display->setMaximumSize(audiochannel_display->sizeHint());
    audiochannel_display->setPixmap( QPixmap("") );
    
	time_display = new QLabel( statusBar() );
	time_display->setAlignment(Qt::AlignRight);
	time_display->setText(" 88:88:88 / 88:88:88 ");
	time_display->setMinimumSize(time_display->sizeHint());
    time_display->setContentsMargins(15,2,1,1);

	frame_display = new QLabel( statusBar() );
	frame_display->setAlignment(Qt::AlignRight);
	frame_display->setText("88888888");
	frame_display->setMinimumSize(frame_display->sizeHint());
    frame_display->setContentsMargins(15,2,1,1);

	statusBar()->setAutoFillBackground(TRUE);   

	ColorUtils::setBackgroundColor( statusBar(), QColor(0,0,0) );
	ColorUtils::setForegroundColor( statusBar(), QColor(255,255,255) );
	ColorUtils::setBackgroundColor( time_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( time_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( frame_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( frame_display, QColor(255,255,255) );
	ColorUtils::setBackgroundColor( audiochannel_display, QColor(0,0,0) );
	ColorUtils::setForegroundColor( audiochannel_display, QColor(255,255,255) );
	statusBar()->setSizeGripEnabled(FALSE);

    

	statusBar()->addPermanentWidget( frame_display, 0 );
	frame_display->setText( "0" );

    statusBar()->addPermanentWidget( time_display, 0 );
	time_display->setText(" 00:00:00 / 00:00:00 ");

    statusBar()->addPermanentWidget( audiochannel_display, 0 );

	time_display->show();
	frame_display->hide();

	connect( this, SIGNAL(timeChanged(QString)),
             this, SLOT(displayTime(QString)) );

	connect( this, SIGNAL(frameChanged(int)),
             this, SLOT(displayFrame(int)) );
}

void TMpc::displayTime(QString text) {
	time_display->setText( text );
	time_label_action->setText(text );
	floating_control_time->setText(text);
}

void TMpc::displayFrame(int frame) {
	if (frame_display->isVisible()) {
		frame_display->setNum( frame );
	}
}

void TMpc::updateAudioChannels() {
    if( core->mdat.audio_nch == 1 ) {
        audiochannel_display->setPixmap( QPixmap(":/gui/mpc/mpc_mono.png") );
    }
    else {
        audiochannel_display->setPixmap( QPixmap(":/gui/mpc/mpc_stereo.png") );
    }
}

void TMpc::showFullscreenControls() {

    if(pref->fullscreen && controlwidget->isHidden() && timeslidewidget->isHidden() && 
        !pref->compact_mode )
    {
	    controlwidget->show();
        timeslidewidget->show();
        statusBar()->show();
    }
}

void TMpc::hideFullscreenControls() {

    if(pref->fullscreen && controlwidget->isVisible() && timeslidewidget->isVisible() )
    {
        controlwidget->hide();
        timeslidewidget->hide();
        statusBar()->hide();
    }
}

void TMpc::setJumpTexts() {
	rewind1Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking1)) );
	rewind2Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking2)) );
	rewind3Act->change( tr("-%1").arg(Helper::timeForJumps(pref->seeking3)) );

	forward1Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking1)) );
	forward2Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking2)) );
	forward3Act->change( tr("+%1").arg(Helper::timeForJumps(pref->seeking3)) );

	/*
	if (qApp->isLeftToRight()) {
	*/
        rewind1Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(80,0,16,16) );
        rewind2Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(80,0,16,16) );
        rewind3Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(64,0,16,16) );

        forward1Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(96,0,16,16) );
        forward2Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(96,0,16,16) );
        forward3Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(112,0,16,16) );
	/*
	} else {
        rewind1Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(96,0,16,16) );
        rewind2Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(96,0,16,16) );
        rewind3Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(112,0,16,16) );

        forward1Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(80,0,16,16) );
        forward2Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(80,0,16,16) );
        forward3Act->setIcon( QPixmap(":/gui/mpc/mpc_toolbar.png").copy(64,0,16,16) );
	}
	*/
}

void TMpc::updateWidgets() {

	TBasePlus::updateWidgets();

	// Frame counter
	/* frame_display->setVisible( pref->show_frame_counter ); */
}

} // namespace Gui

#include "moc_mpc.cpp"
