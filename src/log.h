#ifndef LOG_H
#define LOG_H

#include <QString>
#include <QFile>
#include <QRegExp>

#if QT_VERSION >= 0x050000
#include <QMessageLogContext>
#endif

#include "gui/logwindow.h"

class TLog {
public:
	TLog(bool log_enabled, bool log_file_enabled, const QString& debug_filter);
	virtual ~TLog();

	static TLog* log;

	bool isEnabled() const { return enabled; }
	void setEnabled(bool enable_log) { enabled = enable_log; }
	void setLogFileEnabled(bool log_file_enabled);
	void setFilter(const QString& filter_str) { filter = QRegExp(filter_str); }
	bool passesFilter(const QString& msg) { return filter.indexIn(msg) >= 0; }
	void setLogWindow(Gui::TLogWindow* window);

	void logLine(QtMsgType type, QString line);
	QString getLogLines() { return lines_back + lines; }

private:
	bool enabled;
	QString lines_back;
	QString lines;
	QFile file;
	QRegExp filter;
	Gui::TLogWindow* log_window;

#if QT_VERSION >= 0x050000
	static void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg);
#else
	static void msgHandler(QtMsgType type, const char* msg);
#endif

}; // class TLog


#endif // LOG_H
