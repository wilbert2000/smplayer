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


#ifndef GUI_PLAYERWINDOW_H
#define GUI_PLAYERWINDOW_H

#include <QWidget>
#include <QPoint>
#include <QSize>
#include <QTime>

class QTimer;
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QResizeEvent;

namespace Gui {

class TVideoWindow;

// Window containing video window
class TPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit TPlayerWindow(QWidget* parent,
                           const QString& name,
                           bool previewWindow);

    TVideoWindow* getVideoWindow() const { return video_window; }

    static QSize frame() { return QSize(2, 2); }

    void setResolution(int width, int height, const double afps);
    QSize resolution() const { return video_size; }
    QSize lastVideoOutSize() const { return last_video_out_size; }
    double aspectRatio() const { return aspect; }

    // Zoom
    // Sets current zoom to factor if factor_fullscreen == 0
    // else sets both zoom for normal and full screen.
    void setZoom(double factor,
                 double factor_fullscreen = 0,
                 bool updateVideoWindow = true);
    // Zoom current screen
    double zoom() const;
    // Zoom normal screen
    double zoomNormalScreen() const { return zoom_factor; }
    // Zoom full screen
    double zoomFullScreen() const { return zoom_factor_fullscreen; }

    // Pan
    void setPan(const QPoint& pan, const QPoint& pan_fullscreen);
    // Pan current screen
    QPoint pan() const;
    // Pan normal screen
    QPoint panNormalScreen() const { return pan_offset; }
    // Pan full screen
    QPoint panFullScreen() const { return pan_offset_fullscreen; }

    // Reset zoom and pan full and normal screen
    void resetZoomAndPan();

    void setDelayLeftClick(bool b) { delay_left_click = b; }

    // Calculate size factor for current view
    void getSizeFactors(double& factorX, double& factorY) const;
    double getSizeFactor() const;
    void updateSizeFactor();

    void updateVideoWindow();
    void moveVideo(int dx, int dy);

    void setColorKey();
    void restoreNormalWindow();

signals:
    void leftClicked();
    void doubleClicked();
    void rightClicked();
    void middleClicked();
    void xbutton1Clicked(); // first X button
    void xbutton2Clicked(); // second X button
    void dvdnavMousePos(const QPoint& pos);
    void wheelUp();
    void wheelDown();

    void draggingChanged(bool);

    void videoOutChanged();
    void videoSizeFactorChanged(double, double);

protected:
    virtual void resizeEvent(QResizeEvent*) override;
    virtual void mousePressEvent(QMouseEvent* e) override;
    virtual void mouseMoveEvent(QMouseEvent* e) override;
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
    virtual void wheelEvent(QWheelEvent* e) override;

private:
    TVideoWindow* video_window;
    bool isPreviewWindow;

    // Geometry
    QSize video_size;
    QSize last_video_out_size;
    double fps;
    double last_fps;
    double aspect;

    // Zoom
    double zoom_factor;
    double zoom_factor_fullscreen;
    // Pan
    QPoint pan_offset;
    QPoint pan_offset_fullscreen;

    // Mouse state
    bool double_clicked;
    bool delay_left_click;
    QTimer* left_click_timer;
    QTime left_button_pressed_time;
    QPoint drag_pos;
    bool dragging;

    void moveVideo(QPoint delta);

    void startDragging();
    void stopDragging();

    void setFastWindow();
    void updateSize();

private slots:
    void onLeftClicked();
};

} // namespace Gui

#endif // GUI_PLAYERWINDOW_H

