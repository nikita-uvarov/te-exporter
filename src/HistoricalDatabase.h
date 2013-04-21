#ifndef HISTORICAL_DATABASE_H
#define HISTORICAL_DATABASE_H

#include <memory>

using std::shared_ptr;

const int SIMPLE_DATE_UNSPECIFIED = -1;

struct SimpleDate
{
	int day, month, year;
};

struct ComplexDate
{
	SimpleDate begin, end;
};

class HistoricalEntry
{
public :
	ComplexDate interval;
	QString tag;

	virtual ~HistoricalEntry();
	virtual void dump() = 0;
};

class HistoricalEvent : public HistoricalEntry
{
public :
	QString eventName, eventDescription;

	void dump();
};

class HistoricalTerm : public HistoricalEntry
{
public :
	QString termName, termDefinition, inverseQuestion;

	void dump();
};

class HistoricalQuestion : public HistoricalEntry
{
public :
	QString question, answer;

	void dump();
};

enum HistoricalDatabaseParseMessageType
{
	INFO,
	WARNING,
	ERROR
};

struct HistoricalDatabaseParseMessage
{
	QString fileName;
	int line;

	HistoricalDatabaseParseMessageType type;
	QString message;
};

class HistoricalDatabase
{
public :
	vector <ExportMessage> messages;
	vector <HistoricalEntry*> entries;
};

class DatabaseParser
{
public :
	shared_ptr <HistoricalDatabase> parseDatabase (QString fileName, const QByteArray& fileContents);
};

#endif