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

#include "gui/logwindow.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QTextEdit>
#include <QPushButton>

#include "log4qt/ttcclayout.h"
#include "config.h"
#include "desktop.h"
#include "images.h"
#include "filedialog.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui {

TLogWindowAppender* TLogWindow::appender;

TLogWindowAppender::TLogWindowAppender(QObject* pParent,
                                       Log4Qt::TTCCLayout* alayout) :
    Log4Qt::ListAppender(pParent),
    textEdit(0),
    layout(alayout) {
}

TLogWindowAppender::~TLogWindowAppender() {
}

void TLogWindowAppender::appnd(QString s) {

    //QTextCursor prevCursor = textEdit->textCursor();
    //textEdit->moveCursor(QTextCursor::End);
    //textEdit->insertPlainText(s);
    //textEdit->setTextCursor(prevCursor);

    s.chop(1);
    textEdit->appendPlainText(s);
}

void TLogWindowAppender::append(const Log4Qt::LoggingEvent& rEvent) {

    if (textEdit) {
        appnd(layout->format(rEvent));
    } else {
        Log4Qt::ListAppender::append(rEvent);
    }
}

void TLogWindowAppender::setEdit(QPlainTextEdit* edit) {

    textEdit = edit;
    foreach(const Log4Qt::LoggingEvent& rEvent, list()) {
        appnd(layout->format(rEvent));
    }
    textEdit->moveCursor(QTextCursor::End);
    list().clear();
}


TLogWindow::TLogWindow(QWidget* parent)
    : QWidget(parent, Qt::Window) {

    setupUi(this);
    setObjectName("logwindow");

    edit->setFont(QFont("fixed"));
    edit->setMaximumBlockCount(1000);
    appender->setEdit(edit);
    logger()->debug("TLogWindow: flushed log");

    retranslateStrings();
}

TLogWindow::~TLogWindow() {
    logger()->debug("~TLogWindow");

    Log4Qt::Logger::rootLogger()->removeAppender(appender);
}

void TLogWindow::retranslateStrings() {

    retranslateUi(this);

    setWindowTitle(tr("%1 log").arg(TConfig::PROGRAM_NAME));
    setWindowIcon(Images::icon("logo"));

    saveButton->setText("");
    saveButton->setIcon(Images::icon("save"));
    copyButton->setText("");
    copyButton->setIcon(Images::icon("copy"));
}

void TLogWindow::loadConfig() {
    logger()->debug("loadConfig");

    pref->beginGroup(objectName());
    QPoint p = pref->value("pos", QPoint()).toPoint();
    QSize s = pref->value("size", QPoint()).toSize();
    int state = pref->value("state", 0).toInt();
    pref->endGroup();

    if (s.width() > 200 && s.height() > 200) {
        move(p);
        resize(s);
        setWindowState((Qt::WindowStates) state);
        TDesktop::keepInsideDesktop(this);
    }
}

void TLogWindow::saveConfig() {
    logger()->debug("saveConfig");

    pref->beginGroup(objectName());
    pref->setValue("pos", pos());
    pref->setValue("size", size());
    pref->setValue("state", (int) windowState());
    pref->endGroup();
}

void TLogWindow::showEvent(QShowEvent*) {
    logger()->debug("showEvent");
    emit visibilityChanged(true);
}

void TLogWindow::hideEvent(QShowEvent*) {
    logger()->debug("hideEvent");
    emit visibilityChanged(false);
}

// Fix hideEvent() not called on close
void TLogWindow::closeEvent(QCloseEvent* event) {
    logger()->debug("closeEvent");

    hideEvent(0);
    event->accept();
}

void TLogWindow::onCopyButtonClicked() {

    edit->selectAll();
    edit->copy();
}

void TLogWindow::onSaveButtonClicked() {

    QString s = MyFileDialog::getSaveFileName(
                    this, tr("Choose a filename to save under"), 
                    "", tr("Logs") +" (*.log *.txt)");

    if (!s.isEmpty()) {
        if (QFileInfo(s).exists()) {
            int res =QMessageBox::question(this,
                tr("Confirm overwrite?"),
                tr("The file already exists.\n"
                   "Do you want to overwrite?"),
                QMessageBox::Yes,
                QMessageBox::No,
                QMessageBox::NoButton);
            if (res == QMessageBox::No) {
                return;
            }
        }

        QFile file(s);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << edit->toPlainText();
            file.close();
        } else {
            // Error opening file
            logger()->debug("save: error saving file");
            QMessageBox::warning (this,
                tr("Error saving file"),
                tr("The log couldn't be saved"),
                QMessageBox::Ok,
                QMessageBox::NoButton,
                QMessageBox::NoButton);

        }
    }
}

} // namespace Gui

#include "moc_logwindow.cpp"
