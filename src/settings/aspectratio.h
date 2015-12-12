#ifndef SETTINGS_ASPECTRATIO_H
#define SETTINGS_ASPECTRATIO_H

#include <QObject>
#include <QString>


namespace Settings {

class TAspectRatio : public QObject {
	Q_OBJECT
public:
	// IDs used by menu
	enum TMenuID {
		AspectNone = 0,
		AspectAuto = 1,
		Aspect43 = 2,
		Aspect54 = 3,
		Aspect149 = 4,
		Aspect169 = 5,
		Aspect1610 = 6,
		Aspect235 = 7,
		Aspect11 = 8,
		Aspect32 = 9,
		Aspect1410 = 10,
		Aspect118 = 11
	};
	static const int MAX_MENU_ID = 11;

	// List of predefined aspect ratios
	static const unsigned int RATIOS_COUNT = 10;
	static const double RATIOS[RATIOS_COUNT];
	static const char* RATIO_NAMES[RATIOS_COUNT];
	static TMenuID toTMenuID(const QVariant& id);
	static QString doubleToString(double aspect);
	static QString aspectIDToString(int id);
	static double menuIDToDouble(TMenuID id, int w, int h);

	TAspectRatio();

	TMenuID ID() const { return id; }
	void setID(TMenuID anID) { id = anID; }
	TMenuID nextMenuID() const;

	double toDouble(int w, int h) const;
	int toInt() const { return id; }
	QString toString() const;

private:
	TMenuID id;
};

} // namespace Settings

#endif // SETTINGS_ASPECTRATIO_H