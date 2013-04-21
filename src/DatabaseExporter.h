#ifndef DATABASE_EXPORTER_H
#define DATABASE_EXPORTER_H

#include "HistoricalDatabase.h"

class DatabaseExporter
{
public :
	DatabaseExporter (HistoricalDatabase* database) :
		database (database)
	{}

	void printMessages();
	void dump();
	void exportDatabase (QString deckName, QString exportPath);

private :
	HistoricalDatabase* database;

	std::vector <QString> usedColumns;
	std::vector < std::map <int, QString> > exportData;
	std::map <int, QString> currentRow;

	void exportEntry (HistoricalEntry* entry);

	void setColumnValue (QString columnName, QString columnValue);
	void submitRow();
};

#endif // DATABASE_EXPORTER_H