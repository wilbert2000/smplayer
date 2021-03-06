#include "gui/playlist/plist.h"
#include "gui/playlist/playlistwidget.h"
#include "gui/action/menu/menu.h"
#include "gui/action/action.h"
#include "gui/action/editabletoolbar.h"
#include "gui/mainwindow.h"
#include "gui/dockwidget.h"
#include "gui/filedialog.h"
#include "gui/multilineinputdialog.h"
#include "gui/msg.h"
#include "player/player.h"
#include "extensions.h"
#include "iconprovider.h"
#include "wzfiles.h"
#include "name.h"
#include "version.h"
#include "wzdebug.h"

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

TMenuAddRemoved::TMenuAddRemoved(TPList* pl, const QString& name) :
    TMenu(pl, name, tr("Add removed item"), "noicon"),
    plist(pl) {

    menuAction()->setIcon(iconProvider.trashIcon);

    connect(this, &TMenuAddRemoved::triggered,
            this, &TMenuAddRemoved::onTriggered);
    connect(this, &TMenuAddRemoved::aboutToShow,
            this, &TMenuAddRemoved::onAboutToShow);
    connect(plist->getPlaylistWidget(), &TPlaylistWidget::currentItemChanged,
            this, &TMenuAddRemoved::onCurrentItemChanged);

    setEnabled(false);
}

void TMenuAddRemoved::onAboutToShow() {

    clear();
    int c = 0;
    parentItem = plist->getPlaylistWidget()->plCurrentItem();
    if (parentItem) {
        if (!parentItem->isFolder()) {
            parentItem = parentItem->plParent();
        }
        if (parentItem) {
            foreach(const QString& s, parentItem->getBlacklist()) {
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
        parentItem = plist->getPlaylistWidget()->validateItem(parentItem);
        if (parentItem && parentItem->whitelist(s)) {
            QString fn = parentItem->path() + "/" + s;
            plist->getPlaylistWidget()->addFiles(QStringList() << fn,
                                                 parentItem);
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


LOG4QT_DECLARE_STATIC_LOGGER(logger, Gui::Playlist::TPList)

TPList::TPList(TDockWidget* parent,
               const QString& name,
               const QString& aShortName,
               const QString& aTransName) :
    QWidget(parent),
    dock(parent),
    disableEnableActions(0),
    reachedEndOfPlaylist(false),
    shortName(aShortName),
    tranName(aTransName),
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

void TPList::createTree() {

    playlistWidget = new TPlaylistWidget(this, mainWindow,
                                         objectName() + "_widget",
                                         shortName, tranName, isFavList);

    connect(playlistWidget, &TPlaylistWidget::itemActivated,
            this, &TPList::onItemActivated);
    connect(playlistWidget, &TPlaylistWidget::currentItemChanged,
            this, &TPList::onCurrentItemChanged);
    connect(playlistWidget, &TPlaylistWidget::modifiedChanged,
            this, &TPList::setPLaylistTitle,
            Qt::QueuedConnection);

    connect(playlistWidget, &TPlaylistWidget::rootFilenameChanged,
            this, &TPList::onRootFilenameChanged);
    connect(playlistWidget, &TPlaylistWidget::busyChanged,
            this, &TPList::enableActions);
    connect(playlistWidget, &TPlaylistWidget::busyChanged,
            this, &TPList::busyChanged);
    connect(playlistWidget, &TPlaylistWidget::startPlay,
            this, &TPList::startPlay);
    connect(playlistWidget, &TPlaylistWidget::playItem,
            this, &TPList::playItemNoPause);
}

void TPList::createActions() {

    using namespace Action;

    QString tranNameLower = tranName.toLower();
    QObject* owner;
    if (isFavList) owner = this; else owner = mainWindow;

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
    saveAsAct = new TAction(owner, shortName + "_save_as",
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
    browseDirAct->setIcon(iconProvider.browseURLIcon);
    connect(browseDirAct, &TAction::triggered, this, &TPList::browseDir);

    // Play
    playAct = new TAction(this, shortName + "_play", tr("Play"), "play",
                          Qt::SHIFT | Qt::Key_Space);
    playAct->addShortcut(Qt::Key_MediaPlay);
    connect(playAct, &Action::TAction::triggered, this, &TPList::play);

    // Play in new window
    playInNewWindowAct = new TAction(owner, shortName + "_play_in_new_win",
                             tr("Play in new window"), "play",
                             Qt::CTRL | Qt::Key_Space);
    connect(playInNewWindowAct, &TAction::triggered,
            this, &TPList::playInNewWindow);

    // Context menu
    Action::Menu::TMenu* contextMenu = new Action::Menu::TMenu(this,
        shortName + "_context_menu", tr("%1 context menu").arg(tranName));
    connect(contextMenu, &Action::Menu::TMenu::aboutToShow,
            this, &TPList::enableActions);

    contextMenu->addAction(playAct);
    contextMenu->addAction(playInNewWindowAct);

    // Find playing
    findPlayingAct = new TAction(owner, shortName + "_find_playing",
                                 tr("Find playing item"), "noicon", Qt::Key_F3);
    findPlayingAct->setIcon(iconProvider.findIcon);
    connect(findPlayingAct, &TAction::triggered,
            this, &TPList::findPlayingItem);
    contextMenu->addAction(findPlayingAct);

    contextMenu->addSeparator();
    // Edit name
    editNameAct = new TAction(owner, shortName + "_edit_name",
                              tr("Edit name..."), "", Qt::Key_F2);
    connect(editNameAct, &TAction::triggered,
            this, &TPList::editName);
    contextMenu->addAction(editNameAct);

    // Reset name
    resetNameAct = new TAction(this, shortName + "_clear_name",
                               tr("Reset name to filename"));
    connect(resetNameAct, &TAction::triggered,
            this, &TPList::resetName);
    contextMenu->addAction(resetNameAct);

    // Edit URL
    editURLAct = new TAction(owner, shortName + "_edit_url",
                             tr("Edit url..."), "", Qt::CTRL | Qt::Key_F2);
    connect(editURLAct, &TAction::triggered,
            this, &TPList::editURL);
    contextMenu->addAction(editURLAct);

    // New folder
    newFolderAct = new TAction(this, shortName + "_new_folder",
                               tr("New folder"), "noicon", Qt::Key_F10);
    newFolderAct->setIcon(iconProvider.newFolderIcon);
    connect(newFolderAct, &TAction::triggered, this, &TPList::newFolder);
    contextMenu->addAction(newFolderAct);

    contextMenu->addSeparator();
    // Cut
    cutAct = new TAction(this, shortName + "_cut", tr("Cut file name(s)"),
                         "noicon", QKeySequence("Ctrl+X"));
    cutAct->setIcon(iconProvider.cutIcon);
    connect(cutAct, &TAction::triggered, this, &TPList::cut);
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
    playlistAddMenu = new Menu::TMenu(this, shortName + "_add_menu",
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
            this, &TPList::addPlayingFile);

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
    playlistAddMenu->addMenu(new TMenuAddRemoved(
                                 this, shortName + "_add_rm_menu"));

    contextMenu->addMenu(playlistAddMenu);

    // Remove menu
    playlistRemoveMenu = new Menu::TMenu(this, shortName + "_remove_menu",
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
            this, &TPList::removeSelected);

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
            this, &TPList::removeSelectedFromDisk);

    // Clear playlist
    removeAllAct = new TAction(this, shortName + "_clear",
        tr("Clear %1").arg(tranNameLower) + (isFavList ? "..." : ""),
        "noicon", Qt::CTRL | Qt::Key_Delete);
    removeAllAct->setIcon(iconProvider.clearIcon);
    playlistRemoveMenu->addAction(removeAllAct);
    connect(removeAllAct, &TAction::triggered, this, &TPList::removeAll);

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

void TPList::onRootFilenameChanged(QString rootFilename) {

    playlistFilename = rootFilename;
    setPLaylistTitle();
}

void TPList::setContextMenuToolbar(Action::Menu::TMenu* menu) {

    toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(toolbar, &QToolBar::customContextMenuRequested,
            menu, &Action::Menu::TMenu::execSlot);
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
                && !current->isLink()
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

void TPList::enableActionsCurrentItem() {
    WZTOBJ;

    TPlaylistItem* cur = playlistWidget->plCurrentItem();
    bool haveFile = !player->mdat.filename.isEmpty();
    browseDirAct->setEnabled(haveFile || cur);
    browseDirAct->setText(tr("Browse %1")
                          .arg(cur
                               ? (cur->isUrl() ? tr("URL") : tr("folder"))
                               : tr("the void")));
    browseDirAct->setToolTip(tr("Browse '%1'")
                             .arg(getBrowseURL().toDisplayString()));

    bool enable = !isBusy() && player->stateReady();
    editNameAct->setEnabled(enable && cur);
    resetNameAct->setEnabled(enable && cur && cur->edited());
    newFolderAct->setEnabled(enable);

    cutAct->setEnabled(enable && cur);
    copyAct->setEnabled(haveFile || cur);

    enableRemoveMenu();
}

void TPList::onCurrentItemChanged(QTreeWidgetItem* current,
                                  QTreeWidgetItem* previous) {
    WZTOBJ << "Changed from"
           << (previous ? previous->text(TPlaylistItem::COL_NAME) : "0")
           << (current ? current->text(TPlaylistItem::COL_NAME) : "0");

    enableActionsCurrentItem();
}

void TPList::enableActions() {

    openAct->setEnabled(player->state() != Player::STATE_STOPPING);

    // saveAct->setEnabled(true);
    // saveAsAct->setEnabled(true);
    refreshAct->setEnabled(!playlistFilename.isEmpty());

    bool haveFile = !player->mdat.filename.isEmpty();
    playAct->setEnabled(haveFile || playlistWidget->hasPlayableItems());
    playInNewWindowAct->setEnabled(haveFile || playlistWidget->hasItems());
    newFolderAct->setEnabled(!isBusy() && player->stateReady());
    // findPlayingAct by descendants
    enablePaste();
    addPlayingFileAct->setEnabled(haveFile);

    enableActionsCurrentItem();
}

bool TPList::isBusy() const {
    return playlistWidget->isBusy();
}

bool TPList::hasPlayableItems() const {
    return playlistWidget->hasPlayableItems();
}

void TPList::makeActive() {

    if (!dock->isVisible()) {
        dock->setVisible(true);
    }
    dock->raise();
    activateWindow();
    playlistWidget->setFocus();
}

void TPList::setPlaylistFilename(const QString& filename) {

    playlistFilename = QDir::toNativeSeparators(filename);
    WZTOBJ << "Playlist filename set to" << playlistFilename;
    playlistWidget->root()->setFilename(playlistFilename);
    setPLaylistTitle();
}

bool TPList::maybeSave() {

    if (!playlistWidget->isModified()) {
        return true;
    }

    if (playlistFilename.isEmpty()) {
        int count = playlistWidget->root()->childCount();
        if (count == 0) {
            return true;
        }
        if (count == 1 && playlistWidget->root()->child(0)->childCount() == 0) {
            return true;
        }

        int res = QMessageBox::question(this, tr("%1 modified").arg(tranName),
            tr("The playlist has been modified, do you want to save it?"),
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
                return saveAs();
        }
    }

    QFileInfo fi(playlistFilename);
    if (fi.fileName().compare(TConfig::WZPLAYLIST, caseSensitiveFileNames)
            == 0) {
        return save(!isFavList);
    }

    if (fi.isDir()) {
        if (isFavList || Settings::pref->useDirectoriePlaylists) {
            return save(!isFavList);
        }
        WZDEBUGOBJ("Discarding changes. Saving directorie playlists is"
                   " disabled.");
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
            return save(false);
    }
}

void TPList::openPlaylist(const QString& filename) {

    Settings::pref->last_dir = QFileInfo(filename).absolutePath();
    clear();
    playlistWidget->addFiles(QStringList() << filename, 0, -1, true);
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
                           bool allowFail,
                           bool& savedMetaData) {
    WZTRACEOBJ(QString("Saving '%1'").arg(folder->filename()));

    bool result = true;
    for(int idx = 0; idx < folder->childCount(); idx++) {
        TPlaylistItem* item = folder->plChild(idx);
        QString filename = item->filename();

        if (item->isPlaylist()) {
            if (item->modified()) {
                if (!saveM3uFile(item, item->isWZPlaylist(), allowFail)) {
                    result = false;
                }
            }
        } else if (item->isFolder()) {
            if (linkFolders) {
                if (item->modified()) {
                    QFileInfo fi(filename, TConfig::WZPLAYLIST);
                    filename = QDir::toNativeSeparators(fi.absoluteFilePath());
                    item->setFilename(filename);
                    if (!saveM3uFile(item, linkFolders, allowFail)) {
                        result = false;
                    }
                }
            } else {
                // Note: savedMetaData destroyed as dummy here. It is only used
                // for WZPlaylists which have linkFolders set to true.
                if (saveM3uFolder(item, path, stream, linkFolders, allowFail,
                                  savedMetaData)) {
                    WZINFO("Succesfully saved '" + filename + "'");
                } else {
                    result = false;
                }
                // Files saved inside this playlist, so continue at top
                continue;
            }
        } else {
            // Version 3
            // double d = double(item->durationMS()) / 1000;
            int d = qRound(double(item->durationMS()) / 1000);
            stream << "#EXTINF:" << d << "," << item->baseName() << "\n";
            if (!savedMetaData) {
                if (isFavList || d || item->edited()) {
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

bool TPList::saveM3uFile(TPlaylistItem* folder,
                         bool linkFolders,
                         bool allowFail) {

    QString filename = folder->filename();
    WZTRACEOBJ(QString("Saving '%1'").arg(filename));

    QString path = QDir::toNativeSeparators(QFileInfo(filename).dir().path());
    if (!path.endsWith(QDir::separator())) {
        path += QDir::separator();
    }

    // Note: QFile does not support native seps
    QFile file(QFileInfo(filename).absoluteFilePath());
    do {
        if (file.open(QIODevice::WriteOnly)) {
            break;
        }

        QString msg = file.errorString();
        WZWARN("Failed to open '" + filename + "' for writing. " + msg);

        if (allowFail) {
            return false;
        }

        int res = QMessageBox::warning(this, tr("Save failed"),
            tr("Failed to open '%1' for writing. %2").arg(filename).arg(msg),
            QMessageBox::Retry | QMessageBox::Default,
            QMessageBox::Cancel | QMessageBox::Escape,
            QMessageBox::Ignore);
        switch (res) {
            case QMessageBox::Retry:
                file.unsetError();
                continue;
            case QMessageBox::Ignore:
                folder->setModified(false, true, false);
                return true;
            default:
                return false;
        }
    } while (true);


    QTextStream stream(&file);
    // Need . as decimal separator when writing version 3
    // stream.setLocale(QLocale::c());
    stream.setCodec("UTF-8");

    stream << "#EXTM3U\n"
           // No longer writing version 3 files
           // << "#EXT-X-VERSION:3\n"
           << "# Playlist created by WZPlayer " << TVersion::version << "\n";

    // Keep track of whether we saved anything usefull
    bool didSaveMeta = false;

    if (linkFolders && folder->getBlacklistCount() > 0) {
        didSaveMeta = true;
        foreach(const QString& fn, folder->getBlacklist()) {
            WZDEBUGOBJ("Blacklisting '" + fn + "'");
            stream << "#WZP-blacklist:" << fn << "\n";
        }
    }

    // Save folder
    bool result = saveM3uFolder(folder, path, stream, linkFolders, allowFail,
                                didSaveMeta);

    result = result && stream.status() == QTextStream::Ok;
    stream.flush();
    result = result && stream.status() == QTextStream::Ok;
    file.close();
    result = result && file.error() == QFileDevice::NoError;

    if (result) {
        // Remove wzplaylist.m3u8 from disk if nothing interesting to remember
        if (!didSaveMeta && linkFolders) {
            if (file.remove()) {
                WZINFO(QString("Removed '%1' from disk").arg(filename));
            } else {
                WZWARN(QString("Failed to remove '%1' from disk. %2")
                       .arg(filename).arg(file.errorString()));
            }
        } else {
            WZINFO("Saved '" + filename + "'");
        }

        msg(tr("Saved %1").arg(QFileInfo(filename).fileName()));
        return true;
    }

    QString emsg = file.errorString();
    QString msg = QString("Failed to save '%1'. %2").arg(filename).arg(emsg);
    WZERROR(msg);
    msg = tr("Failed to save '%1'. %2").arg(filename).arg(emsg);
    if (allowFail) {
       Gui::msg(msg);
       return false;
    }
    int res = QMessageBox::warning(this, tr("Save failed"), msg,
            QMessageBox::Cancel | QMessageBox::Escape | QMessageBox::Default,
            QMessageBox::Ignore);
    if (res == QMessageBox::Cancel) {
        return false;
    }

    folder->setModified(false, true, false);
    return true;
}

bool TPList::saveM3u(bool allowFail) {

    // Save sort section and order
    int savedSortSection = playlistWidget->sortSection;
    Qt::SortOrder savedSortOrder = playlistWidget->sortOrder;

    // Set sort to COL_ORDER
    if (isFavList) {
        playlistWidget->setSort(TPlaylistItem::COL_ORDER, Qt::AscendingOrder);
    }

    // Save tree
    TPlaylistItem* root = playlistWidget->root();
    bool result = saveM3uFile(root, root->isWZPlaylist(), allowFail);

    // Restore sort
    if (isFavList) {
        playlistWidget->setSort(savedSortSection, savedSortOrder);
    }

    return result;
}

bool TPList::save(bool allowFail) {
    WZINFO(QString("Saving '%1'").arg(playlistFilename));

    if (playlistFilename.isEmpty()) {
        return saveAs();
    }

    QFileInfo fi(playlistFilename);
    if (fi.isDir()) {
        fi.setFile(fi.absoluteFilePath(), TConfig::WZPLAYLIST);
    }
    // TODO:
    //else if (player->mdat.disc.valid) {
    //    // saveAs() force adds ".m3u8" playlist extension
    //    if (!extensions.isPlaylist(fi)) {
    //        return saveAs();
    //    }
    //}

    setPlaylistFilename(fi.absoluteFilePath());
    TPlaylistItem* root = playlistWidget->root();
    msg(tr("Saving %1").arg(root->baseName()), 0);
    Settings::pref->last_dir = fi.absolutePath();

    if (root->isWZPlaylist()) {
        QString path = fi.absolutePath();
        if (!fi.dir().mkpath(path)) {
            QString msg = strerror(errno);
            WZERROR(QString("Failed to create directory '%1'. %2")
                    .arg(path).arg(msg));
            if (allowFail) {
                return true;
            }
            QMessageBox::warning(this, tr("Error while saving"),
                                 tr("Error while saving. Failed to create"
                                    " directory '%1'. %2").arg(path).arg(msg));
            return false;;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    bool result = saveM3u(allowFail);
    if (result) {
        playlistWidget->clearModified();
        msg(tr("Saved '%1'").arg(root->baseName()));
    } else if (allowFail) {
        playlistWidget->clearModified();
        WZINFO(QString("Ignoring failed save of '%1'").arg(root->filename()));
        msg(tr("Ignoring failed save of '%1'").arg(root->baseName()));
        result = true;
    } else {
        msg(tr("Failed to save '%1'").arg(root->baseName()));
    }

    QApplication::restoreOverrideCursor();

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
    return save(false);
}

void TPList::stop() {
    WZINFOOBJ(QString("State %2").arg(player->stateToString()));

    playlistWidget->abort();
    // player->stop() done by TPlaylist::stop()
}

TPlaylistItem* TPList::getRandomItem() const {

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

void TPList::startPlay() {
    WZTOBJ;

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

void TPList::playItemNoPause(TPlaylistItem *item) {
    playItem(item);
}

void TPList::playEx() {
    WZTRACEOBJ("");

    if (reachedEndOfPlaylist && playlistWidget->hasPlayableItems()) {
        playNext(true);
    } else if (playlistWidget->playingItem) {
        playItem(playlistWidget->playingItem);
    } else if (player->mdat.filename.isEmpty()) {
        startPlay();
    } else {
        TPlaylistItem* item = playlistWidget->findFilename(player->mdat.filename);
        if (item) {
            playItem(item);
        } else {
            player->play();
        }
    }
}

void TPList::play() {
    WZTRACEOBJ("");

    TPlaylistItem* item = playlistWidget->plCurrentItem();
    if (item
            && dock->isActiveWindow()
            && !dock->visibleRegion().isEmpty()) {
        playItem(item);
    } else {
        playEx();
    }
}

void TPList::playNext(bool loop_playlist) {
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

void TPList::playPrev() {
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

void TPList::resetName() {

    TPlaylistItem* current = playlistWidget->plCurrentItem();
    if (current) {
        current->resetName();
    }
}

void TPList::editURL() {

    makeActive();
    playlistWidget->editURL();
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
                QTimer::singleShot(0, this, &TPList::newFolder);
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
        path = fi.absolutePath();
    }

    QString name = TWZFiles::getNewFilename(path, tr("New folder"));
    if (!QDir(path).mkdir(name)) {
        QString error = strerror(errno);
        WZERROR(QString("Failed to create directory '%1' in '%2'. %3")
                .arg(name).arg(path).arg(error));
        QMessageBox::warning (this, tr("Error"),
                              tr("Failed to create folder '%1' in '%2'. %3")
                              .arg(name).arg(path).arg(error));
        return;
    }

    TPlaylistItem* item = new TPlaylistItem(parent, path + "/" + name,
                                            name, 0, false);
    item->setModified();
    playlistWidget->setCurrentItem(item);
    playlistWidget->editName();
}

bool TPList::findPlayingItem() {

    TPlaylistItem* i = playlistWidget->playingItem;
    if (i) {
        makeActive();
        if (i != playlistWidget->currentItem()) {
            playlistWidget->setCurrentItem(i);
        }
        playlistWidget->scrollToItem(i);
        return true;
    }
    return false;
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
        playlistWidget->addFiles(files, parent);
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
       playlistWidget->addFiles(QStringList() << fn,
                                playlistWidget->plCurrentItem());
   }
}

void TPList::addFilesDialog() {

    QStringList files = TFileDialog::getOpenFileNames(this,
        tr("Select one or more files to add"), Settings::pref->last_dir,
        tr("Multimedia") + extensions.allPlayable().forFilter() + ";;" +
        tr("All files") +" (*.*)");

    if (files.count() > 0) {
        playlistWidget->addFiles(files, playlistWidget->plCurrentItem());
    }
}

void TPList::addDirectoryDialog() {

    QString s = TFileDialog::getExistingDirectory(
                this, tr("Choose a directory"), Settings::pref->last_dir);
    if (!s.isEmpty()) {
        playlistWidget->addFiles(QStringList() << s,
                                 playlistWidget->plCurrentItem());
    }
}

void TPList::addUrlsDialog() {

    TMultilineInputDialog d(this);
    if (d.exec() == QDialog::Accepted && d.lines().count() > 0) {
        playlistWidget->addFiles(d.lines(), playlistWidget->plCurrentItem());
    }
}

void TPList::removeSelected(bool deleteFromDisk) {

    if (!playlistWidget->hasFocus() && !toolbar->hasFocus()) {
        WZWARN("Ignoring remove action while playlist not focused");
        return;
    }
    if (isBusy()) {
        WZWARN("Ignoring remove action while busy.");
        return;
    }

    WZTRACEOBJ("");
    disableEnableActions++;
    playlistWidget->removeSelected(deleteFromDisk);
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
    playlistWidget->root()->setModified(!playlistFilename.isEmpty());
}

QUrl TPList::getBrowseURL() {

    QString fn = player->mdat.filename;
    if (!dock->visibleRegion().isEmpty()) {
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

    QUrl url = getBrowseURL();
    if (url.isValid()) {
        if (!QDesktopServices::openUrl(url)) {
            QMessageBox::warning(this, tr("Open URL failed"),
                                 tr("Failed to open URL '%1'")
                                 .arg(url.toString(QUrl::None)));
        }
    } else {
        QMessageBox::warning(this, tr("URL vaildation failed"),
                             tr("Failed to validate URL '%1'")
                             .arg(url.toString(QUrl::None)));
    }
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

void TPList::clear(bool clearFilename) {

    playlistWidget->clr();

    if (clearFilename) {
        playlistFilename = "";
    } else {
        playlistWidget->root()->setFilename(playlistFilename);
    }

    setPLaylistTitle();
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
    for(int i = 0; i < acts.count(); i++) {
        acts.at(i)->setShortcutContext(context);
    }
}


} // namespace Playlist
} // namespace Gui
