#ifndef GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
#define GUI_PLAYLIST_PLAYLISTWIDGETITEM_H

#include <QTreeWidgetItem>
#include <QIcon>


namespace Gui {
namespace Playlist {

enum TPlaylistWidgetItemState {
    PSTATE_STOPPED,
    PSTATE_LOADING,
    PSTATE_PLAYING,
    PSTATE_FAILED
};

class TPlaylistWidget;

extern Qt::CaseSensitivity caseSensitiveFileNames;

// Globals for resizing the name column
extern const int ROOT_NODE_LEVEL;
extern int gNameColumnWidth;
extern QFontMetrics gNameFontMetrics;

// Item flags to use for the root node
const Qt::ItemFlags ROOT_FLAGS = Qt::ItemIsSelectable
                                 | Qt::ItemIsEnabled
                                 | Qt::ItemIsDropEnabled;


class TPlaylistWidgetItem : public QTreeWidgetItem {
public:
    enum TColID {
        COL_NAME = 0,
        COL_TYPE = 1,
        COL_TIME = 2,
        COL_ORDER = 3,
        COL_COUNT = 4
    };

    static int gItemOrder;

    // Create a root node
    TPlaylistWidgetItem();
    // Copy constructor
    TPlaylistWidgetItem(const TPlaylistWidgetItem& item);
    // Create an item from arguments
    TPlaylistWidgetItem(QTreeWidgetItem* parent,
                        const QString& filename,
                        const QString& name,
                        double duration,
                        bool protectName = false);
    virtual ~TPlaylistWidgetItem();

    virtual QVariant data(int column, int role) const override;
    virtual void setData(int column, int role, const QVariant &value) override;

    QString filename() const { return mFilename; }
    void setFilename(const QString& fileName, const QString& baseName);

    QString path() const;
    QString pathPlusSep() const;
    QString fname() const;

    QString baseName() const { return mBaseName; }
    void setName(const QString& baseName, const QString& ext, bool protectName);

    QString extension() const { return mExt; }

    double duration() const { return mDuration; }
    void setDuration(double d) { mDuration = d; }

    TPlaylistWidgetItemState state() const { return mState; }
    void setState(TPlaylistWidgetItemState state);

    bool played() const { return mPlayed; }
    void setPlayed(bool played);

    bool edited() const { return mEdited; }
    void setEdited(bool edited) { mEdited = edited; }

    bool modified() const { return mModified; }
    void setModified(bool modified = true,
                     bool recurse = false,
                     bool markParents = true);

    int order() const { return mOrder; }
    void setOrder(int order) { mOrder = order; }

    bool isFolder() const { return mFolder; }
    bool isPlaylist() const { return mPlaylist; }
    bool isWZPlaylist() const { return mWZPlaylist; }
    bool isSymLink() const { return mSymLink; }
    QString target() const { return mTarget; }

    int playedTime() const { return mPlayedTime; }

    void blacklist(const QString& filename) {
        mBlacklist.append(filename);
    }
    bool blacklisted(const QString& filename) const;
    QStringList getBlacklist() const { return mBlacklist; }
    bool whitelist(const QString& filename);

    static QSize sizeColumnName(int width,
                                const QString& text,
                                const QFontMetrics& fm,
                                const QSize& iconSize,
                                int level);
    void setSzHint(int level);
    int getLevel() const;

    TPlaylistWidget* plTreeWidget() const;
    TPlaylistWidgetItem* plParent() const {
        return static_cast<TPlaylistWidgetItem*>(parent());
    }
    TPlaylistWidgetItem* plChild(int idx) const {
        return static_cast<TPlaylistWidgetItem*>(child(idx));
    }

    void loadIcon();

    virtual bool operator<(const QTreeWidgetItem& other) const;
    bool operator == (const TPlaylistWidgetItem& item);

private:
    int mOrder;
    QString mFilename;
    QString mBaseName;
    QString mExt;
    double mDuration;

    bool mFolder;
    bool mPlaylist;
    bool mWZPlaylist;
    bool mSymLink;
    QString mTarget;

    TPlaylistWidgetItemState mState;
    bool mPlayed;
    bool mEdited;
    bool mModified;
    int mPlayedTime;
    QStringList mBlacklist;

    QIcon itemIcon;
    QIcon getIcon();
    void setStateIcon();

    QString editName() const;

    static QString playlistItemState(TPlaylistWidgetItemState state);

    void refresh(const QString& dir, const QString& newDir);
    bool renameFile(const QString& newName);
    bool rename(const QString& newName);

    void setFileInfo();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
