#include "HistoricalDatabase.h"

#include <QString>
#include <QStringList>
#include <QFileInfo>

const char* DATABASE_INCLUDE_DIRECTORIES_CONTEXT = "database-include-directories";

HistoricalEntry::~HistoricalEntry()
{}

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

void DatabaseParser::updateMonthNames()
{
	obsoleteMonthNames = false;
	for (unsigned i = 1; i <= 12; i++)
	{
		QString variableName = QString ("month_") + (i < 10 ? "0" : "") + QString::number (i) + "_input_variants";
		QString variants = currentDatabase->variableStack->currentState().getVariableValue (variableName);
		if (variants.isNull())
			blockParseError (0, "Failed to start parsing block: variants for month " + QString::number (i) + " missing ('" + variableName + "' not defined).");

		QStringList monthNames = variants.split (' ', QString::SkipEmptyParts);
		assert (!monthNames.empty(), "Every month must have at least one name: missing month " + QString::number(i + 1));

		for (QString name: monthNames)
			monthNameToIndex[name.toLower()] = i;
	}
}

bool DatabaseParser::tryExtractSimpleDate (QString& line, SimpleDate& date)
{
	if (obsoleteMonthNames)
		updateMonthNames();

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
			currentDatabase->entries.push_back (
				shared_ptr <HistoricalEntry> (new HistoricalEvent (date, currentEntryTag, currentDatabase->variableStack->currentState(),
																   applyReplacements (replaceEscapes (firstLine)), applyReplacements (replaceEscapes (eventDescription.trimmed())))));

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
			currentDatabase->entries.push_back
				(shared_ptr <HistoricalEntry> (new HistoricalTerm (forwardDate ? ComplexDate() : lastDate, currentEntryTag, currentDatabase->variableStack->currentState(),
				                                                   applyReplacements (replaceEscapes (beforeDash)), applyReplacements (replaceEscapes (afterDash)),
											                       applyReplacements (replaceEscapes (inverseQuestion.trimmed())))));

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
		currentDatabase->entries.push_back
			(shared_ptr <HistoricalEntry> (new HistoricalQuestion (forwardDate ? ComplexDate() : lastDate, currentEntryTag, currentDatabase->variableStack->currentState(),
																   applyReplacements (replaceEscapes (firstLine)), applyReplacements (answer))));

		if (forwardDate)
			forwardDateEntriesIndices.push_back ((int)currentDatabase->entries.size() - 1);

		return;
	}
}

void DatabaseParser::processDirective (QString directive, shared_ptr <HistoricalDatabase> database)
{
	const QString includeDirectivePrefix = "#include ",
				  pushDirectivePrefix = "#push ",
				  popDirectivePrefix = "#pop ",
				  imageDirectivePrefix = "#image ",
				  pushReplacementPrefix = "#push_replacement",
				  popReplacementPrefix = "#pop_replacement";

	if (directive.startsWith (includeDirectivePrefix))
	{
		QString includePath = directive.right (directive.length() - includeDirectivePrefix.length());
		shared_ptr <DatabaseParser> subParser (new DatabaseParser);
		QPair <QString, QString> contentsNamePair = FileReaderSingletone::instance().readContents (includePath, DATABASE_INCLUDE_DIRECTORIES_CONTEXT);
		subParser->parseDatabase (contentsNamePair.second, contentsNamePair.first, database);
	}
	else if (directive.startsWith (pushDirectivePrefix))
	{
		directive = directive.right (directive.length() - pushDirectivePrefix.length());

		QString name = "", value = "";

		int valueColonIndex = directive.indexOf (':');
		if (valueColonIndex != -1)
		{
			if (directive.length() <= valueColonIndex + 1 || directive[valueColonIndex + 1] != ' ')
				blockParseError (0, "Space expected after a colon in a variable push directive.");

			name = directive.left (valueColonIndex).trimmed();
			value = directive.right (directive.length() - valueColonIndex - 2);
		}
		else
		{
			name = directive.trimmed();
		}

		if (name.isEmpty())
			blockParseError (0, "Variable name is empty.");

		for (QChar c: name)
			if (!c.isLetterOrNumber() && c != '_')
				blockParseError (0, QString ("Invalid character '") + c + "' in variable name '" + name + "'.");

		//qstderr << "Push variable '" << name << "' with value '" << value << "'" << endl;
		currentDatabase->variableStack->pushVariable (name, value);

		if (name.startsWith ("month_") && name.endsWith ("_input_variants"))
			obsoleteMonthNames = true;
	}
	else if (directive.startsWith (popDirectivePrefix))
	{
		directive = directive.right (directive.length() - popDirectivePrefix.length()).trimmed();

		if (!currentDatabase->variableStack->popVariable (directive))
			blockParseError (0, directive.isEmpty() ? "Failed to pop variable: the stack is empty." : "Failed to pop variable '" + directive + "': not found on stack.");
	}
	else if (directive.startsWith (imageDirectivePrefix))
	{
		directive = directive.right (directive.length() - imageDirectivePrefix.length()).trimmed();
		directive = FileReaderSingletone::instance().getAbsolutePath (directive, DATABASE_INCLUDE_DIRECTORIES_CONTEXT);

		currentDatabase->variableStack->pushVariable ("image", directive);
	}
	else if (directive.startsWith (pushReplacementPrefix))
	{
		directive = directive.right (directive.length() - pushReplacementPrefix.length());

		QChar delimiter = directive.isEmpty() ? QChar ('.') : directive[0];

		if (directive.isEmpty())
			blockParseError (0, "Directive '" + pushReplacementPrefix + "' expects a delimiter character.");
		else
			directive = directive.right(directive.size() - 1);

		QStringList parts = directive.split (delimiter, QString::KeepEmptyParts);

		if (parts.size() != 3)
			blockParseError (0, QString ("Delimiter '") + delimiter + "' breaks '" + directive + "' into " + QString::number (parts.size()) + " parts, 3 expected.");

		if (!parts[2].isEmpty())
			blockParseError (0, "Non-empty last part of a '" + pushReplacementPrefix + "' directive: '" + parts[2] + "'.");

		currentDatabase->replacements.push_back (QPair <QString, QString> (parts[0], parts[1]));
	}
	else if (directive.startsWith (popReplacementPrefix))
	{
		if (directive != popReplacementPrefix)
			blockParseError (0, "Directive '" + popReplacementPrefix + "' has no parameters.");

		if (currentDatabase->replacements.empty())
			blockParseError (0, "Replacements stack is empty.");

		currentDatabase->replacements.pop_back();
	}
	else
	{
		blockParseError (0, "Unknown directive: '" + directive + "'.");
	}
}

shared_ptr <HistoricalDatabase> DatabaseParser::parseDatabase (QString fileName, const QString& fileContents, shared_ptr <HistoricalDatabase> appendTo)
{
	if (!appendTo)
		appendTo.reset (new HistoricalDatabase (shared_ptr <VariableStack> (new VariableStack)));

	currentDatabase = appendTo.get();

	int currentDirectoryPushId = FileReaderSingletone::instance().pushFileSearchPath (QFileInfo (fileName).absolutePath(), DATABASE_INCLUDE_DIRECTORIES_CONTEXT);

	QStringList lines = fileContents.split ('\n');

	currentBlock.clear();
	currentFileName = fileName;
	currentEntryTag = "no-tag";

	forwardDate = true;
	forwardDateEntriesIndices.clear();

	obsoleteMonthNames = true;
	
	bool cPlusPlusCommentOpen = false;

	for (int lineNumber = 0; lineNumber < lines.size(); lineNumber++)
	{
		QString& line = lines[lineNumber];

		if (cPlusPlusCommentOpen)
		{
			int commentClosing = line.indexOf ("*/");
			if (commentClosing != -1)
			{
				line = line.right (line.size() - commentClosing - 2);
				cPlusPlusCommentOpen = false;
			}
			else
			{
				line = "";
			}
		}

		bool directive = false;

		if (!line.isEmpty() && line[0] == '#')
		{
			directive = true;
		}
		else
		{
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
			{
				if (currentBlock.isEmpty())
					currentBlockFirstLine = lineNumber + 1;

				currentBlock.push_back (line);
			}
		}

		if ((directive || line.isEmpty() || lineNumber + 1 == lines.size()) && !currentBlock.empty())
		{
			try
			{
				processCurrentBlock();
			}
			catch (DatabaseParserException&) {}

			currentBlock.clear();
		}


		if (directive)
		{
			currentBlockFirstLine = lineNumber + 1;
			try
			{
				processDirective (line, appendTo);
			}
			catch (DatabaseParserException&) {}
		}
	}

	for (unsigned i = 0; i < forwardDateEntriesIndices.size(); i++)
	{
		int line = entryIndexToLine[forwardDateEntriesIndices[i]];
		assert (line > 0, "An entry in a line map is missing or invalid for database entry (index: " + QString::number (forwardDateEntriesIndices[i]) + ")");
		//currentDatabase->messages.push_back (FileLocationMessage (currentFileName, line, FileLocationMessageType::ERROR,
		//														  "Failed to assign a date to a dateless entry."));
	}

	FileReaderSingletone::instance().popFileSearchPath (currentDirectoryPushId);
	currentDatabase = nullptr;
	return appendTo;
}

QString DatabaseParser::applyReplacements (QString str)
{
	for (int i = 0; i < currentDatabase->replacements.size(); i++)
		str = str.replace (QRegExp (currentDatabase->replacements[i].first), currentDatabase->replacements[i].second);
	return str;
}
