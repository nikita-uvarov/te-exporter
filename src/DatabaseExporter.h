#ifndef DATABASE_EXPORTER_H
#define DATABASE_EXPORTER_H

#include "HistoricalDatabase.h"

#include <set>
#include <map>
#include <vector>
#include <algorithm>

class HistoricalDeck
{
public :

	void setColumnValue (QString columnName, QString columnValue);
	void submitRow();

	void writeDeck (QTextStream& stream);

	void removeDuplicates();

private :

	// Qt doesn't implement std::unique-like algorithm, nor it provides hash or comparison functions for its
	// complex data structures - QMap and QSet. So, use STL containers here.
	std::vector <QString> usedColumns;
	std::vector < std::map <int, QString> > exportData;
	std::map <int, QString> currentRow;
};

enum class HistoricalEventExportMode
{
	DATE_TO_NAME,
	NAME_TO_DATE,
	NAME_TO_DATE_AND_DEFINITION
};

enum class HistoricalTermExportMode
{
	DIRECT,
	INVERSE
};

class DatabaseExporter
{
public :
	DatabaseExporter (HistoricalDatabase* database, LocalizationSettings* localization) :
		database (database), localization (localization), localizationSettingsRead (false)
	{}

	void printMessages();
	void dump();
	void exportDatabase (HistoricalDeck* exportTo, HistoricalEventExportMode eventExportMode, HistoricalTermExportMode termExportMode);

private :
	HistoricalDatabase* database;

	HistoricalEventExportMode eventExportMode;
	HistoricalTermExportMode termExportMode;

	LocalizationSettings* localization;

	bool localizationSettingsRead;
	QString fullDateMonthNames[12], partialDateMonthNames[12];

	void exportEntry (HistoricalDeck* exportTo, HistoricalEntry* entry);

	QString complexDateToStringLocalized (ComplexDate date);
	QString simpleDateToStringLocalized (SimpleDate date);

	void readLocalizationSettings();
};

#endif // DATABASE_EXPORTER_H