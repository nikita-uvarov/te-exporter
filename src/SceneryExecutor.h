#ifndef SCENERY_EXECUTOR_H
#define SCENERY_EXECUTOR_H

#include <memory>
#include <QMap>
#include <QString>
#include <QVector>
#include <QSet>

#include "HistoricalDatabase.h"

using std::shared_ptr;

class SceneryCommand
{
public :
	QString name;
	QSet <QString> ignoredArguments;

	void parse (int commandLine, QString string);
	QString getArgument (QString key, QString defaultValue = nullptr);
	void dump();

private :
	int commandLine;

	QMap <QString, QString> arguments;

	void parseError (QString what);
};

class SceneryExecutor
{
public :
	SceneryExecutor (const QString& sceneryContents) :
		sceneryContents (sceneryContents)
	{}

	bool parse();
	void execute();
	void dump();

private :
	QString sceneryContents;
	QVector <SceneryCommand> commands;

	QMap <QString, shared_ptr <HistoricalDatabase> > databases;

	void executeCommand (SceneryCommand& cmd);
};

#endif // SCENERY_EXECUTOR_H