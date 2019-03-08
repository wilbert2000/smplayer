#include "iconprovider.h"
#include "extensions.h"
#include "images.h"
#include <QStyle>


TIconProvider iconProvider;

TIconProvider::TIconProvider() {
}

void TIconProvider::setStyle(QStyle* aStyle) {

    style = aStyle;

    fileIcon = QIcon(style->standardPixmap(QStyle::SP_FileIcon));
    fileLinkIcon = QIcon(style->standardPixmap(QStyle::SP_FileLinkIcon));

    folderIcon = QIcon();
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirClosedIcon),
                         QIcon::Normal, QIcon::Off);
    folderIcon.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon),
                         QIcon::Normal, QIcon::On);

    folderLinkIcon = QIcon();
    folderLinkIcon.addPixmap(style->standardPixmap(QStyle::SP_DirLinkIcon),
                             QIcon::Normal, QIcon::Off);
    folderLinkIcon.addPixmap(style->standardPixmap(QStyle::SP_DirLinkOpenIcon),
                             QIcon::Normal, QIcon::On);

    //driveCDIcon = QIcon(style->standardPixmap(QStyle::SP_DriveCDIcon));
    //driveDVDIcon = QIcon(style->standardPixmap(QStyle::SP_DriveDVDIcon));
    //urlIcon = Images::icon("type_url", iconSize.width());

    iconPlayed =  QIcon(style->standardPixmap(QStyle::SP_DialogOkButton));
    iconLoading =  QIcon(style->standardPixmap(QStyle::SP_MessageBoxQuestion));
    iconPlaying =  QIcon(style->standardPixmap(QStyle::SP_MediaPlay));
    iconFailed =  QIcon(style->standardPixmap(QStyle::SP_MessageBoxWarning));

    // TODO: fix "device independant pixels"?
    iconSize = folderIcon.actualSize(QSize(22, 22));
}

/* No longer used
QIcon TIconProvider::icon(const QFileInfo& fi) const {

    if (fi.isSymLink()) {
        if (fi.isDir() || extensions.isPlaylist(fi)) {
            return folderLinkIcon;
        }
        return fileLinkIcon;
    }

    if (fi.isDir() || extensions.isPlaylist(fi)) {
        return folderIcon;
    }

    if (fi.filePath().startsWith("dvd://")
        || fi.filePath().startsWith("dvdnav://")
        || fi.filePath().startsWith("br://")) {
        return driveDVDIcon;
    }

    if (fi.filePath().startsWith("vcd://")
        || fi.filePath().startsWith("cdda://")) {
        return driveCDIcon;
    }

    if (fi.filePath().indexOf("://") > 0) {
        return urlIcon;
    }

    return fileIcon;
}

QIcon TIconProvider::iconForFile(const QString& filename) const {

    if (filename.isEmpty()) {
        return fileIcon;
    }
    return icon(QFileInfo(filename));
}
*/
