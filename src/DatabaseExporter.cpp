#include "DatabaseExporter.h"

#include <QFile>

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
	for (HistoricalEntry* e: database->entries)
	{
		qstdout << "Tag: " << e->tag << ", date tag: " << e->date;

		if (HistoricalEvent* event = dynamic_cast <HistoricalEvent*> (e))
		{
			qstdout << ", event name: '" << event->eventName << "', description: '" << event->eventDescription << "'" << endl;
		}
		else if (HistoricalTerm* term = dynamic_cast <HistoricalTerm*> (e))
		{
			qstdout << ", term name: '" << term->termName << "', definition: '" << term->termDefinition << "', inverse question: '" << term->inverseQuestion << "'" << endl;
		}
		else if (HistoricalQuestion* question = dynamic_cast <HistoricalQuestion*> (e))
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

void DatabaseExporter::setColumnValue (QString columnName, QString columnValue)
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

void DatabaseExporter::submitRow()
{
	exportData.push_back (currentRow);
	currentRow.clear();
}

void DatabaseExporter::exportDatabase (QString deckName, QString exportPath)
{
	usedColumns.clear();
	exportData.clear();
	currentRow.clear();

	QFile file (exportPath + "/" + deckName + ".txt");
	file.open (QFile::WriteOnly);

	QTextStream stream (&file);
	stream.setCodec("UTF-8");

	for (HistoricalEntry* e: database->entries)
		exportEntry (e);

	for (unsigned i = 0; i < usedColumns.size(); i++)
		stream << usedColumns[i] << ((i + 1) == usedColumns.size() ? "\n" : "\t");

	for (std::map <int, QString>& row : exportData)
		for (unsigned i = 0; i < usedColumns.size(); i++)
			stream << row[i] << ((i + 1) == usedColumns.size() ? "\n" : "\t");

	qstdout << "Deck '" << deckName << "' exported to '" << file.fileName() << "'." << endl;
}

void DatabaseExporter::exportEntry (HistoricalEntry* entry)
{
	if (HistoricalEvent* event = dynamic_cast <HistoricalEvent*> (entry))
	{
		setColumnValue ("Text 1", event->date.toString().replace ('\n', '|'));
		setColumnValue ("Text 2", event->eventName.replace ('\n', '|'));
		submitRow();
	}
	else if (HistoricalTerm* term = dynamic_cast <HistoricalTerm*> (entry))
	{
		//qstdout << ", term name: '" << term->termName << "', definition: '" << term->termDefinition << "', inverse question: '" << term->inverseQuestion << "'" << endl;
	}
	else if (HistoricalQuestion* question = dynamic_cast <HistoricalQuestion*> (entry))
	{
		//qstdout << ", question: '" << question->question << "', answer: '" << question->answer << "'" << endl;
	}
	else
	{
		failure ("Failed to export entry: unknown entry type.");
	}

}
