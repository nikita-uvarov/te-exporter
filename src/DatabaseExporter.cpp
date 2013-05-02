#include "DatabaseExporter.h"

#include <QFile>
#include <QStringList>

void DatabaseExporter::printMessages()
{
	qSort (database->messages);

	QString previousFile = "";

	if (database->messages.empty())
		qstdout << "(no messages)" << endl;

	for (const FileLocationMessage& message: database->messages)
	{
		if (message.fileName != previousFile)
		{
			previousFile = message.fileName;
			qstdout << "From '" << previousFile << "'\n";
		}

		qstdout << "on line " << message.line << ": ";

		switch (message.type)
		{
			case FileLocationMessageType::ERROR  : qstdout << "error: "  ; break;
			case FileLocationMessageType::WARNING: qstdout << "warning: "; break;
			case FileLocationMessageType::INFO   : qstdout << "note: "   ; break;
			default: failure ("Unknown file location message type " + QString::number (static_cast <int> (message.type)));
		}

		qstdout << message.message << "\n";
	}
	qstdout.flush();
}

void DatabaseExporter::dump()
{
	for (shared_ptr <HistoricalEntry> e: database->entries)
	{
		qstdout << "Tag: " << e->tag << ", date tag: " << e->date;

		if (HistoricalEvent* event = dynamic_cast <HistoricalEvent*> (e.get()))
		{
			qstdout << ", event name: '" << event->eventName << "', description: '" << event->eventDescription << "'" << endl;
		}
		else if (HistoricalTerm* term = dynamic_cast <HistoricalTerm*> (e.get()))
		{
			qstdout << ", term name: '" << term->termName << "', definition: '" << term->termDefinition << "', inverse question: '" << term->inverseQuestion << "'" << endl;
		}
		else if (HistoricalQuestion* question = dynamic_cast <HistoricalQuestion*> (e.get()))
		{
			qstdout << ", question: '" << question->question << "', answer: '" << question->answer << "'" << endl;
		}
		else
		{
			qstdout << endl;
			failure ("Failed to print entry: unknown entry type.");
		}
	}
}

void HistoricalDeck::writeDeck (QTextStream& stream)
{
	stream.setCodec("UTF-8");

	for (unsigned i = 0; i < usedColumns.size(); i++)
		stream << usedColumns[i] << ((i + 1) == usedColumns.size() ? "\n" : "\t");

	for (std::map <int, QString>& row : exportData)
		for (unsigned i = 0; i < usedColumns.size(); i++)
			stream << row[i] << ((i + 1) == usedColumns.size() ? "\n" : "\t");
}

void HistoricalDeck::setColumnValue (QString columnName, QString columnValue)
{
	int id = -1;
	for (int i = 0; i < usedColumns.size(); i++)
		if (usedColumns[i] == columnName)
		{
			id = i;
			break;
		}

	if (id == -1)
	{
		id = (int)usedColumns.size();
		usedColumns.push_back (columnName);
	}

	currentRow[id] = columnValue;
}

void HistoricalDeck::submitRow()
{
	exportData.push_back (currentRow);
	currentRow.clear();
}

void HistoricalDeck::removeDuplicates()
{
	// Could be done with unique, but don't mess up the ordering.

	std::vector < std::map <int, QString> > exportDataOld = exportData;
	exportData.clear();

	std::set < std::map <int, QString> > met;

	for (unsigned i = 0; i < exportDataOld.size(); i++)
		if (met.count (exportDataOld[i]) == 0)
		{
			met.insert (exportDataOld[i]);
			exportData.push_back (exportDataOld[i]);
		}
}

void DatabaseExporter::exportDatabase (HistoricalDeck* exportTo, HistoricalEventExportMode eventExportMode, HistoricalTermExportMode termExportMode)
{
	this->eventExportMode = eventExportMode;
	this->termExportMode = termExportMode;

	for (shared_ptr <HistoricalEntry> e: database->entries)
		exportEntry (exportTo, e.get());
}

QString surroundPreamble (QString preamble)
{
	return "<i><color #666666>" + preamble + "</i></color>";
}

QString stringToFlashcardsFormat (QString string)
{
	return string.replace ('\n', '|').replace ('\t', "    ").replace (QRegExp ("`([^`])"), "\\1&#769;");
}

QString surroundDateHeader (QString dateHeader)
{
	return "<b>" + dateHeader + "</b>";
}

void DatabaseExporter::exportEntry (HistoricalDeck* exportTo, HistoricalEntry* entry)
{
	QString cardFront = "", cardBack = "";

	if (HistoricalEvent* event = dynamic_cast <HistoricalEvent*> (entry))
	{
		QString preambleMiddle = "";

		if (eventExportMode == HistoricalEventExportMode::DATE_TO_NAME)
		{
			cardFront = complexDateToStringLocalized (event->date, entry);
			cardBack = event->eventName;

			preambleMiddle = "date_to_name";
		}
		else if (eventExportMode == HistoricalEventExportMode::NAME_TO_DATE)
		{
			cardFront = event->eventName;
			cardBack = complexDateToStringLocalized (event->date, entry);

			preambleMiddle = "name_to_date";
		}
		else if (eventExportMode == HistoricalEventExportMode::NAME_TO_DATE_AND_DEFINITION)
		{
			cardFront = event->eventName;
			QString dateHeader = complexDateToStringLocalized (event->date, entry) + (event->eventDescription.isEmpty() ? "" : ":\n");
			cardBack = (event->eventDescription.isEmpty() ? dateHeader : surroundDateHeader (dateHeader) + event->eventDescription);

			preambleMiddle = (event->eventDescription.isEmpty() ? "name_to_date" : "name_to_date_and_definition");
		}
		else failure ("Invalid event export mode.");

		QString preamble = variableValue (preambleMiddle.replace ("date", event->date.end.isSpecified() ? "complex_date" : "simple_date") + "_preamble", entry);
		if (!preamble.isEmpty())
			cardFront += surroundPreamble ("\n(" + preamble + ")");
	}
	else if (HistoricalTerm* term = dynamic_cast <HistoricalTerm*> (entry))
	{
		if (termExportMode == HistoricalTermExportMode::DIRECT)
		{
			cardFront = term->termName;
			QString preamble = variableValue ("term_to_definition_preamble", entry);
			if (!preamble.isEmpty())
				cardFront += surroundPreamble ("\n(" + preamble + ")");

			cardBack = term->termDefinition;
		}
		else if (termExportMode == HistoricalTermExportMode::INVERSE)
		{
			cardFront = term->inverseQuestion;
			QString preamble = variableValue ("definition_to_term_preamble", entry);
			if (!preamble.isEmpty())
				cardFront += surroundPreamble ("\n(" + preamble + ")");

			cardBack = term->termName;
		}
		else failure ("Invalid term export mode.");
	}
	else if (HistoricalQuestion* question = dynamic_cast <HistoricalQuestion*> (entry))
	{
		cardFront = question->question;
		cardBack = question->answer;
	}
	else
	{
		failure ("Failed to export entry: unknown entry type.");
	}

	cardFront = stringToFlashcardsFormat (cardFront);
	cardBack = stringToFlashcardsFormat (cardBack);

	exportTo->setColumnValue ("Text 1", cardFront);
	exportTo->setColumnValue ("Text 2", cardBack);
	exportTo->submitRow();
}

QString DatabaseExporter::complexDateToStringLocalized (ComplexDate date, HistoricalEntry* entry)
{
	// TODO: long dash
	if (date.end.isSpecified())
		return simpleDateToStringLocalized (date.begin, entry) + " &mdash; " + simpleDateToStringLocalized (date.end, entry);
	else
		return simpleDateToStringLocalized (date.begin, entry);
}

QString DatabaseExporter::simpleDateToStringLocalized (SimpleDate date, HistoricalEntry* entry)
{
	const QString space = "&nbsp";

	assert (date.month == SIMPLE_DATE_UNSPECIFIED || (date.month >= 1 && date.month <= 12));

	if (date.month == SIMPLE_DATE_UNSPECIFIED)
		return QString::number (date.year);

	QString iString = (date.month < 10 ? "0" : "") + QString::number (date.month);
	QString months = variableValue ("month_" + iString + "_output", entry);
	QStringList twoWords = months.split (' ', QString::SkipEmptyParts);
	verify (twoWords.size() == 2, "Localization string 'month_" + iString + "_output' must contain exactly two space-separated month names.");

	if (date.day != SIMPLE_DATE_UNSPECIFIED)
		return QString::number (date.day) + space + twoWords[0] + space + QString::number (date.year);
	else
		return twoWords[1] + space + QString::number (date.year);
}

QString DatabaseExporter::variableValue (QString name, HistoricalEntry* entry)
{
	QString value = entry->variableStack.getVariableValue (name);
	verify (!value.isNull(), "Variable '" + name + "' not defined (required by exporter).");
	return value;
}
