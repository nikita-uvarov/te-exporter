#ifndef HISTORICAL_DATABASE_H
#define HISTORICAL_DATABASE_H

#include <memory>
#include <QString>
#include <QVector>
#include <QMap>

#include "Util.h"

using std::shared_ptr;

const int SIMPLE_DATE_UNSPECIFIED = -1,
		  SIMPLE_DATE_NOT_PARSED  = -2;

struct SimpleDate
{
	// Month is greater than zero
	int day, month, year;

	SimpleDate() :
		day (SIMPLE_DATE_UNSPECIFIED), month (SIMPLE_DATE_UNSPECIFIED), year (SIMPLE_DATE_UNSPECIFIED)
	{}

	bool isParsed() const;
	bool isSpecified() const;
};

QTextStream& operator<< (QTextStream& stream, const SimpleDate& date);

struct ComplexDate
{
	SimpleDate begin, end;

	QString toString();
};

QTextStream& operator<< (QTextStream& stream, const ComplexDate& date);

class HistoricalEntry
{
public :
	ComplexDate date;
	QString tag;

	HistoricalEntry (ComplexDate date, QString tag) :
		date (date), tag (tag)
	{}

	virtual ~HistoricalEntry();
};

class HistoricalEvent : public HistoricalEntry
{
public :
	QString eventName, eventDescription;

	HistoricalEvent (ComplexDate date, QString tag, QString eventName, QString eventDescription) :
		HistoricalEntry (date, tag), eventName (eventName), eventDescription (eventDescription)
	{}
};

class HistoricalTerm : public HistoricalEntry
{
public :
	QString termName, termDefinition, inverseQuestion;

	HistoricalTerm (ComplexDate date, QString tag, QString termName, QString termDefinition, QString inverseQuestion) :
		HistoricalEntry (date, tag), termName (termName), termDefinition (termDefinition), inverseQuestion (inverseQuestion)
	{}
};

class HistoricalQuestion : public HistoricalEntry
{
public :
	QString question, answer;

	HistoricalQuestion (ComplexDate date, QString tag, QString question, QString answer) :
		HistoricalEntry (date, tag), question (question), answer (answer)
	{}
};

class HistoricalDatabase
{
public :
	QVector <FileLocationMessage> messages;
	QVector <HistoricalEntry*> entries;

	~HistoricalDatabase();
};

bool isEndingSymbol (QChar c);
QString replaceEscapes (QString s);

class DatabaseParser
{
public :
	void parseMonthNames (LocalizationSettings* settings);
	shared_ptr <HistoricalDatabase> parseDatabase (QString fileName, const QString& fileContents);

private :
	QMap <QString, int> monthNameToIndex;

	HistoricalDatabase* currentDatabase;
	QVector <QString> currentBlock;
	int currentBlockFirstLine;
	QString currentFileName;
	QString currentEntryTag;

	QMap <int, int> entryIndexToLine;
	ComplexDate lastDate;
	bool forwardDate;
	QVector <int> forwardDateEntriesIndices;

	void processCurrentBlock();
	bool tryExtractSimpleDate (QString& line, SimpleDate& date);
	bool tryExtractDate (QString& line, ComplexDate& date);

	void blockParseError (int blockLine, QString what);
	void blockParseWarning (int blockLine, QString what);
};

#endif // HISTORICAL_DATABASE_H