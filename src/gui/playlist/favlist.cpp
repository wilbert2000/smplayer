#include "gui/playlist/favlist.h"
#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/menu/menu.h"
#include "gui/action/action.h"
#include "gui/action/editabletoolbar.h"
#include "gui/mainwindow.h"
#include "gui/dockwidget.h"
#include "settings/preferences.h"
#include "settings/paths.h"
#include "player/player.h"
#include "images.h"
#include "iconprovider.h"

#include <QDir>
#include <QTimer>
#include <QMessageBox>


namespace Gui {
namespace Playlist {

TFavList::TFavList(TDockWidget *parent, TMainWindow* mw, TPlaylist* playlst) :
    TPList(parent, mw, "favlist", "fav", tr("Favorites")),
    playlist(playlst),
    currentFavAction(0) {

    playlistWidget->setSort(TPlaylistItem::COL_ORDER, Qt::AscendingOrder);

    createToolbar();

    favMenu = new Action::Menu::TMenu(this, mw, "favorites_menu",
                                      tr("Favorites"));
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());
    connect(favMenu, &Action::Menu::TMenu::triggered,
            this, &TFavList::onFavMenuTriggered);

    // Timer to merge TPlist::addedItems and/or multiple
    // TPlaylistWidget::modified signals into less calls to updateFavMenu
    requestUpdateTimer = new QTimer(this);
    requestUpdateTimer->setSingleShot(true);
    requestUpdateTimer->setInterval(500);
    connect(requestUpdateTimer, &QTimer::timeout,
            this, &TFavList::onRequestUpdateTimeout);

    connect(this, &TFavList::addedItems,
            this, &TFavList::onAddedItems);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TFavList::onModifiedChanged);
    connect(playlist->getPlaylistWidget(), &TPlaylistWidget::playingItemChanged,
            this, &TFavList::onPlaylistPlayingItemChanged);

    if (!QDir().mkpath(Settings::TPaths::favoritesPath())) {
        WZERROR(QString("Failed to create favorites directory '%1'. %2")
                .arg(Settings::TPaths::favoritesPath())
                .arg(strerror(errno)));
    }
    setPlaylistFilename(Settings::TPaths::favoritesFilename());
}

void TFavList::createToolbar() {

    QStringList actions;
    actions << "fav_open"
            << "fav_saveas"
            << "separator"
            << "fav_add_menu"
            << "fav_remove_menu"
            << "separator"
            << "fav_refresh"
            << "fav_browse_dir";
    toolbar->setDefaultActions(actions);
}

void TFavList::enableActions() {

    if (disableEnableActions) {
        WZTRACE("Disabled");
        return;
    }
    disableEnableActions++;

    WZTRACE("State " + player->stateToString());
    TPList::enableActions();
    findPlayingAct->setEnabled(!player->mdat.filename.isEmpty());

    disableEnableActions--;
}

// Currently not used
void TFavList::startPlay() {
    WZTRACE("");
    playlist->startPlay();
}

void TFavList::playItem(TPlaylistItem* item, bool keepPaused) {

    if (item) {
        // Could ignore keepPaused, only used by playlist playPrev/Next
        if (keepPaused && player->state() == Player::TState::STATE_PAUSED) {
           player->setStartPausedOnce();
        }
        playlist->open(item->filename(), item->baseName());
    }
}

void TFavList::openPlaylistDialog() {

    if (playlistWidget->hasPlayableItems()) {
        int res = QMessageBox::question(this, tr("Import favorites"),
            tr("This will import a new favorites playlist, overwriting"
               " your current favorites.\n"
               "Are you sure you want to proceed?"),
            QMessageBox::Yes,
            QMessageBox::No | QMessageBox::Default | QMessageBox::Escape);
        if (res != QMessageBox::Yes) {
            return;
        }
    }
    TPList::openPlaylistDialog();
}

bool TFavList::saveAs() {

    bool result = TPList::saveAs();
    // Restore favorites filename
    setPlaylistFilename(Settings::TPaths::favoritesFilename());
    return result;
}

void TFavList::refresh() {
    WZTRACE("");

    if (maybeSave()) {
        clear(false);
        if (QFileInfo(Settings::TPaths::favoritesPath()).exists()) {
            add(QStringList() << playlistFilename);
        }
    }
}

void TFavList::findPlayingItem() {

    TPlaylistItem* i = playlistWidget->findFilename(player->mdat.filename);
    if (i) {
        makeActive();
        if (i == playlistWidget->currentItem()) {
            playlistWidget->scrollToItem(i);
        } else {
            playlistWidget->setCurrentItem(i);
        }
    } else {
        playlist->findPlayingItem();
    }
}

void TFavList::removeAll() {

    if (QMessageBox::question(
                this, tr("Confirm clear favorites"),
                tr("Favorites are automatically saved to disk. Clearing them"
                   " will remove your favorites from '%1'.\n"
                   "Are you sure you want to proceed?")
                .arg(playlistFilename),
                QMessageBox::Yes,
                QMessageBox::No | QMessageBox::Default | QMessageBox::Escape)
            == QMessageBox::Yes) {
        TPList::removeAll();
    }
}

QAction* TFavList::fAction(QMenu* menu, const QString& filename) const {

    QList<QAction*> actions = menu->actions();
    for(int i = 0; i < actions.count(); i++) {
        QAction* action = actions.at(i);
        QVariant v = action->data();
        if (v.canConvert<TPlaylistItem*>()) {
            TPlaylistItem* item = v.value<TPlaylistItem*>();
            if (filename == item->filename()) {
                WZTRACE("Found '" + filename + "'");
                return action;
            }
        }
    }

    return 0;
}

QAction* TFavList::findAction(const QString& filename) const {
    return fAction(favMenu, filename);
}

void TFavList::onPlaylistPlayingItemChanged(TPlaylistItem* item) {

    WZTRACE(item ? item->baseName() : "0");

    if (currentFavAction) {
        markCurrentFavAction(0);
    }

    TPlaylistItem* playingItem = 0;
    TPlaylistItemState state = PSTATE_STOPPED;
    if (item) {
        QAction* action = findAction(item->filename());
        if (action) {
            markCurrentFavAction(action);
            playingItem = action->data().value<TPlaylistItem*>();
            state = item->state();
        }
    }
    playlistWidget->setPlayingItem(playingItem, state);
}

void TFavList::onRequestUpdateTimeout() {
    WZTRACE("");

    if (isBusy()) {
        requestUpdateTimer->start();
    } else {
        updateFavMenu();
    }
}

void TFavList::requestUpdate() {
    WZTRACE("");

    currentFavAction = 0;
    requestUpdateTimer->start();
 }

void TFavList::onAddedItems() {
    WZTRACE("");
    requestUpdate();
}

void TFavList::onModifiedChanged() {

    if (playlistWidget->isModified()) {
        WZTRACE("Modified set");
        requestUpdate();
    } else {
        WZTRACE("Modified cleared");
    }
}

void TFavList::markCurrentFavAction(QAction* action) {

    if (action != currentFavAction) {
        if (currentFavAction) {
            currentFavAction->setIcon(currentFavIcon);
        }
        currentFavAction = action;
        if (currentFavAction) {
            currentFavIcon = action->icon();
            currentFavAction->setIcon(iconProvider.iconPlaying);
        }
    }
}

void TFavList::onFavMenuTriggered(QAction* action) {
    WZTRACE("");

    if (action->data().isValid()) {
        TPlaylistItem* item = action->data().value<TPlaylistItem*>();
        QString filename = item->filename();
        if (!filename.isEmpty()) {
            markCurrentFavAction(action);
            playlist->open(filename, item->baseName());
        } else {
            WZERROR("No filename retrieved from action");
        }
    }
}

void TFavList::updFavMenu(QMenu* menu, TPlaylistItem* folder) {

    TPlaylistItem* playingItem = playlist->getPlaylistWidget()->playingItem;
    for(int i = 0; i < folder->childCount(); i++) {
        TPlaylistItem* item = folder->plChild(i);
        if (item->isFolder()) {
            QMenu* m = new QMenu(item->baseName(), menu);
            updFavMenu(m, item);
            connect(m, &QMenu::triggered, this, &TFavList::onFavMenuTriggered);
            menu->addMenu(m);
        } else {
            QAction* a = new QAction(item->baseName(), menu);
            a->setData(QVariant::fromValue(item));
            a->setIcon(item->getItemIcon());
            a->setStatusTip(item->filename());
            if (playingItem && item->filename() == playingItem->filename()) {
                markCurrentFavAction(a);
                playlistWidget->setPlayingItem(item, playingItem->state());
            }
            menu->addAction(a);
        }
    }
}

void TFavList::updateFavMenu() {
    WZTRACE("");

    currentFavAction = 0;
    playlistWidget->setPlayingItem(0);
    favMenu->clear();
    favMenu->addAction(addPlayingFileAct);
    favMenu->addAction(dock->toggleViewAction());

    TPlaylistItem* root = playlistWidget->root();
    if (root && root->childCount()) {
        favMenu->addSeparator();
        updFavMenu(favMenu, root);
    }

    if (playlistWidget->isModified()) {
        setPlaylistFilename(Settings::TPaths::favoritesFilename());
        save();
        // TODO: when removeSelected can remove trees use it to clean
        // sub dirs from favorites dir when no favs left
    }
}

} // namespace Playlist
} // namespace Gui
