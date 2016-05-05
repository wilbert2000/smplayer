#ifndef GUI_PLAYLIST_PLAYLISTWIDGET_H
#define GUI_PLAYLIST_PLAYLISTWIDGET_H

#include <QTreeWidget>


namespace Gui {
namespace Playlist {

class TPlaylistWidgetItem;


class TPlaylistWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit TPlaylistWidget(QWidget* parent);

    TPlaylistWidgetItem* playing_item;

    int countItems() const;
    int countChildren() const;

    TPlaylistWidgetItem* currentPlaylistWidgetItem() const;
    QTreeWidgetItem* playlistWidgetFolder(QTreeWidgetItem* w) const;
    QTreeWidgetItem* currentPlaylistWidgetFolder() const;
    TPlaylistWidgetItem* firstPlaylistWidgetItem() const;
    TPlaylistWidgetItem* lastPlaylistWidgetItem() const;
    QString playingFile() const;
    QString currentFile() const;
    TPlaylistWidgetItem* findFilename(const QString& filename);

    TPlaylistWidgetItem* getNextPlaylistWidgetItem(TPlaylistWidgetItem* i) const;
    TPlaylistWidgetItem* getNextPlaylistWidgetItem() const;

    TPlaylistWidgetItem* getPreviousPlaylistWidgetItem(TPlaylistWidgetItem* w) const;
    TPlaylistWidgetItem* getPreviousPlaylistWidgetItem() const;

    void setPlayingItem(TPlaylistWidgetItem* item);
    void clearPlayed();
    void clr();
    QTreeWidgetItem* root() const { return invisibleRootItem(); }

    void enableSort(bool enable);

signals:
    void modified();

protected:
    virtual void dropEvent(QDropEvent*);

private:
    int countItems(QTreeWidgetItem* w) const;
    int countChildren(QTreeWidgetItem* w) const;

    TPlaylistWidgetItem* getNextItem(TPlaylistWidgetItem* w,
                                     bool allowChild = true) const;

    TPlaylistWidgetItem* getPreviousItem(TPlaylistWidgetItem* w,
                                         bool allowChild = true) const;

private slots:
    void onSectionClicked(int);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGET_H