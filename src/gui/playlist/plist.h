#ifndef GUI_PLAYLIST_PLIST_H
#define GUI_PLAYLIST_PLIST_H

#include <QWidget>
#include "gui/action/menu/menu.h"
#include "wzdebug.h"


class QToolBar;
class QToolButton;
class QTreeWidgetItem;
class QTextStream;

namespace Gui {

class TMainWindow;
class TDockWidget;

namespace Action {
class TAction;
class TEditableToolbar;
}

namespace Playlist {

class TAddFilesThread;
class TPlaylistWidget;
class TPlaylistItem;
class TMenuAddRemoved;

class TPList : public QWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER
    friend class TMenuAddRemoved;
public:
    explicit TPList(TDockWidget* parent,
                    TMainWindow* mw,
                    const QString& name,
                    const QString& aShortName,
                    const QString& aTransName);
    virtual ~TPList() override;

    void abortThread();
    void add(const QStringList& files,
             bool startPlay = false,
             TPlaylistItem* target = 0,
             const QString& fileToPlay = "");
    bool isBusy() const;

    virtual void startPlay() = 0;
    bool maybeSave();

    virtual void loadSettings();
    virtual void saveSettings();


public slots:
    virtual void enableActions();

    virtual void stop();
    virtual void findPlayingItem() = 0;

signals:
    void addedItems();

protected:
    TMainWindow* mainWindow;
    TDockWidget* dock;
    TPlaylistWidget* playlistWidget;
    Action::TEditableToolbar* toolbar;
    QString playlistFilename;
    TAddFilesThread* thread;
    int disableEnableActions;

    QAction* openAct;
    Action::TAction* saveAct;
    Action::TAction* saveAsAct;
    Action::TAction* refreshAct;
    Action::TAction* browseDirAct;

    Action::TAction* playAct;
    Action::TAction* playInNewWindowAct;

    Action::TAction* editNameAct;
    Action::TAction* newFolderAct;
    Action::TAction* findPlayingAct;

    Action::TAction* cutAct;
    Action::TAction* copyAct;
    Action::TAction* pasteAct;

    Action::Menu::TMenu* playlistAddMenu;
    Action::TAction* addPlayingFileAct;

    Action::Menu::TMenu* playlistRemoveMenu;
    Action::TAction* removeSelectedAct;
    Action::TAction* removeSelectedFromDiskAct;
    Action::TAction* removeAllAct;

    void makeActive();

    void clear(bool clearFilename = true);
    void setPlaylistFilename(const QString& filename);

    virtual void playItem(TPlaylistItem* item, bool keepPaused = false) = 0;
    void openPlaylist(const QString& filename);

protected slots:
    virtual void openPlaylistDialog();
    bool save();
    virtual bool saveAs();
    void play();
    virtual void refresh() = 0;

    void setPLaylistTitle();

private:
    QString shortName;
    QString tranName;

    QToolButton* add_button;
    QToolButton* remove_button;

    QStringList addFiles;
    TPlaylistItem* addTarget;
    QString addFileToPlay;
    bool addStartPlay;
    bool restartThread;

    bool isFavList;
    bool skipRemainingMessages;

    void createTree();
    void createActions();
    void createToolbar();

    void enableRemoveFromDiskAction();

    QUrl getBrowseURL();
    void copySelection(const QString& actionName);

    void addStartThread();

    bool saveM3uFolder(TPlaylistItem* folder,
                       const QString& path,
                       QTextStream& stream,
                       bool linkFolders,
                       bool& savedMetaData);
    bool saveM3u(TPlaylistItem* folder,
                 const QString& filename,
                 bool wzplaylist);
    bool saveM3u(const QString& filename, bool linkFolders);

private slots:
    void playInNewWindow();
    void editName();
    void newFolder();

    void cut();
    void copySelected();
    void enablePaste();
    void paste();

    void addPlayingFile();
    void addFilesDialog();
    void addDirectoryDialog();
    void addUrlsDialog();
    void addRemovedItem(const QString& s);

    void removeSelected(bool deleteFromDisk = false);
    void removeSelectedFromDisk();
    void removeAll();

    void enableRemoveMenu();

    void browseDir();

    void onCurrentItemChanged(QTreeWidgetItem* current,
                              QTreeWidgetItem* previous);
    void onItemActivated(QTreeWidgetItem* i, int column);
    void onThreadFinished();
};

class TMenuAddRemoved : public Action::Menu::TMenu {
    Q_OBJECT
public:
    explicit TMenuAddRemoved(TPList* pl,
                             TMainWindow* mw,
                             TPlaylistWidget* plw,
                             const QString& name);
signals:
    void addRemovedItem(QString s);
private:
    TPlaylistWidget* playlistWidget;
    TPlaylistItem* item;

private slots:
    void onAboutToShow();
    void onCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*);
    void onTriggered(QAction* action);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLIST_H