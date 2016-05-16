#include "gui/playlist/playlistwidget.h"

#include <QDebug>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>
#include <QStyledItemDelegate>
#include <QDropEvent>
#include <QFontMetrics>
#include <QTimer>

#include "gui/playlist/playlistwidgetitem.h"
#include "images.h"
#include "iconprovider.h"

namespace Gui {
namespace Playlist {


class TWordWrapItemDelegate : public QStyledItemDelegate {
public:
    TWordWrapItemDelegate(TPlaylistWidget* parent) :
        QStyledItemDelegate(parent),
        header(parent->header()) {
    }

    virtual ~TWordWrapItemDelegate() {
    }

    static int getLevel(const QModelIndex& index) {

        if (index.parent() == QModelIndex()) {
            return gRootNodeLevel;
        }
        return getLevel(index.parent()) + 1;
    }

    virtual QSize sizeHint(const QStyleOptionViewItem& option,
                           const QModelIndex& index) const {

        if (index.column() != TPlaylistWidgetItem::COL_NAME) {
            return QStyledItemDelegate::sizeHint(option, index);
        }
        QString text = index.model()->data(index).toString();
        if (text.isEmpty()) {
            return QStyledItemDelegate::sizeHint(option, index);
        }

        return TPlaylistWidgetItem::itemSize(
                    text,
                    header->sectionSize(TPlaylistWidgetItem::COL_NAME),
                    option.fontMetrics,
                    option.decorationSize,
                    getLevel(index));
    };

private:
    QHeaderView* header;
};


TPlaylistWidget::TPlaylistWidget(QWidget* parent) :
    QTreeWidget(parent),
    debug(logger()),
    playing_item(0) {

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAutoExpandDelay(750);

    //setRootIsDecorated(false);
    setColumnCount(TPlaylistWidgetItem::COL_COUNT);
    setHeaderLabels(QStringList() << tr("Name") << tr("Length"));
    header()->setStretchLastSection(false);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_NAME,
                                   QHeaderView::Stretch);
    header()->setSectionResizeMode(TPlaylistWidgetItem::COL_TIME,
                                   QHeaderView::ResizeToContents);
#else
    header()->setResizeMode(TPlaylistWidgetItem::COL_NAME,
                            QHeaderView::Stretch);
    header()->setResizeMode(TPlaylistWidgetItem::COL_TIME,
                            QHeaderView::ResizeToContents);
#endif

    // Icons
    gIconSize = iconProvider.folderIcon.actualSize(QSize(22, 22));
    setIconSize(gIconSize);

    okIcon = Images::icon("ok", gIconSize.width());
    loadingIcon = Images::icon("loading", gIconSize.width());
    playIcon = Images::icon("play", gIconSize.width());
    failedIcon = Images::icon("failed", gIconSize.width());

    // Create a TPlaylistWidgetItem root
    addTopLevelItem(new TPlaylistWidgetItem(iconProvider.folderIcon));
    setRootIndex(model()->index(0, 0));

    // Sort
    enableSort(false);
    connect(header(), SIGNAL(sectionClicked(int)),
            this, SLOT(onSectionClicked(int)));

    // Drag and drop
    setDragEnabled(true);
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);
    //setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);

    // Wordwrap
    gNameFontMetrics = fontMetrics();

    setWordWrap(true);
    setUniformRowHeights(false);
    setItemDelegate(new TWordWrapItemDelegate(this));
    wordWrapTimer = new QTimer();
    wordWrapTimer->setInterval(750);
    wordWrapTimer->setSingleShot(true);

    connect(wordWrapTimer, SIGNAL(timeout()),
            this, SLOT(resizeRows()));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(header(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(onSectionResized(int,int,int)));

    setContextMenuPolicy(Qt::CustomContextMenu);
}

TPlaylistWidget::~TPlaylistWidget() {
}

void TPlaylistWidget::clr() {

    playing_item = 0;
    clear();

    // Create a TPlaylistWidgetItem root
    addTopLevelItem(new TPlaylistWidgetItem(iconProvider.folderIcon));
    setRootIndex(model()->index(0, 0));
}

int TPlaylistWidget::countItems(QTreeWidgetItem* w) const {

    int count = 0;
    if (w) {
        count = w->childCount();
        for(int c = 0; c < w->childCount(); c++) {
            count += countItems(w->child(c));
        }
    }
    return count;
}

int TPlaylistWidget::countItems() const {
    return countItems(root());
}

int TPlaylistWidget::countChildren(QTreeWidgetItem* w) const {

    if (w) {
        if (w->childCount()) {
            int count = 0;
            for(int c = 0; c < w->childCount(); c++) {
                count += countChildren(w->child(c));
            }
            return count;
        }
        if (w != root()) {
            return 1;
        }
    }
    return 0;
}

int TPlaylistWidget::countChildren() const {
    return countChildren(root());
}

bool TPlaylistWidget::hasItems() const {

    TPlaylistWidgetItem* r = root();
    return r && r->childCount();
}

TPlaylistWidgetItem* TPlaylistWidget::currentPlaylistWidgetItem() const {
    return static_cast<TPlaylistWidgetItem*>(currentItem());
}

TPlaylistWidgetItem* TPlaylistWidget::firstPlaylistWidgetItem() const {

    TPlaylistWidgetItem* r = root();
    if (r) {
        return r->plChild(0);
    }
    return 0;
}

TPlaylistWidgetItem* TPlaylistWidget::lastPlaylistWidgetItem() const {

    TPlaylistWidgetItem* item = root();
    while (item && item->childCount()) {
        item = item->plChild(item->childCount() - 1);
    }
    if (item == root()) {
        return 0;
    }
    return item;
}

QString TPlaylistWidget::playingFile() const {
    return hasItems() && playing_item ? playing_item->filename() : "";
}

QString TPlaylistWidget::currentFile() const {

    TPlaylistWidgetItem* w = currentPlaylistWidgetItem();
    return w ? w->filename() : "";
}

TPlaylistWidgetItem* TPlaylistWidget::findFilename(const QString &filename) {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (i->filename() == filename) {
            return i;
        }
        ++it;
    }

    return 0;
}

void TPlaylistWidget::setPlayingItem(TPlaylistWidgetItem* item,
                                     TPlaylistItemState state) {
    logger()->debug("setPlayingItem");

    bool setCurrent = true;
    if (playing_item) {
        // Set state previous playing item
        if (playing_item != item
            && playing_item->state() != PSTATE_STOPPED
            && playing_item->state() != PSTATE_FAILED) {
            playing_item->setState(PSTATE_STOPPED);
        }
        // Only set current, when playing_item was current
        setCurrent = playing_item == currentItem();
    }

    playing_item = item;

    if (playing_item) {
        playing_item->setState(state);
    }

    if (playing_item && setCurrent) {
        setCurrentItem(playing_item);
    } else {
        // Hack to trigger playlist enableActions...
        emit currentItemChanged(currentItem(), currentItem());
    }
}

void TPlaylistWidget::clearPlayed() {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder()) {
            i->setPlayed(false);
        }
        ++it;
    }
}

TPlaylistWidgetItem* TPlaylistWidget::getNextItem(TPlaylistWidgetItem* w,
                                                  bool allowChild) const {
    // See also itemBelow()

    if (w == 0) {
        return firstPlaylistWidgetItem();
    }

    if (allowChild && w->childCount() > 0) {
        return w->plChild(0);
    }

    TPlaylistWidgetItem* parent = static_cast<TPlaylistWidgetItem*>(w->parent());
    if(parent) {
       int idx = parent->indexOfChild(w) + 1;
       if (idx < parent->childCount()) {
           return parent->plChild(idx);
       }
       return getNextItem(parent, false);
    }

    return 0;
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem(
        TPlaylistWidgetItem* item) const {

    do {
        item = getNextItem(item);
    } while (item && item->childCount());

    return item;
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem() const {

    TPlaylistWidgetItem* item = playing_item;
    if (item == 0) {
        item = currentPlaylistWidgetItem();
    }
    return getNextPlaylistWidgetItem(item);
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousItem(TPlaylistWidgetItem* w,
                                                      bool allowChild) const {
    // See also itemAbove()

    if (w == 0) {
        return lastPlaylistWidgetItem();
    }

    int c = w->childCount();
    if (allowChild && c > 0) {
        return w->plChild(c - 1);
    }

    QTreeWidgetItem* parent = w->parent();
    if(parent) {
       int idx = parent->indexOfChild(w) - 1;
       if (idx >= 0) {
           return static_cast<TPlaylistWidgetItem*>(parent->child(idx));
       }
       return getPreviousItem(static_cast<TPlaylistWidgetItem*>(parent), false);
    }

    return lastPlaylistWidgetItem();
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem(
        TPlaylistWidgetItem* i) const {

    do {
        i = getPreviousItem(i);
    } while (i && i->childCount());

    return i;
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem() const {

    TPlaylistWidgetItem* item = playing_item;
    if (item == 0) {
        item = currentPlaylistWidgetItem();
    }
    return getPreviousPlaylistWidgetItem(item);
}

TPlaylistWidgetItem* TPlaylistWidget::findPreviousPlayedTime(
        TPlaylistWidgetItem* w) {

    TPlaylistWidgetItem* result = 0;
    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (i->childCount() == 0
            && ((result == 0) || (i->playedTime() >= result->playedTime()))
            && (i->playedTime() < w->playedTime())) {
            result = i;
        }
        ++it;
    }

    return result;
}

// Fix Qt not selecting the drop
void TPlaylistWidget::dropEvent(QDropEvent *e) {
    logger()->debug("dropEvent");

    QTreeWidgetItem* current = currentItem();
    QList<QTreeWidgetItem*>	selection = selectedItems();

    QTreeWidget::dropEvent(e);

    if (e->isAccepted()) {
        clearSelection();
        setCurrentItem(current);
        for(int i = 0; i < selection.count(); i++) {
            selection[i]->setSelected(true);
        }
        emit modified();
    }
}

void TPlaylistWidget::enableSort(bool enable) {

    setSortingEnabled(enable);
    if (enable) {
        header()->setSortIndicator(TPlaylistWidgetItem::COL_NAME,
                                   Qt::AscendingOrder);
    } else {
        header()->setSortIndicator(-1, Qt::AscendingOrder);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionsClickable(true);
#else
        header()->setClickable(true);
#endif
    }
}

void TPlaylistWidget::onSectionClicked(int) {

    // TODO: see sortItems/sortColumn
    if (!isSortingEnabled()) {
        enableSort(true);
    }
}

void TPlaylistWidget::resizeRows(QTreeWidgetItem* w, int level) {

    if (w) {
        level++;
        for(int c = 0; c < w->childCount(); c++) {
            TPlaylistWidgetItem* cw = static_cast<TPlaylistWidgetItem*>(w->child(c));
            if (cw) {
                cw->setSzHint(level);
                if (cw->isExpanded() && cw->childCount()) {
                    resizeRows(cw, level);
                }
            }
        }
    }
}

void TPlaylistWidget::resizeRows() {

    gNameColumnWidth = header()->sectionSize(TPlaylistWidgetItem::COL_NAME);
    logger()->debug("resizeRows: width name %1", gNameColumnWidth);
    resizeRows(root(), gRootNodeLevel);
}

void TPlaylistWidget::onSectionResized(int logicalIndex, int, int newSize) {

    if (logicalIndex == TPlaylistWidgetItem::COL_NAME) {
        wordWrapTimer->start();
        gNameColumnWidth = newSize;
    }
}

void TPlaylistWidget::onItemExpanded(QTreeWidgetItem* w) {
    logger()->debug("onItemExpanded: '" + w->text(0) + "'");

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(w);
    if (i && !wordWrapTimer->isActive()) {
        gNameColumnWidth = header()->sectionSize(TPlaylistWidgetItem::COL_NAME);
        resizeRows(i, i->getLevel());
    }
}

TPlaylistWidgetItem* TPlaylistWidget::add(TPlaylistWidgetItem* item,
                                          QTreeWidgetItem* target,
                                          QTreeWidgetItem* current) {
    logger()->debug("add");

    int sort = header()->sortIndicatorSection();
    Qt::SortOrder sortorder = header()->sortIndicatorOrder();
    if (isSortingEnabled()) {
        enableSort(false);
    }

    // Set parent and child index into parent
    QTreeWidgetItem* parent;
    int idx;
    // TODO: validate target
    if (target) {
        if (target->childCount()) {
            parent = target;
            idx = parent->childCount();
        } else {
            parent = target->parent();
            idx = parent->indexOfChild(target);
        }
    } else {
        parent = topLevelItem(0);
        if (parent) {
            idx = parent->childCount();
        } else {
            idx = 0;
        }
    }

    if (parent == 0
        || parent == invisibleRootItem()
        || (parent == topLevelItem(0) && parent->childCount() == 0)) {

        // Remove single folder in root
        if (item->childCount() == 1 && item->child(0)->childCount()) {
            logger()->debug("add: removing single folder in root");
            TPlaylistWidgetItem* old = item;
            item = static_cast<TPlaylistWidgetItem*>(item->takeChild(0));
            delete old;
        }

        // Delete old root, if any
        setRootIndex(QModelIndex());
        delete takeTopLevelItem(0);

        // Set item as root
        item->setFlags(ROOT_FLAGS);
        addTopLevelItem(item);
        setRootIndex(model()->index(0, 0));

        if (!item->isPlaylist()) {
            enableSort(true);
        }

        if (item->childCount()) {
            setCurrentItem(item->child(0));
        }
    } else {
        QList<QTreeWidgetItem*> children;
        while (item->childCount()) {
            children << item->takeChild(0);
        }
        delete item;
        item = 0;

        clearSelection();
        parent->insertChildren(idx, children);

        if (sort >= 0) {
            setSortingEnabled(true);
            header()->setSortIndicator(sort, sortorder);
        }

        setCurrentItem(current);
    }

    return item;
}

} // namespace Playlist
} // namespace Gui
