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

QString ComplexDate::toString()
{
	QString str = "";
	QTextStream stream (&str);
	stream << *this;
	return str;
}

bool isEndingSymbol (QChar c)
{
	return c == '.' || c == '?' || c == '!' || c == ')';
}

QString replaceEscapes (QString s)
{
	return s.replace ("\\n", "\n").replace ('\\', "");
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

void DatabaseParser::parseMonthNames (LocalizationSettings* settings)
{
	for (unsigned i = 1; i <= 12; i++)
	{
		QString variants = settings->getLocalizedString (QString ("month_") + (i < 10 ? "0" : "") + QString::number (i) + "_output");
		QStringList monthNames = variants.split (' ', QString::SkipEmptyParts);
		assert (!monthNames.empty(), "Every month must have at least one name: missing month " + QString::number(i + 1));

		for (QString name: monthNames)
			monthNameToIndex[name.toLower()] = i;
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
		QRegExp dateRegExp (numComponents == 3 ? "^(\\d+)\\.(\\d+|" + monthesMatch + ")\\.(\\d+)" :
							numComponents == 2 ? "^(\\d+|" + monthesMatch + ")\\.(\\d+)" :
												 "^(\\d+)");
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
	assert (!currentBlock.isEmpty());
	QString firstLine = currentBlock[0];
	assert (!firstLine.isEmpty());

	// Some constructions can be detected easily
	if (firstLine[0] == '[')
	{
		if (firstLine[firstLine.size() - 1] != ']')
			blockParseError (0, "Unclosed square braces in a tag entry: '" + firstLine + "'");

		if (currentBlock.size() != 1)
			blockParseError (1, "Multi-line tag entry.");

		QString newTagName = firstLine.mid (1, firstLine.length() - 2);
		if (newTagName.isEmpty())
			blockParseError (0, "Empty tag name.");

		for (QChar c: newTagName)
			if (!(c == '-' || (c.toLower() >= 'a' && c.toLower() <= 'z')))
				blockParseError (0, QString ("Tag name contains illegal character '") + c + "' (only '-' and latin letters are allowed).");

		currentEntryTag = newTagName;
		return;
	}
	else if (firstLine[0] == '-')
	{
		if (firstLine != "---")
			blockParseError (0, "Expected a separator '---' after the beginning dash, met '" + firstLine + "'.");

		if (currentBlock.size() != 1)
			blockParseError (1, "Multi-line separator entry.");

		if (forwardDate)
			blockParseWarning (0, "The entries are already being assigned a next event's date due to previous separator or file beginning.");

		forwardDate = true;
		return;
	}

	{
		// Try to treat as a dated event
		ComplexDate date;
		if (tryExtractDate (firstLine, date))
		{
			//qstdout << "Got " << date << " and string '" << firstLine << "'" << endl;

			firstLine = firstLine.trimmed();
			if (firstLine.isEmpty() || firstLine[0] != '-')
				blockParseError (0, "Expected a dash followed by an event name after a date interval, " +
								         (firstLine.isEmpty() ? "end of string" : QString ("'") + firstLine[0] + "'") + " found.");

			firstLine = firstLine.right (firstLine.length() - 1).trimmed();

			if (firstLine.isEmpty())
				blockParseError (0, "Empty event name.");

			firstLine[0] = firstLine[0].toUpper();

			if (isEndingSymbol (firstLine[firstLine.length() - 1]) && (firstLine.length() <= 1 || firstLine[firstLine.length() - 2] != '\\'))
				blockParseWarning (0, QString ("An event name ends in unescaped '") + firstLine[firstLine.length() - 1] + "'");

			QRegExp unescapedDash ("^-|[^\\\\]-");

			QString eventDescription = "";
			for (unsigned i = 0; i < currentBlock.size(); i++)
			{
				//if (unescapedDash.indexIn (i == 0 ? firstLine : currentBlock[i]) != -1)
				//	blockParseWarning (i, "Unescaped dash met outside of a date interval/definition construction.");

				if (i == 0) continue;
				eventDescription += (i > 1 ? "\n" : "") + currentBlock[i];
			}

			entryIndexToLine[currentDatabase->entries.size()] = currentBlockFirstLine;
			currentDatabase->entries.push_back (new HistoricalEvent (date, currentEntryTag, replaceEscapes (firstLine), replaceEscapes (eventDescription.trimmed())));

			// Resolve forward dates
			for (unsigned i = 0; i < forwardDateEntriesIndices.size(); i++)
				currentDatabase->entries[forwardDateEntriesIndices[i]]->date = date;
			forwardDateEntriesIndices.clear();
			forwardDate = false;
			lastDate = date;

			return;
		}
	}

	{
		// Try to treat as a term definition

		int numBracketsOpen = 0, dashPosition = -1;
		QChar punctuationMet = 0;
		bool multipleDashes = false;
		bool escaped = false;

		for (int i = 0; i < (int)firstLine.size(); i++)
		{
			if (firstLine[i] == '(')
				numBracketsOpen++;
			else if (firstLine[i] == ')')
				numBracketsOpen--;
			else if (firstLine[i] == '-' && !escaped)
			{
				if (dashPosition != -1)
					multipleDashes = true;
				else
					dashPosition = i;
			}
			else if (dashPosition == -1 && (firstLine[i] == ',' || firstLine[i] == '.' || firstLine[i] == '?'))
				punctuationMet = firstLine[i];

			if (firstLine[i] == '\\')
				escaped = true;
			else
				escaped = false;
		}

		if (dashPosition != -1)
		{
			QString beforeDash = firstLine.left (dashPosition).trimmed();
			if (beforeDash.isEmpty())
				blockParseError (0, "Empty term name.");

			QString afterDash = firstLine.right (firstLine.length() - dashPosition - 1).trimmed();
			if (afterDash.isEmpty())
				blockParseError (0, "Empty term definition.");

			afterDash[0] = afterDash[0].toUpper();

			if (multipleDashes)
				blockParseWarning (0, "Unescaped dash in a term definition.");

			if (!punctuationMet.isNull())
				blockParseWarning (0, QString ("Unescaped punctuation symbol '") + punctuationMet + "' met outside of parentheses.");

			QChar definitionEnd = afterDash[afterDash.length() - 1];
			if ((afterDash.length() == 1 || afterDash[afterDash.length() - 2] != '\\') &&
				(!isEndingSymbol (definitionEnd) || definitionEnd == '?'))
				blockParseWarning (0, QString ("Term definition ends in unescaped '") + definitionEnd + "'.");

			QRegExp unescapedDash ("^-|[^\\\\]-");

			QString inverseQuestion = "";
			for (unsigned i = 1; i < currentBlock.size(); i++)
			{
				if (unescapedDash.indexIn (currentBlock[i]) != -1)
					blockParseWarning (i, "Unescaped dash met in a term inverse question.");

				inverseQuestion += (i > 1 ? "\n" : "") + currentBlock[i];
			}

			entryIndexToLine[currentDatabase->entries.size()] = currentBlockFirstLine;
			currentDatabase->entries.push_back (new HistoricalTerm (forwardDate ? ComplexDate() : lastDate, currentEntryTag,
																	replaceEscapes (beforeDash), replaceEscapes (afterDash), replaceEscapes (inverseQuestion.trimmed())));

			if (forwardDate)
				forwardDateEntriesIndices.push_back ((int)currentDatabase->entries.size() - 1);

			return;
		}
	}

	{
		QRegExp unescapedDash ("^-|[^\\\\]-");

		// Treat as a question
		QString answer = "";

		for (unsigned i = 0; i < currentBlock.size(); i++)
		{
			if (unescapedDash.indexIn (currentBlock[i]) != -1 && i == 0)
				blockParseWarning (i, "Unescaped dash met in a question entry.");

			if (i > 0)
				answer += (i > 1 ? "\n" : "") + currentBlock[i];
		}

		answer = replaceEscapes (answer).trimmed();
		if (answer == "")
			blockParseError (0, "Expected a non-empty answer in a question entry.");

		if (!isEndingSymbol (firstLine[firstLine.length() - 1]) && (firstLine.length() <= 1 || firstLine[firstLine.length() - 2] != '\\'))
			blockParseWarning (0, QString ("Question ends in unescaped '") + firstLine[firstLine.length() - 1] + "'.");

		entryIndexToLine[currentDatabase->entries.size()] = currentBlockFirstLine;
		currentDatabase->entries.push_back (new HistoricalQuestion (forwardDate ? ComplexDate() : lastDate, currentEntryTag,
																    replaceEscapes (firstLine), answer));

		if (forwardDate)
			forwardDateEntriesIndices.push_back ((int)currentDatabase->entries.size() - 1);

		return;
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

	forwardDate = true;
	forwardDateEntriesIndices.clear();
//
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

	for (unsigned i = 0; i < forwardDateEntriesIndices.size(); i++)
	{
		int line = entryIndexToLine[forwardDateEntriesIndices[i]];
		assert (line > 0, "An entry in a line map is missing or invalid for database entry (index: " + QString::number (forwardDateEntriesIndices[i]) + ")");
		currentDatabase->messages.push_back (FileLocationMessage (currentFileName, line, FileLocationMessageType::ERROR,
																  "Failed to assign a date to a dateless entry."));
	}

	return database;
}
