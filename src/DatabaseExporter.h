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

private :
	HistoricalDatabase* database;
};

#endif