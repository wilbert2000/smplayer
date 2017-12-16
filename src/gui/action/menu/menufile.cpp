#include "gui/action/menu/menufile.h"
#include <QMessageBox>
#include <QFileInfo>

#include "gui/mainwindow.h"
#include "gui/playlist/playlist.h"
#include "gui/action/menu/favorites.h"
#include "gui/action/action.h"
#include "settings/paths.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

class TMenuDisc : public TMenu {
public:
    explicit TMenuDisc(TMainWindow* parent);
};

TMenuDisc::TMenuDisc(TMainWindow* parent)
    : TMenu(parent, parent, "disc_menu", tr("Open d&isc"), "open_disc") {

    // DVD
    TAction* a = new TAction(this, "open_dvd", tr("&DVD from drive"), "dvd");
    connect(a, SIGNAL(triggered()), parent, SLOT(openDVD()));

    a = new TAction(this, "open_dvd_folder", tr("D&VD from folder..."),
                    "dvd_hd");
    connect(a, SIGNAL(triggered()), parent, SLOT(openDVDFromFolder()));

    // BluRay
    a = new TAction(this, "open_bluray", tr("&Blu-ray from drive"), "bluray");
    connect(a, SIGNAL(triggered()), parent, SLOT(openBluRay()));

    a = new TAction(this, "open_bluray_folder", tr("Blu-&ray from folder..."),
                    "bluray_hd");
    connect(a, SIGNAL(triggered()), parent, SLOT(openBluRayFromFolder()));

    // VCD
    a = new TAction(this, "open_vcd", tr("V&CD"), "vcd");
    connect(a, SIGNAL(triggered()), parent, SLOT(openVCD()));

    // Audio
    a = new TAction(this, "open_audio_cd", tr("&Audio CD"), "cdda");
    connect(a, SIGNAL(triggered()), parent, SLOT(openAudioCD()));

    addActionsTo(parent);
}


TMenuFile::TMenuFile(TMainWindow* mw) :
    TMenu(mw, mw, "file_menu", tr("&File"), "noicon") {

    // Favorites
    TFavorites* fav = new TFavorites(main_window, "favorites_menu",
                                     tr("Fa&vorites"), "open_favorites",
                                     TPaths::configPath() + "/favorites.m3u8");
    fav->editAct()->setObjectName("edit_fav_list");
    fav->jumpAct()->setObjectName("jump_fav_list");
    fav->nextAct()->setObjectName("next_fav");
    fav->previousAct()->setObjectName("previous_fav");
    fav->addCurrentAct()->setObjectName("add_current_fav");
    main_window->addAction(fav->editAct());
    main_window->addAction(fav->jumpAct());
    main_window->addAction(fav->nextAct());
    main_window->addAction(fav->previousAct());
    main_window->addAction(fav->addCurrentAct());
    addMenu(fav);
    connect(main_window, SIGNAL(mediaFileTitleChanged(const QString&, const QString&)),
            fav, SLOT(getCurrentMedia(const QString&, const QString&)));

    // Recents
    recentfiles_menu = new TMenu(main_window, main_window, "recent_menu",
                                 tr("&Recent files"), "recents");
    clearRecentsAct = new TAction(this, "clear_recents", tr("&Clear"),
                                  "delete", 0, false);
    main_window->addAction(clearRecentsAct);
    connect(clearRecentsAct, SIGNAL(triggered()), this, SLOT(clearRecentsList()));
    addMenu(recentfiles_menu);
    updateRecents();
    connect(main_window, SIGNAL(preferencesChanged()),
            this, SLOT(onPreferencesChanged()));

    addSeparator();


    // Open URL
    TAction* a = new TAction(this, "open_url", tr("Open &URL..."), "",
                             QKeySequence("Ctrl+U"));
    connect(a, SIGNAL(triggered()), main_window, SLOT(openURL()));
    main_window->addAction(a);

    // Open file
    a  = new TAction(this, "open_file", tr("&Open file..."), "open",
                              Qt::CTRL | Qt::Key_F);
    connect(a, SIGNAL(triggered()), main_window, SLOT(openFile()));
    main_window->addAction(a);

    // Open dir
    a = new TAction(this, "open_directory", tr("Open &directory..."),
                             "", QKeySequence("Ctrl+D"));
    connect(a, SIGNAL(triggered()), main_window, SLOT(openDirectory()));
    main_window->addAction(a);

    // Disc submenu
    addMenu(new TMenuDisc(main_window));

    addSeparator();

    // Playlist
    addAction(main_window->getPlaylist()->findChild<TAction*>("pl_open"));
    addAction(main_window->getPlaylist()->findChild<TAction*>("pl_save"));
    addAction(main_window->getPlaylist()->findChild<TAction*>("pl_saveas"));

    addSeparator();

    // Close
    a = new TAction(this, "close", tr("&Close"));
    main_window->addAction(a);
    connect(a, SIGNAL(triggered()), main_window, SLOT(closeWindow()));
}

TMenuFile::~TMenuFile() {
}

void TMenuFile::updateRecents() {

    recentfiles_menu->clear();

    int current_items = 0;

    if (pref->history_recents.count() > 0) {
        for (int n = 0; n < pref->history_recents.count(); n++) {
            QString i = QString::number(n+1);
            QString fullname = pref->history_recents.getURL(n);
            QString filename = fullname;
            QFileInfo fi(fullname);
            // Let's see if it looks like a file (no dvd://1 or something)
            if (fullname.indexOf("://") < 0) {
                filename = fi.fileName();
            }
            if (filename.size() > 85) {
                filename = filename.left(80) + "...";
            }

            QString show_name = filename;
            QString title = pref->history_recents.getTitle(n);
            if (!title.isEmpty())
                show_name = title;

            QAction* a = recentfiles_menu->addAction(QString("%1. " + show_name)
                .arg(i.insert(i.size()-1, '&'), 3, ' '));
            a->setStatusTip(fullname);
            a->setData(n);
            connect(a, SIGNAL(triggered()), main_window, SLOT(openRecent()));
            current_items++;
        }
    } else {
        QAction* a = recentfiles_menu->addAction(tr("<empty>"));
        a->setEnabled(false);
    }

    recentfiles_menu->menuAction()->setVisible(current_items > 0);
    if (current_items  > 0) {
        recentfiles_menu->addSeparator();
        recentfiles_menu->addAction(clearRecentsAct);
    }
}

void TMenuFile::onPreferencesChanged() {
    // Number of recent items might have changed
    updateRecents();
}

void TMenuFile::clearRecentsList() {

    int ret = QMessageBox::question(main_window,
                                    tr("Confirm deletion - WZPlayer"),
                                    tr("Delete the list of recent files?"),
                                    QMessageBox::Cancel, QMessageBox::Ok);

    if (ret == QMessageBox::Ok) {
        // Delete items in menu
        pref->history_recents.clear();
        updateRecents();
    }
}

} // namespace Menu
} // namespace Action
} // namespace Gui
