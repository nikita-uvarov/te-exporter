#ifndef HISTORICAL_DATABASE_H
#define HISTORICAL_DATABASE_H

#include <memory>
#include <QString>
#include <QVector>
#include <QMap>

#include "Util.h"
#include "VariableStack.h"

using std::shared_ptr;

const int SIMPLE_DATE_UNSPECIFIED = -1,
		  SIMPLE_DATE_NOT_PARSED  = -2;

extern const char* DATABASE_INCLUDE_DIRECTORIES_CONTEXT;

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
	VariableStackState variableStack;

	HistoricalEntry (ComplexDate date, QString tag, VariableStackState state) :
		date (date), tag (tag), variableStack (state)
	{}

	virtual ~HistoricalEntry();
};

class HistoricalEvent : public HistoricalEntry
{
public :
	QString eventName, eventDescription;

	HistoricalEvent (ComplexDate date, QString tag, VariableStackState state, QString eventName, QString eventDescription) :
		HistoricalEntry (date, tag, state), eventName (eventName), eventDescription (eventDescription)
	{}

	~HistoricalEvent() {}
};

class HistoricalTerm : public HistoricalEntry
{
public :
	QString termName, termDefinition, inverseQuestion;

	HistoricalTerm (ComplexDate date, QString tag, VariableStackState state, QString termName, QString termDefinition, QString inverseQuestion) :
		HistoricalEntry (date, tag, state), termName (termName), termDefinition (termDefinition), inverseQuestion (inverseQuestion)
	{}

	~HistoricalTerm() {}
};

class HistoricalQuestion : public HistoricalEntry
{
public :
	QString question, answer;

	HistoricalQuestion (ComplexDate date, QString tag, VariableStackState state, QString question, QString answer) :
		HistoricalEntry (date, tag, state), question (question), answer (answer)
	{}

    ~HistoricalQuestion() {}
};

class HistoricalDatabase
{
public :
	QVector <FileLocationMessage> messages;
	QVector < shared_ptr <HistoricalEntry> > entries;

	shared_ptr <VariableStack> variableStack;

	HistoricalDatabase (shared_ptr <VariableStack> variableStack) :
		variableStack (variableStack)
	{}
};

bool isEndingSymbol (QChar c);
QString replaceEscapes (QString s);

class DatabaseParser
{
public :
	shared_ptr <HistoricalDatabase> parseDatabase (QString fileName, const QString& fileContents, shared_ptr <HistoricalDatabase> appendTo);

private :
	bool obsoleteMonthNames;
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

	void processDirective (QString directive, shared_ptr <HistoricalDatabase> database);

	void updateMonthNames();
};

#endif // HISTORICAL_DATABASE_H