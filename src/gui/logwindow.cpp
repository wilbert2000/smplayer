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

#include "gui/logwindow.h"
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QTextEdit>
#include <QPushButton>
#include "log.h"
#include "images.h"
#include "filedialog.h"

namespace Gui {

TLogWindow::TLogWindow( QWidget* parent )
	: QWidget(parent, Qt::Window ) {

	setupUi(this);
	browser->setFont( QFont("fixed") );
	retranslateStrings();
}

TLogWindow::~TLogWindow() {

	TLog::log->setLogWindow(0);
}

void TLogWindow::retranslateStrings() {
	retranslateUi(this);

	saveButton->setText("");
	copyButton->setText("");

	saveButton->setIcon( Images::icon("save") );
	copyButton->setIcon( Images::icon("copy") );

	setWindowIcon( Images::icon("logo") );
}

void TLogWindow::showEvent(QShowEvent*) {

	TLog::log->setLogWindow(this);
}

void TLogWindow::hideEvent(QShowEvent*) {

	TLog::log->setLogWindow(0);
	clear();
}

void TLogWindow::setText(QString log) {
	browser->setPlainText(log);
}

QString TLogWindow::text() {
	return browser->toPlainText();
}

void TLogWindow::setHtml(QString text) {
	browser->setHtml(text);
}

QString TLogWindow::html() {
	return browser->toHtml();
}

void TLogWindow::clear() {
	browser->clear();
}

void TLogWindow::appendText(QString text) {
	browser->moveCursor(QTextCursor::End);
	browser->insertPlainText(text);
}

void TLogWindow::appendHtml(QString text) {
	browser->moveCursor(QTextCursor::End);
	browser->insertHtml(text);
}

void TLogWindow::on_copyButton_clicked() {
	browser->selectAll();
	browser->copy();
}

void TLogWindow::on_saveButton_clicked() {
	QString s = MyFileDialog::getSaveFileName(
                    this, tr("Choose a filename to save under"), 
                    "", tr("Logs") +" (*.log *.txt)" );

	if (!s.isEmpty()) {
		if (QFileInfo(s).exists()) {
			int res =QMessageBox::question( this, 
                                   tr("Confirm overwrite?"),
                                   tr("The file already exists.\n"
                                      "Do you want to overwrite?"),
                                   QMessageBox::Yes,
                                   QMessageBox::No,
                                   QMessageBox::NoButton);
			if (res == QMessageBox::No ) {
				return;
			}
		}

		QFile file( s );
		if ( file.open( QIODevice::WriteOnly ) ) {
	        QTextStream stream( &file );
    		stream << browser->toPlainText();
	        file.close();
	    } else {
			// Error opening file
			qDebug("Gui::TLogWindow::save: error saving file");
			QMessageBox::warning ( this, 
                                   tr("Error saving file"), 
                                   tr("The log couldn't be saved"),
                                   QMessageBox::Ok, 
                                   QMessageBox::NoButton, 
                                   QMessageBox::NoButton );

		}
	}
}

// Language change stuff
void TLogWindow::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

} // namespace Gui

#include "moc_logwindow.cpp"