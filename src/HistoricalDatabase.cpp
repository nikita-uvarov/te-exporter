#include "HistoricalDatabase.h"

#include <QString>
#include <QStringList>

HistoricalEntry::~HistoricalEntry()
{}

HistoricalDatabase::~HistoricalDatabase()
{
	for (HistoricalEntry* e: entries)
		delete e;
	entries.clear();
}

QTextStream& operator<< (QTextStream& stream, const SimpleDate& date)
{
	if (!date.isParsed())
		stream << "(not parsed)";
	else if (date.day != SIMPLE_DATE_UNSPECIFIED)
		stream << date.day << "." << date.month << "." << date.year;
	else if (date.month != SIMPLE_DATE_UNSPECIFIED)
		stream << date.month << "." << date.year;
	else if (date.year != SIMPLE_DATE_UNSPECIFIED)
		stream << date.year;
	else
		stream << "(not specified)";

	return stream;
}

QTextStream& operator<< (QTextStream& stream, const ComplexDate& date)
{
	stream << "(c-date: ";
	if (date.begin.isSpecified())
	{
		if (date.end.isSpecified())
			stream << date.begin << "-" << date.end;
		else
			stream << date.begin;
	}
	else
	{
		stream << "not specified";
	}
	stream << ")";

	return stream;
}

bool isEndingSymbol (QChar c)
{
	return c == '.' || c == '?' || c == '!' || c == ')';
}

bool SimpleDate::isParsed() const
{
	return year != SIMPLE_DATE_NOT_PARSED && month != SIMPLE_DATE_NOT_PARSED && day != SIMPLE_DATE_NOT_PARSED;
}

bool SimpleDate::isSpecified() const
{
	return year != SIMPLE_DATE_UNSPECIFIED;
}

class DatabaseParserException : public std::exception
{
public :
    DatabaseParserException ()
	{}

	const char* what()
	{
		return "Database parser exception: current block is invalid, see error messages for details";
	}
};

void DatabaseParser::blockParseWarning (int blockLine, QString what)
{
	currentDatabase->messages.push_back (FileLocationMessage (currentFileName, currentBlockFirstLine + blockLine, FileLocationMessageType::WARNING, what));
}

void DatabaseParser::blockParseError (int blockLine, QString what)
{
	currentDatabase->messages.push_back (FileLocationMessage (currentFileName, currentBlockFirstLine + blockLine, FileLocationMessageType::ERROR, what));
	throw DatabaseParserException();
}

void DatabaseParser::parseMonthNames (const QString& monthNames)
{
	QStringList monthes = monthNames.split ('\n', QString::SkipEmptyParts);
	assert (monthes.size() == 12, "The monthes file must contain precisely 12 non-empty lines (" + QString::number (monthes.size()) + " present).");

	for (unsigned i = 0; i < monthes.size(); i++)
	{
		QStringList monthNames = monthes[i].split (' ', QString::SkipEmptyParts);
		assert (!monthNames.empty(), "Every month must have at least one name: missing month " + QString::number(i + 1));

		for (QString name: monthNames)
			monthNameToIndex[name] = i + 1;
	}
}

bool DatabaseParser::tryExtractSimpleDate (QString& line, SimpleDate& date)
{
	QStringList monthes = monthNameToIndex.keys();
	QString monthesMatch = "";
	for (QString monthName: monthes)
		monthesMatch = (monthesMatch.isEmpty() ? monthName : monthesMatch + "|" + monthName);

	for (int numComponents = 3; numComponents >= 1; numComponents--)
	{
		QRegExp dateRegExp (numComponents == 3 ? "(\\d+)\\.(\\d+|" + monthesMatch + ")\\.(\\d+)" :
							numComponents == 2 ? "(\\d+|" + monthesMatch + ")\\.(\\d+)" :
												 "(\\d+)");
		dateRegExp.setCaseSensitivity (Qt::CaseInsensitive);
		dateRegExp.setPatternSyntax (QRegExp::RegExp2);

		int matchIndex = dateRegExp.indexIn (line);

		if (matchIndex != -1)
		{
			//qstdout << "Matched " << numComponents << " components in string '" << line << "'\n";
			QStringList dateComponents = dateRegExp.capturedTexts();
			assert (dateComponents.size() == 1 + numComponents);

			date.day   = (numComponents >= 3) ? dateComponents[dateComponents.size() - 3].toInt() : SIMPLE_DATE_UNSPECIFIED;
			date.year  = (numComponents >= 1) ? dateComponents[dateComponents.size() - 1].toInt() : SIMPLE_DATE_UNSPECIFIED;

			if (numComponents >= 2)
			{
				QString monthString = dateComponents[dateComponents.size() - 2].toLower();
				if (monthNameToIndex.find (monthString) != monthNameToIndex.end())
					date.month = monthNameToIndex[monthString];
				else
					date.month = monthString.toInt();
			}
			else
			{
				date.month = SIMPLE_DATE_UNSPECIFIED;
			}

			line = line.right (line.length() - dateComponents[0].length() - matchIndex);

			return true;
		}
	}

	date.day = date.month = date.year = SIMPLE_DATE_NOT_PARSED;
	return false;
}

bool DatabaseParser::tryExtractDate (QString& line, ComplexDate& date)
{
	if (!tryExtractSimpleDate (line, date.begin))
		return false;

	date.end = SimpleDate();

	if (line[0] == '-')
	{
		line = line.right (line.length() - 1);
		if (!tryExtractSimpleDate (line, date.end))
			blockParseError (0, "Interval end date expected after a dash.");
	}

	return true;
}

void DatabaseParser::processCurrentBlock()
{
	QString firstLine = currentBlock[0];

	{
		// Try to treat as a dated event
		ComplexDate date;
		if (tryExtractDate (firstLine, date))
		{
			qstdout << "Got " << date << " and string '" << firstLine << "'" << endl;

			firstLine = firstLine.trimmed();
			if (firstLine.isEmpty() || firstLine[0] != '-')
				blockParseError (0, "Expected a dash followed by an event name after a date interval, " +
								         (firstLine.isEmpty() ? "end of string" : QString ("'") + firstLine[0] + "'") + " found.");

			firstLine = firstLine.right (firstLine.length() - 1).trimmed();

			if (firstLine.isEmpty())
				blockParseError (0, "Empty event name.");

			if (isEndingSymbol (firstLine[firstLine.length() - 1]) && (firstLine.length() <= 1 || firstLine[firstLine.length() - 2] != '\\'))
				blockParseWarning (0, QString ("An event name ends in unescaped '") + firstLine[firstLine.length() - 1] + "'");

			QRegExp unescapedDash ("^-|[^\\\\]-");

			QString eventDescription = "";
			for (unsigned i = 0; i < currentBlock.size(); i++)
			{
				if (unescapedDash.indexIn (i == 0 ? firstLine : currentBlock[i]) != -1)
					blockParseWarning (i, "Unescaped dash met outside of a date interval/definition construction.");

				if (i == 0) continue;
				eventDescription += (i > 1 ? "\n" : "") + currentBlock[i];
			}

			currentDatabase->entries.push_back (new HistoricalEvent (date, currentEntryTag, firstLine, eventDescription));
			return;
		}
	}
}

shared_ptr <HistoricalDatabase> DatabaseParser::parseDatabase (QString fileName, const QString& fileContents)
{
	shared_ptr <HistoricalDatabase> database (new HistoricalDatabase);
	currentDatabase = database.get();

	QStringList lines = fileContents.split ('\n');

	currentBlock.clear();
	currentFileName = fileName;
	currentEntryTag = "no-tag";

	bool cPlusPlusCommentOpen = false;

	for (int lineNumber = 0; lineNumber < lines.size(); lineNumber++)
	{
		QString& line = lines[lineNumber];

		if (cPlusPlusCommentOpen)
		{
			int commentClosing = line.indexOf ("*/");
			if (commentClosing != -1)
			{
				line = line.right (line.size() - commentClosing - 1);
				cPlusPlusCommentOpen = false;
			}
			else
			{
				line = "";
			}
		}

		int lineCommentOpening = line.indexOf ("//");

		if (lineCommentOpening != -1)
			line = line.left (lineCommentOpening);

		int cPlusPlusCommentOpening = line.indexOf ("/*");

		if (cPlusPlusCommentOpening != -1)
		{
			line = line.left (cPlusPlusCommentOpening);
			cPlusPlusCommentOpen = true;
		}

		line = line.trimmed();

		if (!line.isEmpty())
			currentBlock.push_back (line);

		if ((line.isEmpty() || lineNumber + 1 == lines.size()) && !currentBlock.empty())
		{
			currentBlockFirstLine = lineNumber + 1;

			try
			{
				processCurrentBlock();
			}
			catch (DatabaseParserException&) {}

			currentBlock.clear();
		}
	}

	return database;
}
