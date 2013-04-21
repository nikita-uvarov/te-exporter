#include "DatabaseExporter.h"

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
		else
		{
			qstdout << endl;
			failure ("Failed to print entry: unknown entry type.");
		}
	}
}
