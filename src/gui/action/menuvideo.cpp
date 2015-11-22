#include "gui/action/menuvideo.h"
#include "settings/mediasettings.h"
#include "gui/action/menuvideofilter.h"
#include "gui/action/menuvideosize.h"
#include "gui/videoequalizer.h"
#include "gui/base.h"


using namespace Settings;

namespace Gui {


class TtMenuAspec : public TMenu {
public:
	explicit TtMenuAspec(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


TtMenuAspec::TtMenuAspec(QWidget* parent, TCore* c)
	: TMenu(parent, this, "aspect_menu", QT_TR_NOOP("&Aspect ratio"), "aspect")
	, core(c) {

	group = new TActionGroup(this, "aspect");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "aspect_detect", QT_TR_NOOP("&Auto"), TMediaSettings::AspectAuto);
	new TActionGroupItem(this, group, "aspect_1_1", QT_TR_NOOP("1&:1"), TMediaSettings::Aspect11);
	new TActionGroupItem(this, group, "aspect_5_4", QT_TR_NOOP("&5:4"), TMediaSettings::Aspect54);
	new TActionGroupItem(this, group, "aspect_4_3", QT_TR_NOOP("&4:3"), TMediaSettings::Aspect43);
	new TActionGroupItem(this, group, "aspect_11_8", QT_TR_NOOP("11:&8"), TMediaSettings::Aspect118);
	new TActionGroupItem(this, group, "aspect_14_10", QT_TR_NOOP("1&4:10"), TMediaSettings::Aspect1410);
	new TActionGroupItem(this, group, "aspect_3_2", QT_TR_NOOP("&3:2"), TMediaSettings::Aspect32);
	new TActionGroupItem(this, group, "aspect_14_9", QT_TR_NOOP("&14:9"), TMediaSettings::Aspect149);
	new TActionGroupItem(this, group, "aspect_16_10", QT_TR_NOOP("1&6:10"), TMediaSettings::Aspect1610);
	new TActionGroupItem(this, group, "aspect_16_9", QT_TR_NOOP("16:&9"), TMediaSettings::Aspect169);
	new TActionGroupItem(this, group, "aspect_2.35_1", QT_TR_NOOP("&2.35:1"), TMediaSettings::Aspect235);
	addSeparator();
	new TActionGroupItem(this, group, "aspect_none", QT_TR_NOOP("&Disabled"), TMediaSettings::AspectNone);
	group->setChecked(core->mset.aspect_ratio_id);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeAspectRatio(int)));
	connect(core, SIGNAL(aspectRatioChanged(int)), group, SLOT(setChecked(int)));
	addActionsTo(parent);
}

void TtMenuAspec::enableActions(bool stopped, bool video, bool) {
	// Uses mset, so useless to set if stopped or no video
	group->setEnabled(!stopped && video);
}

void TtMenuAspec::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->aspect_ratio_id);
}

void TtMenuAspec::onAboutToShow() {
	group->setChecked(core->mset.aspect_ratio_id);
}


class TMenuDeinterlace : public TMenu {
public:
	explicit TMenuDeinterlace(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


TMenuDeinterlace::TMenuDeinterlace(QWidget* parent, TCore* c)
	: TMenu(parent, this, "deinterlace_menu", QT_TR_NOOP("&Deinterlace"), "deinterlace")
	, core(c) {

	group = new TActionGroup(this, "deinterlace");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "deinterlace_none", QT_TR_NOOP("&None"), TMediaSettings::NoDeinterlace);
	new TActionGroupItem(this, group, "deinterlace_l5", QT_TR_NOOP("&Lowpass5"), TMediaSettings::L5);
	new TActionGroupItem(this, group, "deinterlace_yadif0", QT_TR_NOOP("&Yadif (normal)"), TMediaSettings::Yadif);
	new TActionGroupItem(this, group, "deinterlace_yadif1", QT_TR_NOOP("Y&adif (double framerate)"), TMediaSettings::Yadif_1);
	new TActionGroupItem(this, group, "deinterlace_lb", QT_TR_NOOP("Linear &Blend"), TMediaSettings::LB);
	new TActionGroupItem(this, group, "deinterlace_kern", QT_TR_NOOP("&Kerndeint"), TMediaSettings::Kerndeint);
	group->setChecked(core->mset.current_deinterlacer);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeDeinterlace(int)));
	// No one else sets it
	addActionsTo(parent);
}

void TMenuDeinterlace::enableActions(bool stopped, bool video, bool) {
	// Using mset, so useless to set if stopped or no video
	group->setEnabled(!stopped && video && core->videoFiltersEnabled());
}

void TMenuDeinterlace::onMediaSettingsChanged(TMediaSettings* mset) {
	group->setChecked(mset->current_deinterlacer);
}

void TMenuDeinterlace::onAboutToShow() {
	group->setChecked(core->mset.current_deinterlacer);
}


class TMenuRotate : public TMenu {
public:
	explicit TMenuRotate(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


TMenuRotate::TMenuRotate(QWidget* parent, TCore* c)
	: TMenu(parent, this, "rotate_menu", QT_TR_NOOP("&Rotate"), "rotate")
	, core(c) {

	group = new TActionGroup(this, "rotate");
	group->setEnabled(false);
	new TActionGroupItem(this, group, "rotate_none", QT_TR_NOOP("&Off"), TMediaSettings::NoRotate);
	new TActionGroupItem(this, group, "rotate_clockwise_flip", QT_TR_NOOP("&Rotate by 90 degrees clockwise and flip"), TMediaSettings::Clockwise_flip);
	new TActionGroupItem(this, group, "rotate_clockwise", QT_TR_NOOP("Rotate by 90 degrees &clockwise"), TMediaSettings::Clockwise);
	new TActionGroupItem(this, group, "rotate_counterclockwise", QT_TR_NOOP("Rotate by 90 degrees counterclock&wise"), TMediaSettings::Counterclockwise);
	new TActionGroupItem(this, group, "rotate_counterclockwise_flip", QT_TR_NOOP("Rotate by 90 degrees counterclockwise and &flip"), TMediaSettings::Counterclockwise_flip);
	group->setChecked(core->mset.rotate);
	connect(group, SIGNAL(activated(int)), core, SLOT(changeRotate(int)));
	// No one else changes it
	addActionsTo(parent);
}

void TMenuRotate::enableActions(bool stopped, bool video, bool) {
	// Using mset, so useless to set if stopped or no video
	group->setEnabled(!stopped && video && core->videoFiltersEnabled());
}

void TMenuRotate::onMediaSettingsChanged(Settings::TMediaSettings* mset) {
	group->setChecked(mset->rotate);
}

void TMenuRotate::onAboutToShow() {
	group->setChecked(core->mset.rotate);
}


class TMenuVideoZoomAndPan : public TMenu {
public:
	explicit TMenuVideoZoomAndPan(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
private:
	TCore* core;
	QActionGroup* group;
};


TMenuVideoZoomAndPan::TMenuVideoZoomAndPan(QWidget* parent, TCore* c)
	: TMenu(parent, this, "zoom_and_pan_menu", QT_TR_NOOP("&Zoom and pan"), "zoom_and_pan")
	, core(c) {

	group = new QActionGroup(this);
	group->setExclusive(false);
	group->setEnabled(false);

	// Zoom
	TAction* a = new TAction(this, "reset_zoom", QT_TR_NOOP("&Reset"), "zoom_reset", Qt::SHIFT | Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(resetZoomAndPan()));
	addSeparator();
	a = new TAction(this, "auto_zoom", QT_TR_NOOP("&Auto zoom"), "", Qt::SHIFT | Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoom()));
	a = new TAction(this, "zoom_169", QT_TR_NOOP("Zoom for &16:9"), "", Qt::SHIFT | Qt::Key_A);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor169()));
	a = new TAction(this, "zoom_235", QT_TR_NOOP("Zoom for &2.35:1"), "", Qt::SHIFT | Qt::Key_S);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(autoZoomFor235()));
	addSeparator();
	a = new TAction(this, "dec_zoom", QT_TR_NOOP("Zoom &-"), "", Qt::Key_W);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(decZoom()));
	a = new TAction(this, "inc_zoom", QT_TR_NOOP("Zoom &+"), "", Qt::Key_E);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(incZoom()));

	// Pan
	addSeparator();
	a = new TAction(this, "move_left", QT_TR_NOOP("Move &left"), "", Qt::ALT | Qt::Key_Left);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panRight()));
	a = new TAction(this, "move_right", QT_TR_NOOP("Move &right"), "", Qt::ALT | Qt::Key_Right);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panLeft()));
	a = new TAction(this, "move_up", QT_TR_NOOP("Move &up"), "", Qt::ALT | Qt::Key_Up);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panDown()));
	a = new TAction(this, "move_down", QT_TR_NOOP("Move &down"), "", Qt::ALT | Qt::Key_Down);
	group->addAction(a);
	connect(a, SIGNAL(triggered()), core, SLOT(panUp()));

	addActionsTo(parent);
}

void TMenuVideoZoomAndPan::enableActions(bool stopped, bool video, bool) {
	group->setEnabled(!stopped && video);
}


TMenuVideo::TMenuVideo(TBase* parent, TCore* c, TPlayerWindow* playerwindow, TVideoEqualizer* videoEqualizer)
	: TMenu(parent, this, "video_menu", QT_TR_NOOP("&Video"), "noicon")
	, core(c) {

	fullscreenAct = new TAction(this, "fullscreen", QT_TR_NOOP("&Fullscreen"), "", Qt::Key_F);
	fullscreenAct->addShortcut(QKeySequence("Ctrl+T")); // MCE remote key
	fullscreenAct->setCheckable(true);
	connect(fullscreenAct, SIGNAL(triggered(bool)), parent, SLOT(toggleFullscreen(bool)));

	exitFullscreenAct = new TAction(this, "exit_fullscreen", QT_TR_NOOP("Exit fullscreen"), "", Qt::Key_Escape, false);
	exitFullscreenAct->setEnabled(false);
	parent->addAction(exitFullscreenAct);
	connect(exitFullscreenAct, SIGNAL(triggered()), parent, SLOT(exitFullscreen()));

	// Screen submenu
#if USE_ADAPTER
	screenMenu = new TMenu(parent, this, "screen_menu", QT_TR_NOOP("Scree&n"), "screen");
	screenGroup = new TActionGroup(this, "screen");
	new TActionGroupItem(this, screenGroup, "screen_default", QT_TR_NOOP("Default screen"), -1, false);

#ifdef Q_OS_WIN
	TDeviceList display_devices = TDeviceInfo::displayDevices();
	if (!display_devices.isEmpty()) {
		for (int n = 0; n < display_devices.count(); n++) {
			int id = display_devices[n].ID().toInt();
			QString desc = display_devices[n].desc();
			TAction* screen_item = new TActionGroupItem(this, screenGroup,
				QString("screen_%1").arg(n),
				"&" + QString::number(n) + " - " + desc,
				id, false);
		}
	}
	else
#endif // Q_OS_WIN

	for (int n = 1; n <= 4; n++) {
		TAction* screen_item = new TActionGroupItem(this, screenGroup,
			QString("screen_%1").arg(n), "&" + QString::number(n), n, false);
	}

	screenGroup->setChecked(pref->adapter);
	addMenu(screenMenu);
	parent->addActions(screenGroup->actions());
	connect(screenGroup, SIGNAL(activated(int)), core, SLOT(changeAdapter(int)));
#endif

	// Aspect submenu
	addMenu(new TtMenuAspec(parent, core));
	// Size submenu
	addMenu(new TMenuVideoSize(parent, playerwindow));
	// Zoom and pan submenu
	addMenu(new TMenuVideoZoomAndPan(parent, core));

	// Equalizer
	addSeparator();
	videoEqualizerAct = new TAction(this, "video_equalizer", QT_TR_NOOP("&Equalizer"), "equalizer", QKeySequence("Ctrl+E"));
	videoEqualizerAct->setCheckable(true);
	videoEqualizerAct->setChecked(videoEqualizer->isVisible());
	connect(videoEqualizerAct, SIGNAL(triggered(bool)), videoEqualizer, SLOT(setVisible(bool)));
	connect(videoEqualizer, SIGNAL(visibilityChanged(bool)), videoEqualizerAct, SLOT(setChecked(bool)));

	// Deinterlace submenu
	addMenu(new TMenuDeinterlace(parent, core));
	// Video filter submenu
	addMenu(new TMenuVideoFilter(parent, core));
	// Stereo 3D
	stereo3dAct = new TAction(this, "stereo_3d_filter", QT_TR_NOOP("Stereo &3D filter..."), "stereo3d");
	connect(stereo3dAct, SIGNAL(triggered()), parent, SLOT(showStereo3dDialog()));

	// Rotate submenu
	addSeparator();
	addMenu(new TMenuRotate(parent, core));

	flipAct = new TAction(this, "flip", QT_TR_NOOP("Fli&p image"));
	flipAct->setCheckable(true);
	connect(flipAct, SIGNAL(triggered(bool)), core, SLOT(toggleFlip(bool)));

	mirrorAct = new TAction(this, "mirror", QT_TR_NOOP("&Mirror image"));
	mirrorAct->setCheckable(true);
	connect(mirrorAct, SIGNAL(triggered(bool)), core, SLOT(toggleMirror(bool)));

	// Video track
	addSeparator();
	// Next video track
	nextVideoTrackAct = new TAction(this, "next_video_track", QT_TR_NOOP("Next video track"));
	connect(nextVideoTrackAct, SIGNAL(triggered()), core, SLOT(nextVideoTrack()));

	videoTrackGroup = new TActionGroup(this, "videotrack");
	connect(videoTrackGroup, SIGNAL(activated(int)), core, SLOT(changeVideoTrack(int)));
	connect(core, SIGNAL(videoTrackInfoChanged()), this, SLOT(updateVideoTracks()));
	connect(core, SIGNAL(videoTrackChanged(int)), videoTrackGroup, SLOT(setChecked(int)));

	videoTrackMenu = new TMenu(parent, this, "videotrack_menu", QT_TR_NOOP("&Track"), "video_track");
	addMenu(videoTrackMenu);

	// Screenshots
	addSeparator();
	// Single
	screenshotAct = new TAction(this, "screenshot", QT_TR_NOOP("S&creenshot"), "", Qt::Key_S);
	connect(screenshotAct, SIGNAL(triggered()), core, SLOT(screenshot()));
	// Multiple
	screenshotsAct = new TAction(this, "multiple_screenshots", QT_TR_NOOP("Start/stop takin&g screenshots"), "screenshots", QKeySequence("Shift+D"));
	connect(screenshotsAct, SIGNAL(triggered()), core, SLOT(screenshots()));

#ifdef CAPTURE_STREAM
	capturingAct = new TAction(this, "capture_stream", QT_TR_NOOP("Start/stop capturing stream"), "record");
	connect(capturingAct, SIGNAL(triggered()), core, SLOT(switchCapturing()) );
#endif

#ifdef VIDEOPREVIEW
	addSeparator();
	videoPreviewAct = new TAction(this, "video_preview", QT_TR_NOOP("Thumb&nail generator..."));
	connect(videoPreviewAct, SIGNAL(triggered()), parent, SLOT(showVideoPreviewDialog()));
#endif

	addActionsTo(parent);
}

void TMenuVideo::enableActions(bool stopped, bool video, bool) {

	bool enableVideo = !stopped && video;
	nextVideoTrackAct->setEnabled(enableVideo && core->mdat.videos.count() > 1);

	bool enableFilters = enableVideo && core->videoFiltersEnabled();

	flipAct->setEnabled(enableFilters);
	mirrorAct->setEnabled(enableFilters);
	stereo3dAct->setEnabled(enableFilters);
	videoEqualizerAct->setEnabled(enableFilters);

	bool enableScreenShots = enableFilters
							 && pref->use_screenshot
							 && !pref->screenshot_directory.isEmpty()
							 && QFileInfo(pref->screenshot_directory).isDir();
	screenshotAct->setEnabled(enableScreenShots);
	screenshotsAct->setEnabled(enableScreenShots);

#ifdef CAPTURE_STREAM
	capturingAct->setEnabled(enableVideo
							 && !pref->capture_directory.isEmpty()
							 && QFileInfo(pref->capture_directory).isDir());
#endif

#if USE_ADAPTER
	screenGroup->setActionsEnabled(enableVideo
								   && pref->vo.startsWith("directx"));
#endif
}

void TMenuVideo::fullscreenChanged(bool fullscreen) {

	fullscreenAct->setChecked(fullscreen);
	exitFullscreenAct->setEnabled(fullscreen);
}

void TMenuVideo::onMediaSettingsChanged(Settings::TMediaSettings* mset) {

	flipAct->setChecked(mset->flip);
	mirrorAct->setChecked(mset->mirror);
}


void TMenuVideo::updateVideoTracks() {
	qDebug("Gui::TMenuVideo::updateVideoTracks");

	videoTrackGroup->clear();

	Maps::TTracks* videos = &core->mdat.videos;
	if (videos->count() == 0) {
		QAction* a = videoTrackGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	} else {
		Maps::TTracks::TTrackIterator i = videos->getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTrackData track = i.value();
			QAction* action = new QAction(videoTrackGroup);
			action->setCheckable(true);
			action->setText(track.getDisplayName());
			action->setData(track.getID());
			if (track.getID() == videos->getSelectedID())
				action->setChecked(true);
		}
	}

	videoTrackMenu->addActions(videoTrackGroup->actions());
}


} // namespace Gui
