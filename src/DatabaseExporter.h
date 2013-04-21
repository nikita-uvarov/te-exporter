#ifndef DATABASE_EXPORTER_H
#define DATABASE_EXPORTER_H

#include "HistoricalDatabase.h"

class DatabaseExporter
{
public :
	DatabaseExporter (HistoricalDatabase* database);

	void dump();
};

#endif