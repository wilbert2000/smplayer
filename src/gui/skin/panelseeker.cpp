/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>
    umplayer, Copyright (C) 2010 Ori Rejwan

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

#include "gui/skin/panelseeker.h"

#include <QPainter>
#include <QSlider>
#include <QMouseEvent>
#include <QDebug>
#include <QToolTip>

#include "config.h"
#include "settings/preferences.h"
#include "helper.h"


using namespace Settings;

namespace Gui {
namespace Skin {

TPanelSeeker::TPanelSeeker(QWidget *parent) :
    QAbstractSlider(parent), isPressed(false), leftRightMargin(5), bufferingPixShift(0),
    frozen(false), delayPeriod(100), frozenPeriod(500)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_Hover, true);    
    setEnabled(false);
    setState(Stopped, true);     
    setTracking(pref->update_while_seeking);
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(moved(int)));
    setRange(0, 100);
    freezeTimer = new QTimer(this);
    freezeTimer->setInterval(frozenPeriod);
    freezeTimer->setSingleShot(true);
    dragDelayTimer = new QTimer(this);
    dragDelayTimer->setInterval(delayPeriod);
    dragDelayTimer->setSingleShot(true);
    connect(freezeTimer, SIGNAL(timeout()), this, SLOT(stopFreeze()));
    connect(dragDelayTimer, SIGNAL(timeout()), this, SLOT(goToSliderPosition()));
}


void TPanelSeeker::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.drawPixmap( leftRightMargin, (height()- leftPix.height())/2 , leftPix  );
    p.drawTiledPixmap(QRect(leftRightMargin + leftPix.width(), (height()- centerPix.height())/2   ,width() - 2*leftRightMargin - leftPix.width() - rightPix.width(),centerPix.height()), centerPix);

    p.drawPixmap( width()-rightPix.width() - leftRightMargin , (height()- rightPix.height())/2 ,  rightPix);

    if(state.testFlag(Buffering))
    {
        p.drawTiledPixmap(QRect(leftRightMargin + leftPix.width(), (height()- bufferingPix.height())/2, width() - 2*leftRightMargin -leftPix.width() - rightPix.width(), bufferingPix.height()), bufferingPix, QPoint(bufferingPixShift, 0)  );
    }
    else
    {
        if(isEnabled())
        {
            p.drawTiledPixmap(QRect(leftRightMargin + leftPix.width(), (height()- progressPix.height())/2 , knobRect.center().x() - leftRightMargin - leftPix.width(), progressPix.height()), progressPix);
        }
        p.drawPixmap(knobRect.toRect(), knobCurrentPix  );
    }


}

void TPanelSeeker::setKnobIcon( QPixmap pix )
{
    int w = pix.width();
    int h = pix.height();
    knobPix.setPixmap(pix.copy(0, 0, w, h/4 ), TIcon::Normal, TIcon::Off);
    knobPix.setPixmap(pix.copy(0, h/4, w, h/4 ), TIcon::MouseOver, TIcon::Off);
    knobPix.setPixmap(pix.copy(0, h/2, w, h/4 ), TIcon::MouseDown, TIcon::Off);
    knobPix.setPixmap(pix.copy(0, 3*h/4, w, h/4 ), TIcon::Disabled, TIcon::Off);
    knobCurrentPix = knobPix.pixmap(TIcon::Normal, TIcon::Off);
    /* setSliderValue(minimum()); */
    setState(Normal, true);
}

/*
void TPanelSeeker::setSingleKnobIcon(QPixmap pix)
{
    knobPix.setPixmap(pix, TIcon::Normal, TIcon::Off);
    knobPix.setPixmap(pix, TIcon::MouseOver, TIcon::Off);
    knobPix.setPixmap(pix, TIcon::MouseDown, TIcon::Off);
    knobPix.setPixmap(pix, TIcon::Disabled, TIcon::Off);
    setSliderValue(minimum());
    setState(Normal, true);
}
*/

void TPanelSeeker::mousePressEvent(QMouseEvent *m)
{
    m->accept();
    setTracking(pref->update_while_seeking);
    if(m->button() == Qt::LeftButton)
    {
        #if QT_VERSION >= 0x050000
        QPointF pos = m->localPos();
        #else
        QPointF pos = m->posF();
        #endif
        if(knobRect.contains(pos))
        {
            isPressed = true;
            mousePressPos = pos;
            mousePressDifference = pos.x() - knobRect.center().x();
            setSliderDown(true);
            setState(Pressed, true);
        }
        else
        {
            isPressed = false;
            #if QT_VERSION >= 0x050000
            knobAdjust( m->localPos().x() - knobRect.center().x(), true);
            #else
            knobAdjust( m->posF().x() - knobRect.center().x(), true);
            #endif
        }
    }
}

void TPanelSeeker::mouseMoveEvent(QMouseEvent *m)
{
    m->accept();
    if(isPressed)
    {
        #if QT_VERSION >= 0x050000
        knobAdjust(m->localPos().x() - knobRect.center().x() - mousePressDifference );
        #else
        knobAdjust(m->posF().x() - knobRect.center().x() - mousePressDifference );
        #endif
    }
}

void TPanelSeeker::mouseReleaseEvent(QMouseEvent *m)
{    
    setSliderDown(false);        
    if(isPressed)
    {
        isPressed = false;
        setState(Pressed, false);
        frozen = true;
        freezeTimer->start();
        /*if(mousePressPos == m->pos())
        {            
            knobAdjust( m->posF().x() - knobRect.center().x(), true);
            triggerAction(SliderMove);
        } */
    }
}

void TPanelSeeker::resetKnob( bool start)
{
    if(start)
    {
        moved(minimum());
    }
    else
    {
        moved(maximum());
    }    
}

void TPanelSeeker::knobAdjust(qreal x, bool isSetValue)
{    
    if(state.testFlag(Buffering)) return;
    qreal value = minimum() + (knobRect.center().x() - (leftRightMargin + knobCurrentPix.width()/2) +x ) * (maximum() - minimum()) /(width() - (leftRightMargin) - (leftRightMargin) - knobCurrentPix.width()) ;
    if(isSetValue)
    {
        frozen = true;
        if( state.testFlag(Stopped) ) value = minimum();
        setValue(qRound(value));
        freezeTimer->start();
    }
    else {
        if(hasTracking())
        {
            blockSignals(true);
            moved(qRound(value));
            blockSignals(false);
            emit sliderMoved(qRound(value));
            if(!dragDelayTimer->isActive())
                dragDelayTimer->start();
        }
        else
            moved(qRound(value));
    }
}

bool TPanelSeeker::event(QEvent *e)
{    
    if(e->type() == QEvent::HoverMove || e->type() == QEvent::HoverEnter  )
    {
        QHoverEvent* he = static_cast<QHoverEvent*>(e);
        if( knobRect.contains(he->pos()) )
        {
            setState(Hovered, true);
        }
        else
        {
            setState(Hovered, false);
        }
    }
    if(e->type() == QEvent::HoverLeave)
    {        
        setState(Hovered, false);
    }
    return QAbstractSlider::event(e);
}

qreal TPanelSeeker::valueForPos(int pos)
{
        qreal value = (qreal)( pos - (leftRightMargin + knobCurrentPix.width()/2) ) * maximum() /(width() - (leftRightMargin) - (leftRightMargin) - knobCurrentPix.width());
        return value;
}

void TPanelSeeker::setState(State st, bool on)
{
    if(on)
    {
        if(state.testFlag(st)) return;
        state |= st;        
    }
    else
    {
        if(!state.testFlag(st)) return;
        state &= ~st;
    }
    if(state.testFlag(Buffering))
    {
        startTimer(100);
    }
    if(state.testFlag(Disabled))
    {
        knobCurrentPix = knobPix.pixmap(TIcon::Disabled, TIcon::Off);
    }
    else if(state.testFlag(Pressed))
    {
        knobCurrentPix = knobPix.pixmap(TIcon::MouseDown, TIcon::Off);
    }
    else if(state.testFlag(Hovered))
    {
        knobCurrentPix = knobPix.pixmap(TIcon::MouseOver, TIcon::Off);
    }
    else
    {
        knobCurrentPix = knobPix.pixmap(TIcon::Normal, TIcon::Off);
    }
    update();
}

void TPanelSeeker::moved( int value)
{
    if(value > maximum()) value = maximum();
    if(value < minimum()) value = minimum();
    if( state.testFlag(Stopped) ) value = minimum();

    qreal ratio =  (qreal)(value - minimum())/(maximum()-minimum());
    qreal centerPixel = ratio*(width() - (leftRightMargin ) - (leftRightMargin) - knobCurrentPix.width());
    QSize size = knobPix.size(TIcon::Normal, TIcon::Off);
    knobRect = QRectF(QPointF(centerPixel + (leftRightMargin + knobCurrentPix.width()/2) - size.width()/2, ( height() - size.height())/2 ), size );
    setSliderPosition(value);
    update();
}

void TPanelSeeker::resizeEvent(QResizeEvent *)
{
    setSliderValue(value());
}

void TPanelSeeker::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::EnabledChange)
    {
        if(isEnabled())
        {
            setState(Disabled, false);
            setState(Stopped, false);

        }
        else
        {
            setState(Disabled, true);
            setState(Stopped, true);
        }
    }    
}

void TPanelSeeker::timerEvent(QTimerEvent *t)
{
    if (bufferingPix.width() < 1) return;

    if(!state.testFlag(Buffering))
    {
        killTimer(t->timerId());
        bufferingPixShift = 0;
    }
    else
    {
        bufferingPixShift +=1;
        bufferingPixShift = qRound(bufferingPixShift)% bufferingPix.width();
        update();
    }
}

void TPanelSeeker::setSliderValue(int value)
{
    if(!isPressed && !frozen)
    {
        blockSignals(true);
        moved(value);
        setValue(value);
        blockSignals(false);
    }
}

void TPanelSeeker::stopFreeze()
{
    frozen = false;
    update();
}

void TPanelSeeker::goToSliderPosition()
{
    emit valueChanged(sliderPosition());
    dragDelayTimer->stop();
}

void TPanelSeeker::wheelEvent(QWheelEvent *e)
{
    blockSignals(true);
    QAbstractSlider::wheelEvent(e);
    moved(value());
    blockSignals(false);
    frozen = true;
    freezeTimer->start();
    if(!dragDelayTimer->isActive())
        dragDelayTimer->start();

}

void TPanelTimeSeeker::wheelEvent(QWheelEvent *e) {
	qDebug("Gui::Skin::TPanelTimeSeeker::wheelEvent: delta: %d", e->delta());
	e->accept();

	if (e->orientation() == Qt::Vertical) {
		if (e->delta() >= 0)
			emit wheelUp();
		else
			emit wheelDown();
	} else {
		qDebug("Gui::Skin::TPanelTimeSeeker::wheelEvent: horizontal event received, doing nothing");
	}
}

} // namesapce Skin
} // namespace Gui

#include "moc_panelseeker.cpp"