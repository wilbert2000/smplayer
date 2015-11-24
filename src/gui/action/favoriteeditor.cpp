/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "gui/action/favoriteeditor.h"
#include "images.h"

#include <QHeaderView>
#include <QFileDialog>
#include <QItemDelegate>
#include "filechooser.h"

#define COL_ICON 0
#define COL_NAME 1
#define COL_FILE 2

#include <QItemDelegate>

class FEDelegate : public QItemDelegate 
{
public:
	FEDelegate(QObject *parent = 0);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;
	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, 
                              const QModelIndex & index) const;
};

FEDelegate::FEDelegate(QObject *parent) : QItemDelegate(parent) {
}

QWidget* FEDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & option, const QModelIndex & index) const {
	//qDebug("FEDelegate::createEditor");

	if (index.column() == COL_FILE) {
		FileChooser* fch = new FileChooser(parent);
		fch->setOptions(QFileDialog::DontUseNativeDialog | QFileDialog::DontResolveSymlinks); // Crashes if use the KDE dialog
		fch->setText(index.model()->data(index, Qt::DisplayRole).toString());
		return fch;
	} 
	else 
	if (index.column() == COL_NAME) {
		QLineEdit* e = new QLineEdit(parent);
		e->setText(index.model()->data(index, Qt::DisplayRole).toString());
		return e;
	}
	else {
		return QItemDelegate::createEditor(parent, option, index);
	}
}

void FEDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
	if (index.column() == COL_FILE) {
		FileChooser* fch = static_cast<FileChooser*>(editor);
		model->setData(index, fch->text());
	} 
	else 
	if (index.column() == COL_NAME) {
		QLineEdit* e = static_cast<QLineEdit*>(editor);
		model->setData(index, e->text());
	}
}

namespace Gui {

QString TFavoriteEditor::last_dir;

TFavoriteEditor::TFavoriteEditor(QWidget* parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	setupUi(this);

	add_button->setIcon(Images::icon("bookmark_add"));
	add_submenu_button->setIcon(Images::icon("bookmark_folder"));
	delete_button->setIcon(Images::icon("delete"));
	delete_all_button->setIcon(Images::icon("trash"));
	up_button->setIcon(Images::icon("up"));
	down_button->setIcon(Images::icon("down"));

	table->setColumnCount(3);
	table->setHorizontalHeaderLabels(QStringList() << tr("Icon") << tr("Name") << tr("Media"));

	table->setAlternatingRowColors(true);
#if QT_VERSION >= 0x050000
	table->horizontalHeader()->setSectionResizeMode(COL_FILE, QHeaderView::Stretch);
#else
	table->horizontalHeader()->setResizeMode(COL_FILE, QHeaderView::Stretch);
#endif

	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::ExtendedSelection);

	table->setItemDelegateForColumn(COL_NAME, new FEDelegate(table));
	table->setItemDelegateForColumn(COL_FILE, new FEDelegate(table));

	connect(table, SIGNAL(cellActivated(int,int)), this, SLOT(edit_icon(int,int)));

	setWindowTitle(tr("Favorite editor"));

	setCaption(tr("Favorite list"));
	setIntro(tr("You can edit, delete, sort or add new items. Double click on "
                 "a cell to edit its contents."));

	setDialogIcon(Images::icon("favorite"));
}

TFavoriteEditor::~TFavoriteEditor() {
}

void TFavoriteEditor::setCaption(const QString & caption) {
	caption_text = caption;
	updateTitleLabel();
}

QString TFavoriteEditor::caption() {
	return caption_text;
}

void TFavoriteEditor::setIntro(const QString & intro) {
	intro_text = intro;
	updateTitleLabel();
}

QString TFavoriteEditor::intro() {
	return intro_text;
}

void TFavoriteEditor::updateTitleLabel() {
	title_label->setText("<h1>" + caption_text + "</h1>" + intro_text);
}

void TFavoriteEditor::setDialogIcon(const QPixmap & icon) {
	dialog_icon->setPixmap(icon);
}

const QPixmap* TFavoriteEditor::dialogIcon() const {
	return dialog_icon->pixmap();
}

void TFavoriteEditor::setData(TFavoriteList list) {
	table->setRowCount(list.count());

	for (int n = 0; n < list.count(); n++) {
		QTableWidgetItem* icon_item = new QTableWidgetItem;
		icon_item->setIcon(QIcon(list[n].icon()));
		icon_item->setData(Qt::UserRole, list[n].icon());
		icon_item->setData(Qt::ToolTipRole, list[n].icon());
		icon_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

		QTableWidgetItem* name_item = new QTableWidgetItem;
		name_item->setText(list[n].name());

		QTableWidgetItem* file_item = new QTableWidgetItem;
		file_item->setData(Qt::ToolTipRole, list[n].file());
		file_item->setData(Qt::UserRole, list[n].isSubentry());
		if (list[n].isSubentry()) {
			file_item->setFlags(Qt::ItemIsSelectable);
			file_item->setData(Qt::UserRole + 1, list[n].file());
			file_item->setText(tr("Favorite list"));
		} else {
			file_item->setText(list[n].file());
		}

		table->setItem(n, COL_ICON, icon_item);
		table->setItem(n, COL_NAME, name_item);
		table->setItem(n, COL_FILE, file_item);
	}

	//table->resizeColumnsToContents();

	//table->setCurrentCell(0, 0);
	table->setCurrentCell(table->rowCount()-1, 0);
}

TFavoriteList TFavoriteEditor::data() {
	TFavoriteList list;

	for (int n = 0; n < table->rowCount(); n++) {
		TFavorite f;
		f.setName(table->item(n, COL_NAME)->text());
		f.setIcon(table->item(n, COL_ICON)->data(Qt::UserRole).toString());
		f.setSubentry(table->item(n, COL_FILE)->data(Qt::UserRole).toBool());
		if (f.isSubentry()) {
			f.setFile(table->item(n, COL_FILE)->data(Qt::UserRole + 1).toString());
		} else {
			f.setFile(table->item(n, COL_FILE)->text());
		}

		list.append(f);
	}

	return list;
}

void TFavoriteEditor::on_delete_button_clicked() {
	int row = table->currentRow();
	qDebug("Gui::TFavoriteEditor::on_delete_button_clicked: current_row: %d", row);

	if (row > -1) table->removeRow(row);

	if (row >= table->rowCount()) row--;
	table->setCurrentCell(row, table->currentColumn());
}

void TFavoriteEditor::on_delete_all_button_clicked() {
	qDebug("Gui::TFavoriteEditor::on_delete_all_button_clicked");
	table->setRowCount(0);
}

void TFavoriteEditor::on_add_button_clicked() {
	int row = table->currentRow();
	qDebug("Gui::TFavoriteEditor::on_add_button_clicked: current_row: %d", row);
	row++;
	table->insertRow(row);

	QTableWidgetItem* icon_item = new QTableWidgetItem;
	icon_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	table->setItem(row, COL_ICON, icon_item);
	table->setItem(row, COL_NAME, new QTableWidgetItem);
	table->setItem(row, COL_FILE, new QTableWidgetItem);

	table->setCurrentCell(row, table->currentColumn());
}

void TFavoriteEditor::on_add_submenu_button_clicked() {
	qDebug("Gui::TFavoriteEditor::on_add_submenu_button_clicked");
	qDebug("Gui::TFavoriteEditor::on_add_submenu_button_clicked: store_path: '%s'", store_path.toUtf8().constData());

	QString filename;
	//QString s;
	int n = 1;
	do {
		filename = QString("favorites%1.m3u8").arg(n, 4, 10, QChar('0'));
		if (!store_path.isEmpty()) filename = store_path +"/"+ filename;
		qDebug("Gui::TFavoriteEditor::on_add_submenu_button_clicked: filename: '%s'", filename.toUtf8().constData());
		n++;
	} while (QFile::exists(filename));

	qDebug("Gui::TFavoriteEditor::on_add_submenu_button_clicked: choosen filename: '%s'", filename.toUtf8().constData());


	int row = table->currentRow();
	row++;
	table->insertRow(row);

	QTableWidgetItem* icon_item = new QTableWidgetItem;
	icon_item->setData(Qt::UserRole, Images::file("openfolder"));
	icon_item->setIcon(Images::icon("openfolder"));
	icon_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	table->setItem(row, COL_ICON, icon_item);
	table->setItem(row, COL_NAME, new QTableWidgetItem);

	QTableWidgetItem* file_item = new QTableWidgetItem;
	file_item->setData(Qt::UserRole, true);
	file_item->setFlags(Qt::ItemIsSelectable);
	file_item->setData(Qt::UserRole + 1, filename);
	file_item->setText(tr("Favorite list"));
	file_item->setData(Qt::ToolTipRole, filename);
	table->setItem(row, COL_FILE, file_item);

	table->setCurrentCell(row, table->currentColumn());
}

void TFavoriteEditor::on_up_button_clicked() {
	int row = table->currentRow();
	qDebug("Gui::TFavoriteEditor::on_up_button_clicked: current_row: %d", row);

	if (row == 0) return;

	// take whole rows
	QList<QTableWidgetItem*> source_items = takeRow(row);
	QList<QTableWidgetItem*> dest_items = takeRow(row-1);
 
	// set back in reverse order
	setRow(row, dest_items);
	setRow(row-1, source_items);

	table->setCurrentCell(row-1, table->currentColumn());
}

void TFavoriteEditor::on_down_button_clicked() {
	int row = table->currentRow();
	qDebug("Gui::TFavoriteEditor::on_down_button_clicked: current_row: %d", row);

	if ((row+1) >= table->rowCount()) return;

	// take whole rows
	QList<QTableWidgetItem*> source_items = takeRow(row);
	QList<QTableWidgetItem*> dest_items = takeRow(row+1);
 
	// set back in reverse order
	setRow(row, dest_items);
	setRow(row+1, source_items);

	table->setCurrentCell(row+1, table->currentColumn());
}
 
// takes and returns the whole row
QList<QTableWidgetItem*> TFavoriteEditor::takeRow(int row) {
	QList<QTableWidgetItem*> rowItems;
	for (int col = 0; col < table->columnCount(); ++col)
	{
		rowItems << table->takeItem(row, col);
	}
	return rowItems;
}
 
// sets the whole row
void TFavoriteEditor::setRow(int row, const QList<QTableWidgetItem*>& rowItems)
{
	for (int col = 0; col < table->columnCount(); ++col)
	{
		table->setItem(row, col, rowItems.at(col));
	}
}

void TFavoriteEditor::edit_icon(int row, int column) {
	qDebug("Gui::TFavoriteEditor::edit_icon: %d, %d", row, column);

	if (column != COL_ICON) return;

	QTableWidgetItem* i = table->item(row, column);
	QString icon_filename = i->data(Qt::UserRole).toString();

	qDebug("Gui::TFavoriteEditor::edit_icon: icon file: '%s'", icon_filename.toUtf8().constData());

	QString dir = icon_filename;
	if (dir.isEmpty()) dir = last_dir;

	QString res = QFileDialog::getOpenFileName(this, tr("Select an icon file"),
                                               dir,
                                               tr("Images") + " (*.png *.xpm *.jpg)");
	if (!res.isEmpty()) {
		i->setIcon(QIcon(res));
		i->setData(Qt::UserRole, res);

		last_dir = QFileInfo(res).absolutePath();
	}
}

} // namespace Gui

#include "moc_favoriteeditor.cpp"