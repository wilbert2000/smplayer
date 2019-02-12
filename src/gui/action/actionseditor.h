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

/* This is based on qq14-actioneditor-code.zip from Qt */

#ifndef GUI_ACTION_ACTIONSEDITOR_H
#define GUI_ACTION_ACTIONSEDITOR_H

#include <QWidget>
#include <QStringList>
#include <gui/action/actionlist.h>
#include <wzdebug.h>

class QTableWidget;
class QTableWidgetItem;
class QSettings;
class QPushButton;

namespace Gui {
namespace Action {

typedef QList<QKeySequence> TShortCutList;

class TActionsEditor : public QWidget {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER
public:
    // Static functions
    static QString actionTextToDescription(const QString& text,
                                           const QString& action_name);

    static void saveToConfig(QSettings* set, QObject* o);
    static void loadFromConfig(QSettings* set, const TActionList& all_actions);

    TActionsEditor(QWidget* parent = 0, Qt::WindowFlags f = 0);
    virtual ~TActionsEditor();

    // Clear the actionlist
    void clear();
    void addActions(QWidget* widget);
    QString findShortcutsAction(const QString& shortcuts);

public slots:
    void applyChanges();
    void updateView();

protected:
    virtual void retranslateStrings();
    virtual void resizeEvent(QResizeEvent* event);

    // Find in table, not in actionslist
    int findActionName(const QString& name);
    int findShortcuts(const QString& accel, int ignoreRow = -1);
    bool hasConflicts();
    static bool containsShortcut(const QString& accel, const QString& shortcut);

private:
    enum TActionCols {
        COL_CONFLICTS = 0,
        COL_ACTION = 1,
        COL_DESC = 2,
        COL_SHORTCUT = 3,
        COL_COUNT = 4
    };

    QTableWidget* actionsTable;
    TActionList actionsList;
    QPushButton* saveButton;
    QPushButton* loadButton;
    QPushButton* editButton;
    QString last_dir;

    static QString keyToString(QKeySequence key);
    static QKeySequence stringToKey(QString s);
    static QString shortcutsToString(const TShortCutList& shortcuts);
    static TShortCutList stringToShortcuts(const QString& shortcuts);

    static QString actionToString(QAction *action);
    static void setActionFromString(QAction* action, const QString& s, const TActionList& actions);
    static void removeShortcuts(const TActionList& actions, const TShortCutList& shortcuts, QAction* skip_action);

    static QAction* findAction(const TActionList& actions, const QString& name);

    bool loadActionsTableFromFile(const QString& filename);
    bool saveActionsTableAsFile(const QString& filename);
    void resizeColumns();

private slots:
    void editShortcut();
    void loadActionsTable();
    void saveActionsTable();
};

} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_ACTIONSEDITOR_H
