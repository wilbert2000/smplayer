#include "gui/action/menuvideosize.h"
#include <QDebug>
#include "desktop.h"
#include "settings/preferences.h"
#include "playerwindow.h"
#include "gui/base.h"


using namespace Settings;

namespace Gui {

TVideoSizeGroup::TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw)
	: TActionGroup(parent, "size")
	, size_percentage(qRound(pref->size_factor * 100))
	, playerWindow(pw) {

	setEnabled(false);

	TActionGroupItem* a;
	new TActionGroupItem(this, this, "size_25", QT_TR_NOOP("25%"), 25, false);
	new TActionGroupItem(this, this, "size_50", QT_TR_NOOP("5&0%"), 50, false);
	new TActionGroupItem(this, this, "size_75", QT_TR_NOOP("7&5%"), 75, false);
	a = new TActionGroupItem(this, this, "size_100", QT_TR_NOOP("&100%"), 100, false);
	a->setShortcut(Qt::CTRL | Qt::Key_1);
	new TActionGroupItem(this, this, "size_125", QT_TR_NOOP("125%"), 125, false);
	new TActionGroupItem(this, this, "size_150", QT_TR_NOOP("15&0%"), 150, false);
	new TActionGroupItem(this, this, "size_175", QT_TR_NOOP("1&75%"), 175, false);
	a = new TActionGroupItem(this, this, "size_200", QT_TR_NOOP("&200%"), 200, false);
	a->setShortcut(Qt::CTRL | Qt::Key_2);
	new TActionGroupItem(this, this, "size_300", QT_TR_NOOP("&300%"), 300, false);
	new TActionGroupItem(this, this, "size_400", QT_TR_NOOP("&400%"), 400, false);

	setChecked(size_percentage);
}

void TVideoSizeGroup::uncheck() {

	QAction* current = checkedAction();
	if (current)
		current->setChecked(false);
}

void TVideoSizeGroup::enableVideoSizeGroup(bool on) {

	QSize s = playerWindow->resolution();
	setEnabled(on && s.width() > 0 && s.height() > 0);
}

void TVideoSizeGroup::updateVideoSizeGroup() {
	//qDebug("Gui::TVideoSizeGroup::updateVideoSizeGroup");

	uncheck();
	QSize s = playerWindow->resolution();
	if (s.width() <= 0 || s.height() <= 0) {
		setEnabled(false);
	} else {
		setEnabled(true);
		int factor = qRound(playerWindow->getSizeFactor() * 100);
		setChecked(factor);
		qDebug("Gui::TVideoSizeGroup::updateVideoSizeGroup: updating size factor from %d to %d",
			   size_percentage, factor);
		size_percentage = factor;
	}
}


TMenuVideoSize::TMenuVideoSize(TBase* mw, TCore* core, TPlayerWindow* pw)
	: TMenu(mw, this, "videosize_menu", QT_TR_NOOP("&Size"), "video_size")
	, mainWindow(mw)
	, playerWindow(pw)
	, block_update_size_factor(0) {

	group = new TVideoSizeGroup(this, pw);
	addActions(group->actions());
	connect(group, SIGNAL(activated(int)), mainWindow, SLOT(changeSize(int)));

	addSeparator();
	doubleSizeAct = new TAction(this, "toggle_double_size", QT_TR_NOOP("&Toggle double size"), "", Qt::CTRL | Qt::Key_D);
	connect(doubleSizeAct, SIGNAL(triggered()), mainWindow, SLOT(toggleDoubleSize()));

	currentSizeAct = new TAction(this, "video_size", "");
	connect(currentSizeAct, SIGNAL(triggered()), this, SLOT(optimizeSizeFactor()));

	connect(mainWindow, SIGNAL(videoSizeFactorChanged()),
			this, SLOT(onVideoSizeFactorChanged()));
	connect(core, SIGNAL(zoomChanged(double)),
			this, SLOT(onZoomChanged(double)));
	connect(mainWindow, SIGNAL(fullscreenChanged()),
			this, SLOT(onFullscreenChanged()));
	connect(mainWindow, SIGNAL(fullscreenChangedDone()),
			this, SLOT(onFullscreenChangedDone()));
	connect(mainWindow, SIGNAL(mainWindowResizeEvent(QResizeEvent*)),
			this, SLOT(onMainWindowResizeEvent(QResizeEvent*)));

	// Setup size factor changed timer
	update_size_factor_timer.setSingleShot(true);
	update_size_factor_timer.setInterval(750);
	connect(&update_size_factor_timer, SIGNAL(timeout()),
			this, SLOT(updateSizeFactor()));

	addActionsTo(mainWindow);
	upd();
}

void TMenuVideoSize::enableActions(bool stopped, bool video, bool) {

	group->enableVideoSizeGroup(!stopped && video);
	doubleSizeAct->setEnabled(group->isEnabled());
	currentSizeAct->setEnabled(group->isEnabled());
}

void TMenuVideoSize::upd() {
	qDebug("Gui::TMenuVideoSize:upd: %f", pref->size_factor);

	group->updateVideoSizeGroup();
	doubleSizeAct->setEnabled(group->isEnabled());
	currentSizeAct->setEnabled(group->isEnabled());

	// Update text and tips
	QString txt = translator->tr("&Optimize size %1%").arg(QString::number(group->size_percentage));
	currentSizeAct->setTextAndTip(txt);

	txt = translator->tr("Size %1%").arg(QString::number(group->size_percentage));
	QString scut = menuAction()->shortcut().toString();
	if (!scut.isEmpty()) {
		txt += " (" + scut + ")";
	}
	menuAction()->setToolTip(txt);
}

void TMenuVideoSize::onFullscreenChanged() {

	// Don't update size factor during all the resizing
	block_update_size_factor++;
}

void TMenuVideoSize::unlockSizeFactor() {
	qDebug("Gui::TMenuVideoSize::unlockSizeFactor");

	// Remove lock and upd. OK to not test lock
	block_update_size_factor--;
	upd();
}

void TMenuVideoSize::onFullscreenChangedDone() {

	// Delay update until resizing settles down
	QTimer::singleShot(500, this, SLOT(unlockSizeFactor()));
}

void TMenuVideoSize::onMainWindowResizeEvent(QResizeEvent* event) {

	// Update size factor if user induced
	if (event->spontaneous() && block_update_size_factor == 0) {
		// Delay update size factor, so multiple resizes get merged into
		// 1 call to updateSizeFactor()
		update_size_factor_timer.start();
	}
}

void TMenuVideoSize::updateSizeFactor() {
	qDebug("Gui::TMenuVideoSize:updateSizeFactor");

	playerWindow->updateSizeFactor();
	upd();
}

void TMenuVideoSize::onAboutToShow() {
	upd();
}

void TMenuVideoSize::onVideoSizeFactorChanged() {
	upd();
}

void TMenuVideoSize::onZoomChanged(double) {

	if (pref->fullscreen) {
		upd();
	}
}

bool TMenuVideoSize::optimizeSizeFactorPreDef(int factor, int predef_factor) {

	int d = predef_factor / 10;
	if (d < 10)
		d = 10;
	if (qAbs(factor - predef_factor) < d) {
		qDebug("Gui::TMenuVideoSize::optimizeSizeFactorPreDef: optimizing size from %d%% to predefined value %d%%",
			   factor, predef_factor);
		mainWindow->changeSize(predef_factor);
		return true;
	}
	return false;
}

void TMenuVideoSize::optimizeSizeFactor() {
	qDebug("Gui::TMenuVideoSize::optimizeSizeFactor");

	double factor;

	if (pref->fullscreen) {
		playerWindow->setZoom(1.0);
		factor = playerWindow->getSizeFactor();
		qDebug("Gui::TMenuVideoSize::optimizeSizeFactor: updating size factor from %f to %f",
			   pref->size_factor, factor);
		pref->size_factor = factor;
		upd();
		return;
	}

	double size_factor = pref->size_factor;

	// Limit size to 0.6 of available desktop
	const double f = 0.6;
	QSize available_size = TDesktop::availableSize(playerWindow);
	QSize res = playerWindow->resolution();
	QSize video_size = playerWindow->getAdjustedSize(res.width(), res.height(), size_factor);

	double max = f * available_size.height();
	// Adjust height first
	if (video_size.height() > max) {
		factor = max / res.height();
		qDebug("Gui::TMenuVideoSize::optimizeSizeFactor: height larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = playerWindow->getAdjustedSize(res.width(), res.height(), size_factor);
	}
	// Adjust width
	max = f * available_size.width();
	if (video_size.width() > max) {
		factor = max / res.width();
		qDebug("Gui::TMenuVideoSize::optimizeSizeFactor: width larger as %f desktop, reducing size factor from %f to %f",
			   f, size_factor, factor);
		size_factor = factor;
		video_size = playerWindow->getAdjustedSize(res.width(), res.height(), size_factor);
	}

	// Round to predefined values
	int factor_int = qRound(size_factor * 100);
	const int factors[] = {25, 50, 75, 100, 125, 150, 175, 200, 300, 400 };
	for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); i++) {
		if (optimizeSizeFactorPreDef(factor_int, factors[i])) {
			return;
		}
	}

	// Make width multiple of 16
	int new_w = ((video_size.width() + 8) / 16) * 16;
	factor = (double) new_w / res.width();
	qDebug("Gui::TMenuVideoSize::optimizeSizeFactor: optimizing width %d factor %f to multiple of 16 %d factor %f",
		   video_size.width(), size_factor, new_w, factor);
	mainWindow->changeSize(factor);
}

} // namespace Gui
