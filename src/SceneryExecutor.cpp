#include "SceneryExecutor.h"
#include "Util.h"
#include "DatabaseExporter.h"

#include <QStringList>

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

	QRegExp switchRegExp ("^-([-a-z]+)$");
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
	if (cmd.name == "load")
	{
		QString dbName = cmd.getArgument ("db"), dbPath = cmd.getArgument ("path"), monthsPath = cmd.getArgument ("months");
		if (databases.count (dbName) > 0)
			failure ("Duplicate database '" + dbName + "'.");

		QString monthNames = readContents (monthsPath);

		QString fileContents = readContents (dbPath);

		shared_ptr <DatabaseParser> parser (new DatabaseParser);
		parser->parseMonthNames (monthNames);
		shared_ptr <HistoricalDatabase> database = parser->parseDatabase (dbPath, fileContents);

		shared_ptr <DatabaseExporter> exporter (new DatabaseExporter (database.get()));

		qstdout << "\nParser messages:" << endl;
		exporter->printMessages();

		qstdout << "\nDatabase '" << dbName << "' loaded from '" << dbPath << "'." << endl;

		databases[dbName] = database;
	}
	else if (cmd.name == "export")
	{
		QString dbName = cmd.getArgument ("db"), deckName = cmd.getArgument ("deck"), exportPath = cmd.getArgument ("path");
		if (databases.count (dbName) == 0)
			failure ("Database not found: '" + dbName + "'.");

		shared_ptr <DatabaseExporter> exporter (new DatabaseExporter (databases[dbName].get()));
		qstdout << "Exporting database '" + dbName + "' to deck '" + deckName + "'." << endl;
		exporter->exportDatabase (deckName, exportPath);

		// TODO: Output messages
		// TODO: Export switches
	}
}
