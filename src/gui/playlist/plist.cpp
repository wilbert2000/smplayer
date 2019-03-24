#include "gui/playlist/plist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/playlist/addfilesthread.h"
#include "gui/action/menu/menu.h"
#include "gui/action/action.h"
#include "gui/action/editabletoolbar.h"
#include "gui/mainwindow.h"
#include "gui/dockwidget.h"
#include "gui/filedialog.h"
#include "gui/multilineinputdialog.h"
#include "gui/msg.h"
#include "player/player.h"
#include "settings/preferences.h"
#include "settings/paths.h"
#include "extensions.h"
#include "iconprovider.h"
#include "version.h"

#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDesktopServices>


namespace Gui {
namespace Playlist {

TMenuAddRemoved::TMenuAddRemoved(TPList* pl,
                                 TMainWindow* mw,
                                 TPlaylistWidget* plw,
                                 const QString& name) :
    TMenu(pl, mw, name, tr("Add removed item"), "noicon"),
    playlistWidget(plw) {

    menuAction()->setIcon(iconProvider.trashIcon);

    connect(this, &TMenuAddRemoved::triggered,
            this, &TMenuAddRemoved::onTriggered);
    connect(this, &TMenuAddRemoved::addRemovedItem,
            pl, &TPList::addRemovedItem);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TMenuAddRemoved::onCurrentItemChanged);
    connect(this, &TMenuAddRemoved::aboutToShow,
            this, &TMenuAddRemoved::onAboutToShow);

    setEnabled(false);
}

void TMenuAddRemoved::onAboutToShow() {

    clear();
    int c = 0;
    item = playlistWidget->plCurrentItem();
    if (item) {
        if (!item->isFolder()) {
            item = item->plParent();
        }
        if (item) {
            foreach(const QString& s, item->getBlacklist()) {
                QAction* action = new QAction(s, this);
                action->setData(s);
                addAction(action);
                c++;
            }
        }
    }

    if (c == 0) {
        QAction* action = new QAction(tr("No removed items"), this);
        action->setEnabled(false);
        addAction(action);
    }
}

void TMenuAddRemoved::onTriggered(QAction* action) {

    QString s = action->data().toString();
    if (!s.isEmpty()) {
        // Check item still valid
        item = playlistWidget->validateItem(item);
        if (item && item->whitelist(s)) {
            item->setModified();
            // TODO:
            emit addRemovedItem(s);
        }
    }
}

void TMenuAddRemoved::onCurrentItemChanged(QTreeWidgetItem* current,
                                           QTreeWidgetItem*) {

    bool enable = false;
    if (current) {
        TPlaylistItem* cur = static_cast<TPlaylistItem*>(current);
        if (cur->isFolder()) {
            enable = cur->getBlacklistCount();
        } else {
            cur = cur->plParent();
            if (cur) {
                enable = cur->getBlacklistCount();
            }
        }
    }
    setEnabled(enable);
}

TPList::TPList(TDockWidget* parent,
               TMainWindow* mw,
               const QString& name,
               const QString& aShortName,
               const QString& aTransName) :
    QWidget(parent),
    debug(logger()),
    mainWindow(mw),
    dock(parent),
    thread(0),
    disableEnableActions(0),
    shortName(aShortName),
    tranName(aTransName),
    restartThread(false),
    isFavList(aShortName == "fav"),
    skipRemainingMessages(false) {

    setObjectName(name);

    createTree();
    createActions();
    createToolbar();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(toolbar);
    layout->addWidget(playlistWidget);
    setLayout(layout);

    dock->setFocusProxy(playlistWidget);
}

TPList::~TPList() {

    // Prevent onThreadFinished handling results
    thread = 0;
}

void TPList::createTree() {

    playlistWidget = new TPlaylistWidget(this, objectName() + "widget",
                                         shortName, tranName);

    connect(playlistWidget, &TPlaylistWidget::itemActivated,
            this, &TPList::onItemActivated);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TPList::onCurrentItemChanged);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TPList::setPLaylistTitle,
            Qt::QueuedConnection);
}

void TPList::createActions() {

    using namespace Action;

    QString tranNameLower = tranName.toLower();
    QObject* owner;
    if (isFavList) {
        owner = this;
    } else {
        owner = mainWindow;
    }

    // Open playlist
    openAct = new TAction(owner, shortName + "_open",
                          isFavList
                          ? tr("Import favorites")
                          : tr("Open playlist"),
                          "noicon", QKeySequence("Ctrl+P"));
    openAct->setIcon(iconProvider.openIcon);
    connect(openAct, &TAction::triggered,
            this, &TPList::openPlaylistDialog);

    // Save playlist
    saveAct = new TAction(owner, shortName + "_save",
                          tr("Save %1").arg(tranNameLower),
                          "noicon", QKeySequence("Ctrl+S"));
    saveAct->setIcon(iconProvider.saveIcon);
    connect(saveAct, &TAction::triggered, this, &TPList::save);

    // SaveAs
    saveAsAct = new TAction(owner, shortName + "_saveas",
                            tr("Save %1 as...").arg(tranNameLower), "noicon");
    saveAsAct->setIcon(iconProvider.saveAsIcon);
    connect(saveAsAct, &TAction::triggered, this, &TPList::saveAs);

    // Refresh
    refreshAct = new TAction(owner, shortName+ "_refresh",
                             tr("Refresh %1").arg(tranNameLower), "noicon",
                             Qt::Key_F5);
    refreshAct->setIcon(iconProvider.refreshIcon);
    connect(refreshAct, &TAction::triggered, this, &TPList::refresh);

    // Browse directory
    browseDirAct = new TAction(owner, shortName + "_browse_dir",
                               tr("Browse directory or URL"), "noicon");
    browseDirAct->setIcon(QIcon(style()->standardIcon(QStyle::SP_DirOpenIcon)));
    connect(browseDirAct, &TAction::triggered, this, &TPList::browseDir);

    // Play
    playAct = new TAction(owner, shortName + "_play", tr("Play"), "play",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, &Action::TAction::triggered, this, &TPList::play);

    // Play in new window
    playInNewWindowAct = new TAction(owner, shortName + "_play_in_new_window",
                             tr("Play in new window"), "play",
                             Qt::CTRL | Qt::Key_Space);
    connect(playInNewWindowAct, &TAction::triggered,
            this, &TPList::playInNewWindow);

    // Context menu
    Action::Menu::TMenu* contextMenu = new Action::Menu::TMenu(
                this, mainWindow, shortName + "_context_menu",
                tr("%1 context menu").arg(tranName));
    connect(contextMenu, &Action::Menu::TMenu::aboutToShow,
            this, &TPList::enableActions);

    contextMenu->addAction(playAct);
    contextMenu->addAction(playInNewWindowAct);

    contextMenu->addSeparator();
    // Edit name
    editNameAct = new TAction(owner, shortName + "_edit_name",
                              tr("Edit name..."), "", Qt::Key_F2);
    connect(editNameAct, &TAction::triggered,
            this, &TPList::editName);
    contextMenu->addAction(editNameAct);

    // New folder
    newFolderAct = new TAction(this, shortName + "_new_folder",
                               tr("New folder"), "noicon", Qt::Key_F10);
    newFolderAct->setIcon(iconProvider.newFolderIcon);
    connect(newFolderAct, &TAction::triggered, this, &TPList::newFolder);
    contextMenu->addAction(newFolderAct);

    // Find playing
    findPlayingAct = new TAction(owner, shortName + "_find_playing",
                                 tr("Find playing item"), "noicon", Qt::Key_F3);
    findPlayingAct->setIcon(iconProvider.findIcon);
    connect(findPlayingAct, &TAction::triggered,
            this, &TPList::findPlayingItem);
    contextMenu->addAction(findPlayingAct);

    contextMenu->addSeparator();
    // Cut
    cutAct = new TAction(this, shortName + "_cut", tr("Cut file name(s)"),
                         "noicon", QKeySequence("Ctrl+X"));
    cutAct->setIcon(iconProvider.cutIcon);
    connect(cutAct, &TAction::triggered, this, &TPlaylist::cut);
    contextMenu->addAction(cutAct);

    // Copy
    copyAct = new TAction(owner, shortName + "_copy", tr("Copy file name(s)"),
                          "noicon", QKeySequence("Ctrl+C"));
    copyAct->setIcon(iconProvider.copyIcon);
    connect(copyAct, &TAction::triggered, this, &TPList::copySelected);
    contextMenu->addAction(copyAct);

    // Paste
    pasteAct = new TAction(owner, shortName + "_paste",
                           tr("Paste file name(s)"), "noicon",
                           QKeySequence("Ctrl+V"));
    pasteAct->setIcon(iconProvider.pasteIcon);
    connect(pasteAct, &TAction::triggered, this, &TPList::paste);
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &TPList::enablePaste);
    contextMenu->addAction(pasteAct);

    contextMenu->addSeparator();
    // Add menu
    playlistAddMenu = new Menu::TMenu(this, mainWindow, shortName + "_add_menu",
                                      tr("Add to %1").arg(tranName.toLower()),
                                      "noicon");
    playlistAddMenu->menuAction()->setIcon(iconProvider.okIcon);

    // Add playing
    QObject* altOwner;
    if (isFavList) {
        altOwner = mainWindow;
    } else {
        altOwner = this;
    }
    addPlayingFileAct = new TAction(altOwner, shortName + "_add_playing",
                                    tr("Add playing file"), "play");
    playlistAddMenu->addAction(addPlayingFileAct);
    connect(addPlayingFileAct, &TAction::triggered,
            this, &TPlaylist::addPlayingFile);

    // Add files
    TAction* a = new TAction(owner, shortName + "_add_files",
                             tr("Add file(s)..."), "noicon");
    a->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    playlistAddMenu->addAction(a);
    connect(a, &TAction::triggered, this, &TPList::addFilesDialog);

    // Add dir
    a = new TAction(owner, shortName + "_add_directory", tr("Add directory..."),
                    "noicon");
    a->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    playlistAddMenu->addAction(a);
    connect(a, &TAction::triggered, this, &TPList::addDirectoryDialog);

    // Add URLs
    a = new TAction(owner, shortName + "_add_urls", tr("Add URL(s)..."),
                    "add_url");
    playlistAddMenu->addAction(a);
    connect(a, &TAction::triggered, this, &TPList::addUrlsDialog);

    // Add removed sub menu
    playlistAddMenu->addMenu(new TMenuAddRemoved(this, mainWindow,
         playlistWidget, shortName + "_add_removed_menu"));

    contextMenu->addMenu(playlistAddMenu);

    // Remove menu
    playlistRemoveMenu = new Menu::TMenu(this, mainWindow,
        shortName + "_remove_menu",
        tr("Remove from %1").arg(tranNameLower), "noicon");
    playlistRemoveMenu->menuAction()->setIcon(iconProvider.cancelIcon);
    connect(playlistRemoveMenu, &Menu::TMenu::aboutToShow,
            this, &TPList::enableRemoveMenu);

    // Delete from playlist
    removeSelectedAct = new TAction(this, shortName + "_delete",
        tr("Delete from %1").arg(tranNameLower), "noicon", Qt::Key_Delete);
    removeSelectedAct->setIcon(iconProvider.trashIcon);
    playlistRemoveMenu->addAction(removeSelectedAct);
    connect(removeSelectedAct, &TAction::triggered,
            this, &TPlaylist::removeSelected);

    // Delete from disk
    QString txt = isFavList ?
                tr("Delete from favorites folder...")
              : tr("Delete from disk...");
    removeSelectedFromDiskAct = new TAction(this,
                                            shortName + "_delete_from_disk",
                                            txt, "noicon",
                                            Qt::SHIFT | Qt::Key_Delete);
    removeSelectedFromDiskAct->setIcon(iconProvider.discardIcon);
    playlistRemoveMenu->addAction(removeSelectedFromDiskAct);
    connect(removeSelectedFromDiskAct, &TAction::triggered,
            this, &TPlaylist::removeSelectedFromDisk);

    // Clear playlist
    removeAllAct = new TAction(this, shortName + "_clear",
                               tr("Clear %1").arg(tranNameLower),
                               "noicon", Qt::CTRL | Qt::Key_Delete);
    removeAllAct->setIcon(iconProvider.clearIcon);
    playlistRemoveMenu->addAction(removeAllAct);
    connect(removeAllAct, &TAction::triggered, this, &TPlaylist::removeAll);

    contextMenu->addMenu(playlistRemoveMenu);

    playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(playlistWidget, &TPlaylistWidget::customContextMenuRequested,
            contextMenu, &Action::Menu::TMenu::execSlot);

    contextMenu->addSeparator();
    contextMenu->addAction(refreshAct);
    contextMenu->addAction(browseDirAct);
}

void TPList::createToolbar() {

    toolbar = new Action::TEditableToolbar(this, shortName + "_toolbar",
                                           tr("%1 toolbar").arg(tranName));
    QAction* action = toolbar->toggleViewAction();
    action->setShortcut(isFavList
                        ? Qt::SHIFT | Qt::Key_F9
                        : Qt::SHIFT | Qt::Key_F8);
    addAction(action);
}

void TPList::enablePaste() {

    pasteAct->setEnabled(!isBusy()
                         && QApplication::clipboard()->mimeData()->hasText());
}

void TPList::enableRemoveFromDiskAction() {

    TPlaylistItem* current = playlistWidget->plCurrentItem();
    removeSelectedFromDiskAct->setEnabled(
                !isBusy()
                && current
                && (!current->isWZPlaylist()
                    || current->isSymLink()
                    || current->childCount() == 0));
    // Leave test for non media files in directory to deleteFileFromDisk()
}

void TPList::enableRemoveMenu() {

    bool enable = !isBusy() && playlistWidget->hasItems();
    removeSelectedAct->setEnabled(enable);
    removeAllAct->setEnabled(enable);
    enableRemoveFromDiskAction();
}

void TPList::enableActions() {
    WZTRACEOBJ("");

    openAct->setEnabled(player->state() != Player::STATE_STOPPING);

    // saveAct->setEnabled(true);
    // saveAsAct->setEnabled(true);
    refreshAct->setEnabled(!playlistFilename.isEmpty());

    bool haveFile = !player->mdat.filename.isEmpty();
    bool cur = playlistWidget->plCurrentItem();
    browseDirAct->setEnabled(haveFile || cur);
    browseDirAct->setToolTip(tr("Browse '%1'")
                             .arg(getBrowseURL().toDisplayString()));
    playInNewWindowAct->setEnabled(haveFile || playlistWidget->hasItems());

    bool enable = !isBusy() && player->stateReady();
    editNameAct->setEnabled(enable && cur);
    newFolderAct->setEnabled(enable);
    // findPlayingAct by descendants

    cutAct->setEnabled(enable && cur);
    copyAct->setEnabled(haveFile || cur);
    enablePaste();

    // Add menu
    addPlayingFileAct->setEnabled(haveFile);
    // Remove menu
    enableRemoveMenu();
}

bool TPList::isBusy() const {
    return thread || playlistWidget->isBusy();
}

void TPList::makeActive() {

    if (!dock->isVisible()) {
        dock->setVisible(true);
    }
    activateWindow();
    raise();
    playlistWidget->setFocus(Qt::OtherFocusReason);
}

void TPList::setPlaylistFilename(const QString& filename) {

    playlistFilename = QDir::toNativeSeparators(filename);
    WZTRACE("Setting playlist filename to '" + playlistFilename + "'");
    playlistWidget->root()->setFilename(playlistFilename);
    setPLaylistTitle();
}

bool TPList::maybeSave() {

    if (!playlistWidget->isModified()) {
        WZTRACEOBJ(tranName + " not modified");
        return true;
    }

    // TODO: be more precise
    if (playlistFilename.isEmpty()) {
        WZINFO("Discarding unnamed playlist");
        return true;
    }

    QFileInfo fi(playlistFilename);
    if (fi.fileName().compare(TConfig::WZPLAYLIST, caseSensitiveFileNames)
            == 0) {
        return save();
    }

    if (fi.isDir()) {
        if (Settings::pref->useDirectoriePlaylists) {
            return save();
        }
        WZDEBUGOBJ("Discarding changes. Saving directorie playlists is disabled.");
        return true;

    }

    int res = QMessageBox::question(this, tr("%1 modified").arg(tranName),
        tr("The playlist has been modified, do you want to save the changes to"
           " '%1'?").arg(playlistFilename),
        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

    switch (res) {
        case QMessageBox::No:
            playlistWidget->clearModified();
            WZINFO("Selected no save");
            return true;
        case QMessageBox::Cancel:
            WZINFO("Selected cancel save");
            return false;
        default:
            WZINFO("Selected save");
            return save();
    }
}

void TPList::openPlaylist(const QString& filename) {

    Settings::pref->last_dir = QFileInfo(filename).absolutePath();
    clear();
    add(QStringList() << filename, true);
}

void TPList::openPlaylistDialog() {

    if (maybeSave()) {
        QString fn = TFileDialog::getOpenFileName(this, tr("Choose a file"),
            Settings::pref->last_dir,
            tr("Playlists") + extensions.playlists().forFilter() + ";;"
            + tr("All files") +" (*.*)");

        if (!fn.isEmpty()) {
            openPlaylist(fn);
        }
    }
}

bool TPList::saveM3uFolder(TPlaylistItem* folder,
                           const QString& path,
                           QTextStream& stream,
                           bool linkFolders,
                           bool& savedMetaData) {
    WZTRACEOBJ(QString("Saving '%1'").arg(folder->filename()));

    bool result = true;
    for(int idx = 0; idx < folder->childCount(); idx++) {
        TPlaylistItem* i = folder->plChild(idx);
        QString filename = i->filename();

        if (i->isPlaylist()) {
            if (i->modified()) {
                if (!saveM3u(i, filename, i->isWZPlaylist())) {
                    result = false;
                }
            } else {
                WZTRACEOBJ("Playlist '" + filename + "' not modified");
            }
        } else if (i->isFolder()) {
            if (linkFolders) {
                if (i->modified()) {
                    QFileInfo fi(filename, TConfig::WZPLAYLIST);
                    filename = QDir::toNativeSeparators(fi.absoluteFilePath());
                    if (!saveM3u(i, filename, linkFolders)) {
                        result = false;
                    }
                } else {
                    WZTRACEOBJ("Folder '" + filename + "' not modified");
                }
            } else {
                // Note: savedMetaData destroyed as dummy here. It is only used
                // for WZPlaylists which have linkFolders set to true.
                if (saveM3uFolder(i, path, stream, linkFolders, savedMetaData)) {
                    WZINFO("Succesfully saved '" + filename + "'");
                } else {
                    result = false;
                }
                continue;
            }
        } else {
            int d = (int) i->duration();
            stream << "#EXTINF:" << d << "," << i->baseName() << "\n";
            if (!savedMetaData) {
                if (isFavList || d || i->edited()) {
                    savedMetaData = true;
                }
            }
        }

        if (filename.startsWith(path)) {
            filename = filename.mid(path.length());
        }
        stream << filename << "\n";
    }

    return result;
}

bool TPList::saveM3u(TPlaylistItem* folder,
                     const QString& filename,
                     bool wzplaylist) {
    WZTRACEOBJ(QString("Saving '%1'").arg(filename));

    QString path = QDir::toNativeSeparators(QFileInfo(filename).dir().path());
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    QFile file(filename);

    do {
        if (file.open(QIODevice::WriteOnly)) {
            break;
        }

        // Ok to ignore failed wzplaylist
        QString msg = file.errorString();
        if (wzplaylist && !isFavList) {
            WZINFO(QString("Ignoring failed save of '%1'. %2")
                   .arg(filename).arg(msg));
            return true;
        }

        WZERROR("Failed to save '" + filename + "'. " + msg);
        int res = QMessageBox::warning(this, tr("Save failed"),
                                       tr("Failed to open '%1' for writing. %2")
                                       .arg(filename).arg(msg),
                                       QMessageBox::Retry | QMessageBox::Default,
                                       QMessageBox::Cancel | QMessageBox::Escape,
                                       QMessageBox::Ignore);
        switch (res) {
            case QMessageBox::Retry:
                file.unsetError();
                continue;
            case QMessageBox::Cancel: return false;
            default: return true;
        }
    } while (true);


    QTextStream stream(&file);
    if (QFileInfo(filename).suffix().toLower() == "m3u") {
        stream.setCodec(QTextCodec::codecForLocale());
    } else {
        stream.setCodec("UTF-8");
    }

    stream << "#EXTM3U" << "\n"
           << "# Playlist created by WZPlayer " << TVersion::version << "\n";

    // Keep track of whether we saved anything usefull
    bool didSaveMeta = false;

    if (wzplaylist && folder->getBlacklistCount() > 0) {
        didSaveMeta = true;
        foreach(const QString& fn, folder->getBlacklist()) {
            WZDEBUGOBJ("Blacklisting '" + fn + "'");
            stream << "#WZP-blacklist:" << fn << "\n";
        }
    }

    bool result = saveM3uFolder(folder, path, stream, wzplaylist, didSaveMeta);

    result = result && stream.status() == QTextStream::Ok;
    stream.flush();
    result = result && stream.status() == QTextStream::Ok;
    file.close();
    result = result && file.error() == QFileDevice::NoError;

    // Remove wzplaylist.m3u8 from disk if nothing interesting to remember
    if (!didSaveMeta && wzplaylist) {
        if (file.remove()) {
            WZINFO(QString("Removed '%1' from disk").arg(filename));
        } else {
            WZWARN(QString("Failed to remove '%1' from disk. %2")
                    .arg(filename).arg(file.errorString()));
        }
        // Ignore result for wzplaylist
        return true;
    }

    if (result) {
        WZINFO("Saved '" + filename + "'");
        msgOSD(tr("Saved %1").arg(QFileInfo(filename).fileName()));
        return true;
    }

    QString emsg = file.errorString();
    QString msg = QString("Failed to save '%1'. %2").arg(filename).arg(emsg);
    WZERROR(msg);
    msg = tr("Failed to save '%1'. %2").arg(filename).arg(emsg);
    int res = QMessageBox::warning(this, tr("Save failed"), msg,
            QMessageBox::Cancel | QMessageBox::Escape | QMessageBox::Default,
            QMessageBox::Ignore);
    if (res == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

bool TPList::saveM3u(const QString& filename, bool linkFolders) {

    TPlaylistItem* root = playlistWidget->root();
    return saveM3u(root, filename, linkFolders);
}

bool TPList::save() {
    WZINFO(QString("Saving '%1'").arg(playlistFilename));

    if (playlistFilename.isEmpty()) {
        return saveAs();
    }

    bool wzplaylist = false;
    QFileInfo fi(playlistFilename);
    if (fi.isDir()) {
        fi.setFile(fi.absoluteFilePath(), TConfig::WZPLAYLIST);
        wzplaylist = true;
    } else if (fi.fileName() == TConfig::WZPLAYLIST) {
        wzplaylist = true;
    } else if (player->mdat.disc.valid) {
        // saveAs() force adds ".m3u8" playlist extension
        if (!extensions.isPlaylist(fi)) {
            return saveAs();
        }
    }

    msgOSD(tr("Saving %1").arg(fi.fileName()), 0);
    setPlaylistFilename(fi.absoluteFilePath());
    Settings::pref->last_dir = fi.absolutePath();

    if (wzplaylist) {
        QString path = fi.absolutePath();
        if (!fi.dir().mkpath(path)) {
            QString msg = strerror(errno);
            WZERROR(QString("Failed to create directory '%1'. %2")
                    .arg(path).arg(msg));
            QMessageBox::warning(this, tr("Error while saving"),
                                 tr("Error while saving. Failed to create"
                                    " directory '%1'. %2")
                                 .arg(path).arg(msg));
            return false;
        }
    }

    bool result = saveM3u(playlistFilename, wzplaylist);
    if (result) {
        playlistWidget->clearModified();
        msgOSD(tr("Saved '%1'").arg(fi.fileName()));
    } else {
        // Error box and log already done, but need to remove 0 secs save msg
        msgOSD(tr("Failed to save '%1'").arg(fi.fileName()));
    }

    return result;
}

bool TPList::saveAs() {

    QString s = TFileDialog::getSaveFileName(this, tr("Choose a filename"),
        Settings::pref->last_dir,
        tr("Playlists") + extensions.playlists().forFilter());

    if (s.isEmpty()) {
        return false;
    }

    // Force add ".m3u8", cause I hate anything not utf8.
    // Save() depends on setting a playlist extension here,
    // for its handling of playlists for discs.
    QFileInfo fi(s);
    if (fi.suffix().toLower() != "m3u8") {
        fi.setFile(s + ".m3u8");
    }

    if (fi.exists()) {
        int res = QMessageBox::question(this, tr("Confirm overwrite"),
                tr("The file %1 already exists.\n"
                   "Do you want to overwrite it?")
                .arg(fi.absoluteFilePath()),
                QMessageBox::Yes,
                QMessageBox::No | QMessageBox::Default | QMessageBox::Escape);
        if (res == QMessageBox::No) {
            return false;
        }
    }

    setPlaylistFilename(fi.absoluteFilePath());
    return save();
}

void TPList::stop() {
    WZINFOOBJ(QString("State %2").arg(player->stateToString()));

    if (playlistWidget->playingItem) {
        playlistWidget->setPlayingItem(0);
    }
    if (player->state() == Player::STATE_STOPPED) {
        if (thread) {
            abortThread();
        } else {
            playlistWidget->abortFileCopier();
        }
    }
    // player->stop() done by TPlaylist::stop()
}

void TPList::play() {
    WZTRACEOBJ("");

    TPlaylistItem* item = playlistWidget->plCurrentItem();
    if (item) {
        playItem(item);
    } else {
        player->play();
    }
}

void TPList::playInNewWindow() {
    WZTRACEOBJ("");

    QStringList files;
    QTreeWidgetItemIterator it(playlistWidget,
                               QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        files << i->filename();
        ++it;
    }

    if (files.count() == 0) {
        if (playlistWidget->plCurrentItem()) {
            files << playlistWidget->plCurrentItem()->filename();
        } else if (!player->mdat.filename.isEmpty()) {
            files << player->mdat.filename;
        }
    }

    // Save settings and modified playlist
    mainWindow->save();

    QProcess p;
    if (p.startDetached(qApp->applicationFilePath(), files)) {
        WZINFO("Started new instance");
    } else {
        QString msg = strerror(errno);
        WZERROR(QString("Failed to start '%1'. %2")
                .arg(qApp->applicationFilePath()).arg(msg));
        QMessageBox::warning(this, tr("Start failed"),
                             tr("Failed to start '%1'. %2")
                             .arg(qApp->applicationFilePath()).arg(msg),
                             QMessageBox::Ok);
    }
}

void TPList::editName() {

    makeActive();
    playlistWidget->editName();
}

void TPList::newFolder() {
    WZTRACEOBJ("");

    TPlaylistItem* parent = playlistWidget->plCurrentItem();
    if (parent == 0) {
        parent = playlistWidget->root();
    } else if (!parent->isFolder()) {
        parent = parent->plParent();
    }

    QString path = parent->filename();
    if (path.isEmpty()) {
        if (QMessageBox::question(this, tr("Save playlist?"),
                tr("To create folders the playlist needs to be saved first."
                   " Do you want to save it now?"),
                QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            if (saveAs()) {
                QTimer::singleShot(0, this, &TPlaylist::newFolder);
            }
        }
        return;
    }

    QFileInfo fi(path);
    if (!fi.exists()) {
        QMessageBox::information(this, tr("Information"),
            tr("Failed to create a new folder. Could not find '%1'").arg(path));
        return;
    }

    if (!fi.isDir()) {
        // TODO: confirm for non wzplaylist?
        path = fi.absolutePath();
    }

    QString baseName = tr("New folder");
    QDir dir(path);
    int i = 2;
    QString name = baseName;
    while (dir.exists(name)) {
        name = baseName + " " + QString::number(i++);
    }

    if (!dir.mkdir(name)) {
        QString error = strerror(errno);
        WZERROR(QString("Failed to create directory '%1' in '%2'. %3")
                .arg(name).arg(path).arg(error));
        QMessageBox::warning (this, tr("Error"),
                              tr("Failed to create folder '%1' in '%2'. %3")
                              .arg(name).arg(path).arg(error));
        return;
    }

    TPlaylistItem* item = new TPlaylistItem(parent, path + "/" + baseName,
                                            baseName, 0, false);
    item->setModified();
    playlistWidget->setCurrentItem(item);
    playlistWidget->editName();
}

void TPList::copySelection(const QString& actionName) {

    QString text;
    int copied = 0;

    QTreeWidgetItemIterator it(playlistWidget,
                               QTreeWidgetItemIterator::Selected);
    while (*it) {
        TPlaylistItem* i = static_cast<TPlaylistItem*>(*it);
        text += i->filename() + "\n";
        copied++;
        it++;
    }

    if (copied == 0 && player->mdat.filename.count()) {
        text = player->mdat.filename + "\n";
        copied = 1;
    }

    if (copied > 0) {
        if (copied == 1) {
            // Remove trailing new line
            text = text.left(text.length() - 1);
            msgOSD(actionName + " " + text);
        } else {
            msgOSD(tr("%1 %2 file names",
                   "Action 'Cut' or 'Copied', number of file names")
                .arg(actionName).arg(copied));
        }
        QApplication::clipboard()->setText(text);
    }
}

void TPList::copySelected() {
    copySelection(tr("Copied"));
}

void TPList::paste() {

    QStringList files = QApplication::clipboard()->text()
                        .split("\n",  QString::SkipEmptyParts);
    if (files.count()) {
        TPlaylistItem* parent = playlistWidget->plCurrentItem();
        if (parent && !parent->isFolder()) {
            parent = parent->plParent();
        }
        add(files, false, parent);
    }
}

void TPList::cut() {
    WZTRACEOBJ("");

    copySelection(tr("Cut"));
    removeSelected();
}

void TPList::addPlayingFile() {
   WZTRACEOBJ("");

   QString fn = player->mdat.filename;
   if (!fn.isEmpty()) {
       add(QStringList() << fn, false, playlistWidget->plCurrentItem());
   }
}

void TPList::addFilesDialog() {

    QStringList files = TFileDialog::getOpenFileNames(this,
        tr("Select one or more files to add"), Settings::pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;" +
        tr("All files") +" (*.*)");

    if (files.count() > 0) {
        add(files, false, playlistWidget->plCurrentItem());
    }
}

void TPList::addDirectoryDialog() {

    QString s = TFileDialog::getExistingDirectory(
                this, tr("Choose a directory"), Settings::pref->last_dir);
    if (!s.isEmpty()) {
        add(QStringList() << s, false, playlistWidget->plCurrentItem());
    }
}

void TPList::addUrlsDialog() {

    TMultilineInputDialog d(this);
    if (d.exec() == QDialog::Accepted && d.lines().count() > 0) {
        add(d.lines(), false, playlistWidget->plCurrentItem());
    }
}

void TPList::removeSelected(bool deleteFromDisk) {

    if (!playlistWidget->hasFocus() && !toolbar->hasFocus()) {
        WZWARN("Ignoring remove action while playlist not focused");
        return;
    }
    if (isBusy()) {
        WZWARN("Ignoring remove request while busy.");
        return;
    }

    WZTRACEOBJ("Disabling enableActions");
    disableEnableActions++;
    playlistWidget->removeSelected(deleteFromDisk);
    // TODO: ...
    if (playlistFilename.isEmpty() && !playlistWidget->hasItems()) {
        // Start with a clean sheet
        playlistWidget->clearModified();
    }
    WZTRACEOBJ("Enabling enableActions");
    disableEnableActions--;
    enableActions();
}

void TPList::removeSelectedFromDisk() {
    removeSelected(true);
}

void TPList::removeAll() {
    clear(false);
}

QUrl TPList::getBrowseURL() {

    QString fn = player->mdat.filename;
    if (dock->isVisible()) {
        TPlaylistItem* item = playlistWidget->plCurrentItem();
        if (item) {
            fn = item->filename();
        }
    }

    QUrl url;
    if (fn.isEmpty()) {
        return url;
    }

    QFileInfo fi(fn);
    if (fi.exists()) {
        if (fi.isDir()) {
            url = QUrl::fromLocalFile(fi.absoluteFilePath());
        } else {
            url = QUrl::fromLocalFile(fi.absolutePath());
        }
    } else {
        url.setUrl(fn);
    }

    return url;
}

void TPList::browseDir() {

    QString fn = player->mdat.filename;
    if (dock->isVisible()) {
        TPlaylistItem* item = playlistWidget->plCurrentItem();
        if (item) {
            fn = item->filename();
        }
    }
    if (fn.isEmpty()) {
        return;
    }

    QUrl url;
    QFileInfo fi(fn);
    if (fi.exists()) {
        if (fi.isDir()) {
            url = QUrl::fromLocalFile(fi.absoluteFilePath());
        } else {
            url = QUrl::fromLocalFile(fi.absolutePath());
        }
    } else {
        url.setUrl(fn);
    }

    if (!QDesktopServices::openUrl(url)) {
        QMessageBox::warning(this, tr("Open URL failed"),
                             tr("Failed to open URL '%1'")
                             .arg(url.toString(QUrl::None)));
    }
}

void TPList::onCurrentItemChanged(QTreeWidgetItem* current,
                                  QTreeWidgetItem* previous) {
    WZTRACEOBJ(QString("Changed from '%1' to '%2'")
            .arg(previous ? previous->text(TPlaylistItem::COL_NAME) : "null")
            .arg(current ? current->text(TPlaylistItem::COL_NAME) : "null"));
    enableActions();
}

void TPList::onItemActivated(QTreeWidgetItem* i, int) {
    WZTRACEOBJ("");

    TPlaylistItem* item = static_cast<TPlaylistItem*>(i);
    if (item && !item->isFolder()) {
        playItem(item);
    }
}

void TPList:: setPLaylistTitle() {

    QString title;
    if (!isFavList) {
        TPlaylistItem* root = playlistWidget->root();
        if (root) {
            title = root->baseName();
        }
    }

    title = tr("%1%2%3%4",
               "1 Playlist or Favorites translated string,"
               " 2 optional space if playlist has name,"
               " 3 optional playlist name,"
               " 4 optional modified star")
            .arg(tranName)
            .arg(title.isEmpty() ? "" : " ")
            .arg(title)
            .arg(playlistWidget->isModified() ? "*" : "");

    dock->setWindowTitle(title);
}

void TPList::abortThread() {

    if (thread) {
        WZINFOOBJ("Aborting add files thread");
        addStartPlay = false;
        restartThread = false;
        thread->abort();
    }
}

void TPList::onThreadFinished() {
    WZTRACEOBJ("");

    if (thread == 0) {
        // Only during destruction, so no need to enable actions
        WZDEBUGOBJ("Thread is gone");
        return;
    }

    // Get data from thread
    TPlaylistItem* root = thread->root;
    thread->root = 0;
    if (!thread->latestDir.isEmpty() && !isFavList) {
        Settings::pref->last_dir = thread->latestDir;
    }

    // Clean up
    delete thread;
    thread = 0;

    if (root == 0) {
        // Thread aborted
        if (restartThread) {
            WZDEBUGOBJ("Thread aborted, starting new request");
            addStartThread();
        } else {
            WZDEBUGOBJ("Thread aborted");
            addFiles.clear();
            enableActions();
        }
        return;
    }

    QString msg = addFiles.count() == 1 ? addFiles.at(0) : "";
    addFiles.clear();

    // Found nothing to play?
    if (root->childCount() == 0) {
        delete root;
        if (msg.isEmpty()) {
            msg = tr("Found nothing to play.");
        } else {
            msg = tr("Found no files to play in '%1'.").arg(msg);
        }
        WZINFO(msg);
        if (!isFavList) {
            QMessageBox::information(this, tr("Nothing to play"), msg);
        }
        enableActions();
        return;
    }

    // Returns a newly created root when all items are replaced
    // or 0 when the new items are inserted into the existing root.
    root = playlistWidget->add(root, addTarget);
    if (root) {
        if (isFavList) {
            setPlaylistFilename(Settings::TPaths::favoritesFilename());
        } else {
            playlistFilename = root->filename();
            WZINFOOBJ("Filename set to '" + playlistFilename + "'");
            setPLaylistTitle();
            Settings::pref->addRecent(playlistFilename, root->fname());
        }
    }

    emit addedItems();

    if (addStartPlay) {
        if (!addFileToPlay.isEmpty()) {
            TPlaylistItem* item = playlistWidget->findFilename(addFileToPlay);
            if (item) {
                playItem(item);
                return;
            }
        }
        startPlay();
        return;
    }

    enableActions();
}

void TPList::addStartThread() {

    if (thread) {
        // Thread still running, abort it and restart it in onThreadFinished()
        WZDEBUGOBJ("Add files thread still running. Aborting it...");
        restartThread = true;
        thread->abort();
    } else {
        WZDEBUGOBJ("Starting add files thread");
        restartThread = false;

        // Allow single image
        bool addImages = Settings::pref->addImages
                         || ((addFiles.count() == 1)
                             && extensions.isImage(addFiles.at(0)));

        thread = new TAddFilesThread(this,
                                     addFiles,
                                     Settings::pref->nameBlacklist,
                                     Settings::pref->addDirectories,
                                     Settings::pref->addVideo,
                                     Settings::pref->addAudio,
                                     Settings::pref->addPlaylists,
                                     addImages,
                                     isFavList);

        connect(thread, &TAddFilesThread::finished,
                this, &TPList::onThreadFinished);
        connect(thread, &TAddFilesThread::displayMessage,
                msgSlot, &TMsgSlot::msg);

        thread->start();
        enableActions();
    }
}

void TPList::add(const QStringList& files,
                    bool startPlay,
                    TPlaylistItem* target,
                    const QString& fileToPlay) {
    debug << "add files" << files << "startPlay" << startPlay << debug;

    addFiles = files;
    addStartPlay = startPlay;
    addTarget = target;
    addFileToPlay = fileToPlay;

    addStartThread();
}

void TPList::clear(bool clearFilename) {

    abortThread();
    bool hadItems = playlistWidget->hasItems();
    playlistWidget->clr();

    if (clearFilename) {
        playlistFilename = "";
    } else {
        TPlaylistItem* root = playlistWidget->root();
        root->setFilename(playlistFilename);
        if (hadItems) {
            root->setModified();
        }
    }

    setPLaylistTitle();
}

void TPList::addRemovedItem(const QString& item) {
    WZTRACEOBJ("'" + item + "'");

    // TODO: just add the item instead of refresh
    refresh();
}

void TPList::saveSettings() {

    Settings::pref->beginGroup(objectName());
    playlistWidget->saveSettings(Settings::pref);
    toolbar->saveSettings();
    Settings::pref->setValue("toolbar_visible",
                             toolbar->toggleViewAction()->isChecked());
    Settings::pref->endGroup();
}

void TPList::loadSettings() {

    Settings::pref->beginGroup(objectName());
    playlistWidget->loadSettings(Settings::pref);
    toolbar->loadSettings();
    toolbar->toggleViewAction()->setChecked(
                Settings::pref->value("toolbar_visible", true).toBool());
    Settings::pref->endGroup();

    // Set shortcut context
    Qt::ShortcutContext context = Qt::WidgetWithChildrenShortcut;
    QList<QAction*> acts = actions();
    for(int i = 0; i< acts.count(); i++) {
        QAction* action = acts.at(i);
        QString name =  action->objectName();
        if (!name.isEmpty()) {
            action->setShortcutContext(context);
            WZTRACEOBJ(name);
        }
    }
}


} // namespace Playlist
} // namespace Gui