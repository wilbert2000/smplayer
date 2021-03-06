#ifndef GUI_LOGWINDOWAPPENDER_H
#define GUI_LOGWINDOWAPPENDER_H

#include "log4qt/varia/listappender.h"


class QPlainTextEdit;

namespace Log4Qt {
class Layout;
class LoggingEvent;
}

namespace Gui {

class TLogWindowAppender : public Log4Qt::ListAppender {

public:
    TLogWindowAppender(Log4Qt::Layout* aLayout);

    void setEdit(QPlainTextEdit* edit);

protected:
    virtual void append(const Log4Qt::LoggingEvent& rEvent);

private:
    QPlainTextEdit* textEdit;
    Log4Qt::Layout* layout;

    static void removeNewLine(QString& s);
    void appendTextToEdit(QString s);
};

} // namespace Gui

#endif // GUI_LOGWINDOWAPPENDER_H
