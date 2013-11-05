#ifndef DATABASE_EXPORTER_H
#define DATABASE_EXPORTER_H

#include "FlashcardsDatabase.h"
#include "FlashcardUtilities.h"

#include <set>
#include <map>
#include <vector>
#include <algorithm>

class FlashcardsDeck
{
public :

	void setColumnValue (QString columnName, QString columnValue);
	void submitRow();

	void writeDeck (QTextStream& stream);
	void saveResources (QString deckFileName, bool verbose = false);

	void removeDuplicates();

	void setMediaDirectoryName (QString name);
	QString getResourceDeckPath (QString resourceAbsolutePath);

private :

	// Qt doesn't implement std::unique-like algorithm, nor it provides hash or comparison functions for its
	// complex data structures - QMap and QSet. So, use STL containers here.
	std::vector <QString> usedColumns;
	std::vector < std::map <int, QString> > exportData;
	std::map <int, QString> currentRow;

	QString mediaDirectoryName;
	QMap <QString, QString> resourceAbsoluteToDeckPathMap;
};

class DatabaseExporter
{
public :
	DatabaseExporter (FlashcardsDatabase* database) :
		database (database)
	{}

	void printMessages();
	void dump();
	void exportDatabase (FlashcardsDeck* exportTo, VariableStackState exportArguments);

private :
    FlashcardsDatabase* database;

    void exportEntry (FlashcardsDeck* exportTo, SimpleFlashcard* entry);

    /*QString complexDateToStringLocalized (ComplexDate date, SimpleFlashcard* entry);
    QString simpleDateToStringLocalized (SimpleDate date, SimpleFlashcard* entry);

    QString variableValue (QString name, SimpleFlashcard* entry);
    bool booleanVariableValue (QString name, SimpleFlashcard* entry);*/
};

#endif // DATABASE_EXPORTER_H