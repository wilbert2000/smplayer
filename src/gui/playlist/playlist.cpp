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

#include "gui/playlist/playlist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/playlist/playlistitem.h"
#include "gui/mainwindow.h"
#include "gui/action/action.h"
#include "gui/action/editabletoolbar.h"
#include "gui/msg.h"
#include "gui/filedialog.h"
#include "player/player.h"
#include "wzfiles.h"
#include "images.h"
#include "extensions.h"
#include "iconprovider.h"
#include "name.h"

#include <QToolBar>
#include <QMimeData>


namespace Gui {

using namespace Action;

namespace Playlist {

TPlaylist::TPlaylist(TDockWidget* parent, TMainWindow* mw) :
    TPList(parent, mw, "playlist", "pl", tr("Playlist")),
    debug(logger()),
    reachedEndOfPlaylist(false) {

    setAcceptDrops(true);

    createActions();
    createToolbar();

    connect(player, &Player::TPlayer::playerError,
            this, &TPlaylist::onPlayerError);
    connect(player, &Player::TPlayer::newMediaStartedPlaying,
            this, &TPlaylist::onNewMediaStartedPlaying);
    connect(player, &Player::TPlayer::titleTrackChanged,
            this, &TPlaylist::onTitleTrackChanged);
    connect(player, &Player::TPlayer::mediaEOF,
            this, &TPlaylist::onMediaEOF, Qt::QueuedConnection);
    connect(player, &Player::TPlayer::noFileToPlay,
            this, &TPlaylist::startPlay, Qt::QueuedConnection);

    // Random seed
    QTime t;
    t.start();
    qsrand(t.hour() * 3600 + t.minute() * 60 + t.second());
}

void TPlaylist::createActions() {

    // Play/pause
    playOrPauseAct = new TAction(mainWindow, "play_or_pause", tr("Play"),
                                 "play", Qt::Key_Space);
    // Add MCE remote key
    playOrPauseAct->addShortcut(QKeySequence("Toggle Media Play/Pause"));
    connect(playOrPauseAct, &TAction::triggered, this, &TPlaylist::playOrPause);

    // Play next
    playNextAct = new TAction(mainWindow, "play_next", tr("Play next"), "next",
                              QKeySequence(">"));
    playNextAct->addShortcut(QKeySequence("."));
    playNextAct->addShortcut(Qt::Key_MediaNext); // MCE remote key
    playNextAct->setData(4);
    connect(playNextAct, &TAction::triggered, this, &TPlaylist::playNext);

    // Play prev
    playPrevAct = new TAction(mainWindow, "play_prev", tr("Play previous"),
                              "previous", QKeySequence("<"));
    playPrevAct->addShortcut(QKeySequence(","));
    playPrevAct->addShortcut(Qt::Key_MediaPrevious); // MCE remote key
    playPrevAct->setData(4);
    connect(playPrevAct, &TAction::triggered, this, &TPlaylist::playPrev);

    // Repeat
    repeatAct = new TAction(mainWindow, "pl_repeat", tr("Repeat playlist"),
                            "", Qt::CTRL | Qt::Key_Backslash);
    repeatAct->setCheckable(true);
    connect(repeatAct, &TAction::triggered,
            this, &TPlaylist::onRepeatToggled);

    // Shuffle
    shuffleAct = new TAction(mainWindow, "pl_shuffle", tr("Shuffle playlist"),
                             "shuffle", Qt::ALT | Qt::Key_Backslash);
    shuffleAct->setCheckable(true);
    connect(shuffleAct, &TAction::triggered,
            this, &TPlaylist::onShuffleToggled);
}

void TPlaylist::createToolbar() {

    QStringList actions;
    actions << "pl_open"
            << "pl_save"
            << "pl_saveas"
            << "separator"
            << "pl_add_menu"
            << "pl_remove_menu"
            << "separator"
            << "pl_refresh"
            << "pl_browse_dir"
            << "separator"
            << "pl_shuffle"
            << "pl_repeat";
    toolbar->setDefaultActions(actions);
}


void TPlaylist::getFilesToPlay(QStringList& files) const {

    TPlaylistItem* root = playlistWidget->root();
    if (root == 0) {
        return;
    }

    QString fn = root->filename();
    if (!fn.isEmpty()) {
        files.append(fn);
        return;
    }

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        files.append(i->filename());
        it++;
    }
}

void TPlaylist::onRepeatToggled(bool toggled) {

    if (toggled)
        msgOSD(tr("Repeat playlist set"));
    else
        msgOSD(tr("Repeat playlist cleared"));
}

void TPlaylist::onShuffleToggled(bool toggled) {

    if (toggled)
        msgOSD(tr("Shuffle playlist set"));
    else
        msgOSD(tr("Shuffle playlist cleared"));
}

void TPlaylist::playOrPause() {
    WZDEBUG("state " + player->stateToString());

    switch (player->state()) {
        case Player::STATE_PLAYING: player->pause(); break;
        case Player::STATE_PAUSED: player->play(); break;
        case Player::STATE_STOPPING: break;

        case Player::STATE_RESTARTING:
        case Player::STATE_LOADING:
            stop();
            break;

        case Player::STATE_STOPPED:
        default:
            if (reachedEndOfPlaylist) {
                playNext(true);
            } else if (playlistWidget->playingItem) {
                playItem(playlistWidget->playingItem);
            } else if (player->mdat.filename.isEmpty()) {
                startPlay();
            } else {
                TPlaylistItem* item = playlistWidget->findFilename(
                            player->mdat.filename);
                if (item) {
                    playItem(item);
                } else {
                    player->play();
                }
            }
            break;
    }
}

void TPlaylist::stop() {
    WZINFO("State " + player->stateToString());

    TPList::stop();
    player->stop();
}

TPlaylistItem* TPlaylist::getRandomItem() const {

    bool foundFreeItem = false;
    double count =  playlistWidget->countChildren();
    int selected = int(count * qrand() / (RAND_MAX + 1.0));
    bool foundSelected = false;

    do {
        int idx = 0;
        QTreeWidgetItemIterator it(playlistWidget);
        while (*it) {
            TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
            if (!i->isFolder()) {
                if (idx == selected) {
                    foundSelected = true;
                }

                if (!i->played() && i->state() != PSTATE_FAILED) {
                    if (foundSelected) {
                        return i;
                    } else {
                        foundFreeItem = true;
                    }
                }

                idx++;
            }
            it++;
        }
    } while (foundFreeItem);

    WZDEBUG("End of playlist");
    return 0;
}

bool TPlaylist::haveUnplayedItems() const {

    QTreeWidgetItemIterator it(playlistWidget);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        if (!i->isFolder() && !i->played() && i->state() != PSTATE_FAILED) {
            return true;
        }
        ++it;
    }

    return false;
}

void TPlaylist::startPlay() {
    WZTRACE("");

    TPlaylistItem* item = playlistWidget->firstPlaylistItem();
    if (item) {
        if (shuffleAct->isChecked()) {
            playItem(getRandomItem());
        } else {
            playItem(item);
        }
    } else {
        WZINFO("Nothing to play");
        msg(tr("Nothing to play"));
    }
}

void TPlaylist::playItem(TPlaylistItem* item, bool keepPaused) {

    reachedEndOfPlaylist = false;
    while (item && item->isFolder()) {
        item = playlistWidget->getNextPlaylistItem(item);
    }
    if (item) {
        WZDEBUG("'" + item->filename() + "'");
        if (keepPaused && player->state() == Player::TState::STATE_PAUSED) {
           player->setStartPausedOnce();
        }
        playlistWidget->setPlayingItem(item, item->state());
        player->open(item->filename(), playlistWidget->hasSingleItem());
    } else {
        WZDEBUG("End of playlist");
        stop();
        reachedEndOfPlaylist = true;
        msg(tr("End of playlist"), 0);
        emit playlistFinished();
    }
}

void TPlaylist::playNext(bool loop_playlist) {
    WZDEBUG("");

    TPlaylistItem* item;
    if (shuffleAct->isChecked()) {
        item = getRandomItem();
        if (item == 0 && (repeatAct->isChecked() || loop_playlist)) {
            // Restart the playlist
            playlistWidget->clearPlayed();
            item = getRandomItem();
        }
    } else {
        item = playlistWidget->getNextPlaylistItem();
        if (item == 0 && (repeatAct->isChecked() || loop_playlist)) {
            // Select first item in playlist
            item = playlistWidget->firstPlaylistItem();
        }
    }
    playItem(item, player->mdat.image);
}

void TPlaylist::playPrev() {
    WZDEBUG("");

    TPlaylistItem* item = playlistWidget->playingItem;
    if (item && shuffleAct->isChecked()) {
        item = playlistWidget->findPreviousPlayedTime(item);
    } else {
        item = playlistWidget->getPreviousPlaylistWidgetItem();
    }
    if (item == 0) {
        item = playlistWidget->lastPlaylistItem();
    }
    if (item) {
        playItem(item, player->mdat.image);
    }
}

QString TPlaylist::playingFile() const {
    return playlistWidget->playingFile();
}

QString TPlaylist::getPlayingTitle(bool addModified) const {

    TPlaylistItem* item = playlistWidget->playingItem;
    if (!item) {
        item = playlistWidget->root();
    }
    QString title = item->editName();
    if (addModified && item->modified()) {
        title += "*";
    }
    return title;
}

void TPlaylist::enablePlayOrPause() {

    Player::TState ps = player->state();
    if (ps == Player::STATE_STOPPING) {
        playOrPauseAct->setTextAndTip(tr("Stopping player..."));
        playOrPauseAct->setIcon(iconProvider.iconStopping);
        playOrPauseAct->setEnabled(false);
        WZTRACE("Disabled in state stopping");
    } else if (ps == Player::STATE_PLAYING) {
        playOrPauseAct->setTextAndTip(tr("Pause"));
        playOrPauseAct->setEnabled(true);
        if (thread) {
            playOrPauseAct->setIcon(iconProvider.iconLoading);
            WZTRACE("Enabled in state playing while loading");
        } else {
            playOrPauseAct->setIcon(iconProvider.pauseIcon);
            WZTRACE("Enabled in state playing");
        }
    } else if (ps == Player::STATE_PAUSED) {
        playOrPauseAct->setTextAndTip(tr("Play"));
        playOrPauseAct->setEnabled(true);
        if (thread) {
            playOrPauseAct->setIcon(iconProvider.iconLoading);
            WZTRACE("Enabled in state paused while loading");
        } else {
            playOrPauseAct->setIcon(iconProvider.playIcon);
            WZTRACE("Enabled in state paused");
        }
    } else if (ps == Player::STATE_RESTARTING || ps == Player::STATE_LOADING) {
        QString s = player->stateToString().toLower();
        playOrPauseAct->setTextAndTip(tr("Stop %1").arg(s));
        playOrPauseAct->setIcon(iconProvider.iconStopping);
        playOrPauseAct->setEnabled(true);
        WZTRACE("Enabled in state " + s);
    } else if (ps == Player::STATE_STOPPED) {
        playOrPauseAct->setTextAndTip(tr("Play"));
        playOrPauseAct->setIcon(iconProvider.playIcon);
        playOrPauseAct->setEnabled(!player->mdat.filename.isEmpty()
                                   || playlistWidget->hasPlayableItems());
    }
} // enablePlayOrPause()

void TPlaylist::updatePlayingItem() {

    Player::TState ps = player->state();
    if (ps == Player::STATE_STOPPING) {
        WZTRACE("No updates in state stopping");
        return;
    }

    TPlaylistItem* item = playlistWidget->playingItem;
    if (!item) {
        WZTRACE("No playing item");
        return;
    }

    bool filenameMismatch = item->filename() != player->mdat.filename;
    if (filenameMismatch) {
        WZWARN(QString("File name playing item '%1' does not match"
                       " player filename '%2' in state %3")
               .arg(item->filename())
               .arg(player->mdat.filename)
               .arg(player->stateToString()));
    }

    if (ps == Player::STATE_STOPPED) {
        if (item->state() != PSTATE_STOPPED && item->state() != PSTATE_FAILED) {
            playlistWidget->setPlayingItem(item, PSTATE_STOPPED);
        }
    } else if (ps == Player::STATE_PLAYING || ps == Player::STATE_PAUSED) {
        if (item->state() != PSTATE_PLAYING) {
            playlistWidget->setPlayingItem(item, PSTATE_PLAYING);
        }
    } else  if (ps == Player::STATE_RESTARTING) {
        if (item->state() != PSTATE_LOADING) {
            playlistWidget->setPlayingItem(item, PSTATE_LOADING);
        }
    } else if (ps == Player::STATE_LOADING) {

        // The player->open(url) call can change the URL. It sets
        // player->mdat.filename just before switching state to loading.
        // If we do not copy the url here, onNewMediaStartedPlaying() will
        // start a new playlist, clearing the current playlist.
        if (filenameMismatch && !player->mdat.filename.isEmpty()) {
            WZWARN("Updating item file name");
            item->setFilename(player->mdat.filename);
            item->setModified();
        }

        if (item->state() != PSTATE_LOADING) {
            playlistWidget->setPlayingItem(item, PSTATE_LOADING);
        }
    }
}

void TPlaylist::enableActions() {

    if (disableEnableActions) {
        WZTRACE("Disabled");
        return;
    }
    disableEnableActions++;

    WZTRACE("State " + player->stateToString());
    updatePlayingItem();
    enablePlayOrPause();

    bool e = thread == 0
            && player->stateReady()
            && playlistWidget->hasPlayableItems();
    playNextAct->setEnabled(e);
    playPrevAct->setEnabled(e);
    findPlayingAct->setEnabled(playlistWidget->playingItem);
    // Repeat and shuffle always enabled

    TPList::enableActions();
    disableEnableActions--;
}

void TPlaylist::onPlayerError() {

    if (player->state() != Player::STATE_STOPPING) {
        TPlaylistItem* item = playlistWidget->playingItem;
        if (item && item->filename() == player->mdat.filename) {
            playlistWidget->setPlayingItem(item, PSTATE_FAILED);
            playlistWidget->setCurrentItem(item);
            playlistWidget->scrollToItem(item);
        }
    }
}

void TPlaylist::onNewMediaStartedPlaying() {

    const TMediaData* md = &player->mdat;
    QString filename = md->filename;
    QString curFilename = playlistWidget->playingFile();

    if (md->disc.valid) {
        // Handle disk, count items to check for changed disk :(
        TDiscName curDisc(curFilename);
        if (curDisc.valid
                && curDisc.protocol == md->disc.protocol
                && curDisc.device == md->disc.device
                // TODO: compare title?
                // && md->titles[curDisc.title].getDisplayName(false)
                // == playlistWidget->playingItem->baseName()
                && md->titles.count() == playlistWidget->countItems()) {
            WZDEBUG("Item is from current disc");
            return;
        }
    } else if (filename == curFilename) {
        // Handle current item started playing
        TPlaylistItem* item = playlistWidget->playingItem;
        if (item == 0) {
            return;
        }
        bool modified = false;

        // Update item name
        if (!item->edited()) {
            QString name = md->name();
            if (name != item->baseName()) {
                WZDEBUG(QString("Updating name from '%1' to '%2'")
                        .arg(item->baseName()).arg(name));
                item->setName(name, item->extension(), false);
                modified = true;
            }
        }

        // Update item duration
        if (!md->image && md->duration > 0) {
            if (!this->playlistFilename.isEmpty()
                && qAbs(md->duration - item->duration()) > 1) {
                modified = true;
            }
            WZDEBUG(QString("Updating duration from %1 to %2")
                    .arg(item->duration()).arg(md->duration));
            item->setDuration(md->duration);
        }

        if (modified) {
            item->setModified();
        } else {
            WZDEBUG("Item considered uptodate");
        }

        // Could set playingItem now, but wait for player to change state to
        // playing to not trigger additional calls to enableActions().
        // playlistWidget->setPlayingItem(item, PSTATE_PLAYING);

        // Pause a single image
        if (player->mdat.image && playlistWidget->hasSingleItem()) {
            mainWindow->runActionsLater("pause", true, true);
        }

        return;
    }

    // Create new playlist
    WZTRACE("Creating new playlist for '" + filename + "'");
    clear();

    if (md->disc.valid) {
        // Add disc titles without sorting
        playlistWidget->disableSort();
        TPlaylistItem* root = playlistWidget->root();
        root->setFilename(filename, md->name());
        // setFilename() won't recognize iso's as folder.
        // No need to update icon as it is not visible
        root->setFolder(true);

        TDiscName disc = md->disc;
        foreach(const Maps::TTitleData& title, md->titles) {
            disc.title = title.getID();
            TPlaylistItem* i = new TPlaylistItem(
                        root,
                        disc.toString(),
                        title.getDisplayName(false),
                        title.getDuration(),
                        false /* protext name */);
            if (title.getID() == md->titles.getSelectedID()) {
                playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
            }
        }
    } else {
        // Add current file
        TPlaylistItem* current = new TPlaylistItem(
                    playlistWidget->root(),
                    filename,
                    md->name(),
                    md->duration,
                    false);
        playlistWidget->setPlayingItem(current, PSTATE_PLAYING);
    }

    setPLaylistTitle();
    WZTRACE("Created new playlist for '" + filename + "'");
}

void TPlaylist::onMediaEOF() {
    WZTRACE("");
    playNext(false);
}

void TPlaylist::onTitleTrackChanged(int id) {
    WZTRACE(QString::number(id));

    if (id < 0) {
        playlistWidget->setPlayingItem(0);
        return;
    }

    // Search for id
    TDiscName disc = player->mdat.disc;
    disc.title = id;
    QString filename = disc.toString();

    TPlaylistItem* i = playlistWidget->findFilename(filename);
    if (i) {
        playlistWidget->setPlayingItem(i, PSTATE_PLAYING);
    } else {
        WZWARN("title id " + QString::number(id) + " filename '" + filename
               + "' not found in playlist");
    }
}

void TPlaylist::refresh() {

    if (!playlistFilename.isEmpty() && maybeSave()) {
        QString playing;
        if (player->statePOP()) {
            playing = playlistWidget->playingFile();
            if (!playing.isEmpty()) {
                player->saveRestartState();
            }
        }
        clear(false);
        add(QStringList() << playlistFilename, !playing.isEmpty(), 0, playing);
    }
}

void TPlaylist::findPlayingItem() {

    TPlaylistItem* i = playlistWidget->playingItem;
    if (i) {
        makeActive();
        if (i == playlistWidget->currentItem()) {
            playlistWidget->scrollToItem(i);
        } else {
            playlistWidget->setCurrentItem(i);
        }
    }
}

// Drag&drop
void TPlaylist::dragEnterEvent(QDragEnterEvent *e) {
    debug << "dragEnterEvent" << e->mimeData()->formats();
    debug << debug;

    if (e->mimeData()->hasUrls()) {
        if (e->proposedAction() & Qt::CopyAction) {
            e->acceptProposedAction();
            return;
        }
        if (e->possibleActions() & Qt::CopyAction) {
            e->setDropAction(Qt::CopyAction);
            e->accept();
            return;
        }
    }
    TPList::dragEnterEvent(e);
}

void TPlaylist::dropEvent(QDropEvent *e) {
    debug << "dropEvent" << e->mimeData()->formats();
    debug << debug;

    if (e->mimeData()->hasUrls()) {
        QStringList files;
        foreach(const QUrl& url, e->mimeData()->urls()) {
            files.append(url.toString());
        }

        if (files.count()) {
            // TODO: see dropIndicator for above/below
            QTreeWidgetItem* target = playlistWidget->itemAt(
                        e->pos()
                        - playlistWidget->pos()
                        - playlistWidget->viewport()->pos());
            add(files, false, static_cast<TPlaylistItem*>(target));
        }

        e->accept();
        return;
    }
    TPList::dropEvent(e);
}

void TPlaylist::open(const QString &fileName, const QString& name) {
    WZDEBUG("'" + fileName + "'");

    if (fileName.isEmpty()) {
        WZDEBUG("Nothing to play");
        return;
    }
    if (!maybeSave()) {
        return;
    }

    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            openDirectory(fi.absoluteFilePath());
            return;
        }
        if (extensions.isPlaylist(fi)) {
            openPlaylist(fi.absoluteFilePath());
            return;
        }
        Settings::pref->last_dir = fi.absolutePath();
    }

    // Find in playlist
    TPlaylistItem* item = playlistWidget->findFilename(fileName);
    if (item) {
        WZDEBUG("Found item in playlist");
        playItem(item);
        return;
    }

    WZDEBUG("Starting new playlist");
    clear();
    bool edited = !name.isEmpty() && name != TName::baseNameForURL(fileName);
    playItem(new TPlaylistItem(playlistWidget->root(),
                               fileName,
                               name,
                               0,
                               edited));
    WZDEBUG("done");
}

void TPlaylist::openFileDialog() {
    WZDEBUG("");

    QString s = TFileDialog::getOpenFileName(
        this,
        tr("Choose a file"),
        Settings::pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;"
        + tr("Video") + extensions.video().forFilter() + ";;"
        + tr("Audio") + extensions.audio().forFilter() + ";;"
        + tr("Playlists") + extensions.playlists().forFilter() + ";;"
        + tr("Images") + extensions.images().forFilter() + ";;"
        + tr("All files") +" (*.*)");

    if (!s.isEmpty()) {
        open(s);
    }
}


void TPlaylist::openFiles(const QStringList& files, const QString& fileToPlay) {
    WZDEBUG("");

    if (files.empty()) {
        WZDEBUG("No files in list to open");
        return;
    }

    if (maybeSave()) {
        clear();
        add(files, true, 0, fileToPlay);
    }
}

void TPlaylist::openDirectory(const QString& dir) {
    WZDEBUG("'" + dir + "'");

    Settings::pref->last_dir = dir;

    if (TWZFiles::directoryContainsDVD(dir)) {
        // Calls clear()
        openDisc(TDiscName(dir, Settings::pref->useDVDNAV()));
    } else {
        clear();
        add(QStringList() << dir, true);
    }
}

void TPlaylist::openDirectoryDialog() {
    WZDEBUG("");

    if (maybeSave()) {
        QString s = TFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"), Settings::pref->last_dir);
        if (!s.isEmpty()) {
            openDirectory(s);
        }
    }
}

void TPlaylist::openDisc(const TDiscName& disc) {

    if (maybeSave()) {
        // onNewMediaStartedPlaying() will pick up the playlist
        clear();
        player->openDisc(disc);
    }
}

void TPlaylist::saveSettings() {

    TPList::saveSettings();
    Settings::pref->beginGroup(objectName());
    Settings::pref->setValue("repeat", repeatAct->isChecked());
    Settings::pref->setValue("shuffle", shuffleAct->isChecked());
    Settings::pref->endGroup();
}

void TPlaylist::loadSettings() {

    TPList::loadSettings();
    Settings::pref->beginGroup(objectName());
    repeatAct->setChecked(
        Settings::pref->value("repeat", repeatAct->isChecked()).toBool());
    shuffleAct->setChecked(
        Settings::pref->value("shuffle", shuffleAct->isChecked()).toBool());
    Settings::pref->endGroup();
}

} // namespace Playlist
} // namespace Gui

#include "moc_playlist.cpp"
