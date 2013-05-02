#include "SceneryExecutor.h"
#include "Util.h"
#include "DatabaseExporter.h"

#include <QStringList>
#include <QFile>
#include <QDir>

class SceneryCommandParseException : public std::exception
{
public :
	const char* what()
	{
		return "Failed to parse the command; refer to stdout for more info.";
	}
};

void SceneryCommand::parseError (QString what)
{
	qstdout << "Command parse error on line " << commandLine << ": " << what << endl;
	throw SceneryCommandParseException();
}

void SceneryCommand::parse (int commandLine, QString string)
{
	this->commandLine = commandLine;

	QStringList tokens = string.split (' ', QString::SkipEmptyParts);
	name = tokens[0];

	QRegExp switchRegExp ("^-([a-z\\-]+)$");
	QRegExp valueRegExp ("^\"([^\"]*)\"$");

	for (unsigned i = 1; i < tokens.size(); i++)
	{
		if (switchRegExp.indexIn (tokens[i]) == -1)
			parseError (QString ("Invalid switch '" + tokens[i] + "'."));

		if (i + 1 >= tokens.size())
			parseError (QString ("Expected key '" + tokens[i] + "' value."));

		if (valueRegExp.indexIn (tokens[i + 1]) == -1)
			parseError (QString ("Invalid switch '" + tokens[i] + "' value: '" + tokens[i + 1] + "'."));

		QString argumentSwitch = switchRegExp.cap (1), argumentValue = valueRegExp.cap (1);
		if (arguments.count (argumentSwitch) != 0)
			parseError (QString ("Duplicate switch '" + tokens[i] + "'."));
		arguments[argumentSwitch] = argumentValue;
		ignoredArguments.insert (argumentSwitch);

		i++;
	}
}

QString SceneryCommand::getArgument (QString key, QString defaultValue)
{
	if (arguments.count (key) > 0)
	{
		ignoredArguments.remove (key);
		return arguments[key];
	}

	if (defaultValue.isNull())
		failure ("Mandatory argument missing: " + key);

	return defaultValue;
}

void SceneryCommand::dump()
{
	qstdout << "Command: '" << name << "'\n";
	for (auto argument: arguments.toStdMap())
		qstdout << "Argument: '" << argument.first << "', value: '" << argument.second << "'\n";
	qstdout.flush();
}

bool SceneryExecutor::parse()
{
	try
	{
		QStringList lines = sceneryContents.split ('\n');

		int commandLine = 1;
		for (QString& line: lines)
		{
			int commentBegining = line.indexOf ('#');
			if (commentBegining != -1)
				line = line.left (commentBegining);
			line = line.trimmed();

			if (line.isEmpty()) continue;
			SceneryCommand cmd;
			cmd.parse (commandLine, line);
			commands.push_back (cmd);

			commandLine++;
		}

		return true;
	}
	catch (SceneryCommandParseException& e) { return false; }
}

void SceneryExecutor::dump()
{
	for (SceneryCommand& cmd: commands)
		cmd.dump();
}

void SceneryExecutor::execute()
{
	for (SceneryCommand& cmd: commands)
	{
		executeCommand (cmd);

		QString ignoredSwitches = "";
		for (QString ignored: cmd.ignoredArguments)
			ignoredSwitches += (ignoredSwitches.isEmpty() ? "" : ", ") + ignored;

		if (!ignoredSwitches.isEmpty())
			qstdout << "Warning: the following switches were ignored: " << ignoredSwitches << endl;
	}
}

void SceneryExecutor::executeCommand (SceneryCommand& cmd)
{
	/*if (cmd.name == "load_localization")
	{
		QString path = cmd.getArgument ("path");
		QPair <QString, QString> contents = FileReaderSingletone::instance().readContents (path, "global");

		localization.reset (new LocalizationSettings);
		localization->parse (contents.first);
	}
	else */

	if (cmd.name == "load")
	{
		QString dbName = cmd.getArgument ("db"), dbPath = cmd.getArgument ("path");
		if (databases.count (dbName) > 0)
			failure ("Duplicate database '" + dbName + "'.");

		QPair <QString, QString> fileContents = FileReaderSingletone::instance().readContents (dbPath, "global");

		shared_ptr <DatabaseParser> parser (new DatabaseParser);

		QPair <QString, QString> globalHeaderContents = FileReaderSingletone::instance().readContents ("global.txt", DATABASE_INCLUDE_DIRECTORIES_CONTEXT);
		shared_ptr <HistoricalDatabase> globalHeader = parser->parseDatabase (globalHeaderContents.second, globalHeaderContents.first, nullptr);
		shared_ptr <HistoricalDatabase> database = parser->parseDatabase (fileContents.second, fileContents.first, globalHeader);

		shared_ptr <DatabaseExporter> exporter (new DatabaseExporter (database.get()));

		qstdout << "\nDatabase '" << dbName << "' loaded from '" << dbPath << "'.\n";
		qstdout << "Parser messages:" << endl;
		exporter->printMessages();

		qstdout << endl;

		bool wereErrors = false;

		for (FileLocationMessage& m: database->messages)
			if (m.type == FileLocationMessageType::ERROR)
			{
				wereErrors = true;
				break;
			}

		if (wereErrors)
		{
			qstdout << "There are errors in database file, exiting." << endl;
			return;
		}

		databases[dbName] = database;
	}
	else if (cmd.name == "export")
	{
		QString dbName = cmd.getArgument ("db"), deckName = cmd.getArgument ("deck");

		if (databases.count (dbName) == 0)
			failure ("Database not found: '" + dbName + "'.");

		if (decks.count (deckName) == 0)
			decks[deckName] = shared_ptr <HistoricalDeck> (new HistoricalDeck);

		HistoricalEventExportMode eventExportMode = HistoricalEventExportMode::NAME_TO_DATE_AND_DEFINITION;
		QString eventExportModeString = cmd.getArgument ("event-export-mode", "name-to-date-and-definition").toLower();

		if (eventExportModeString == "name-to-date-and-definition")
			eventExportMode = HistoricalEventExportMode::NAME_TO_DATE_AND_DEFINITION;
		else if (eventExportModeString == "name-to-date")
			eventExportMode = HistoricalEventExportMode::NAME_TO_DATE;
		else if (eventExportModeString == "date-to-name")
			eventExportMode = HistoricalEventExportMode::DATE_TO_NAME;
		else
			failure ("Unknown event export mode '" + eventExportModeString + "'.");

		HistoricalTermExportMode termExportMode = HistoricalTermExportMode::DIRECT;
		QString termExportModeString = cmd.getArgument ("term-export-mode", "direct").toLower();
		if (termExportModeString == "direct")
			termExportMode = HistoricalTermExportMode::DIRECT;
		else if (termExportModeString == "inverse")
			termExportMode = HistoricalTermExportMode::INVERSE;
		else
			failure ("Unknown term export mode '" + termExportModeString + "'.");

		shared_ptr <DatabaseExporter> exporter (new DatabaseExporter (databases[dbName].get()));

		qstdout << "Exporting database '" + dbName + "' to deck '" + deckName + "'." << endl;
		exporter->exportDatabase (decks[deckName].get(), eventExportMode, termExportMode);
	}
	else if (cmd.name == "save")
	{
		QString deckName = cmd.getArgument ("deck"), fileName = cmd.getArgument ("path");

		if (decks.count (deckName) == 0)
			failure ("No deck '" + deckName + "' found.");

		fileName = FileReaderSingletone::instance().expandPathMacros (fileName);
		QFile exportTo (fileName);
		verify (exportTo.open (QIODevice::WriteOnly | QIODevice::Text), "Failed to open save destination file '" + fileName + "'.");

		QTextStream exportStream (&exportTo);

		shared_ptr <HistoricalDeck> deck = decks[deckName];
		qstdout << "Writing deck '" << deckName << "' to file '" << fileName << "'." << endl;
		deck->writeDeck (exportStream);
	}
	else if (cmd.name == "remove_duplicates")
	{
		QString deckName = cmd.getArgument ("deck");

		if (decks.count (deckName) == 0)
			failure ("No deck '" + deckName + "' found.");

		decks[deckName]->removeDuplicates();
		qstdout << "Duplicates removed from database '" << deckName << "'." << endl;
	}
	else if (cmd.name == "filter")
	{
		QString sourceDb = cmd.getArgument ("source"), destinationDb = cmd.getArgument ("destination");

		if (databases.count (sourceDb) == 0)
			failure ("Database '" + sourceDb + "' does not exist.");

		if (databases.count (destinationDb) > 0)
			failure ("Duplicate database '" + destinationDb + "'.");

		databases[destinationDb] = shared_ptr <HistoricalDatabase> (new HistoricalDatabase (databases[sourceDb]->variableStack));

		QVector <shared_ptr <HistoricalEntry> >& sourceEntries = databases[sourceDb]->entries,
								               & destinationEntries = databases[destinationDb]->entries;

		// More advanced filters are easy to implement, if really needed (probably date filter would be useless)
		QString tagFilter = cmd.getArgument ("tag", "*");

		for (shared_ptr <HistoricalEntry> e: sourceEntries)
		{
			if (tagFilter != "*" && e->tag != tagFilter) continue;

			destinationEntries.push_back (e);
		}

		qstdout << "Database '" << sourceDb << "' filtered into '" << destinationDb << "' by tag '" << tagFilter << "'." << endl;
	}
	else
	{
		failure ("Unknown command: " + cmd.name);
	}
}
