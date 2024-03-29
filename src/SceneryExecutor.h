#ifndef SCENERY_EXECUTOR_H
#define SCENERY_EXECUTOR_H

#include <memory>
#include <QMap>
#include <QString>
#include <QVector>
#include <QSet>
#include <QStringList>

#include "FlashcardsDatabaseParser.h"
#include "DatabaseExporter.h"

using std::shared_ptr;

class SceneryCommand
{
public :
	QString name;
	QSet <QString> ignoredArguments;

	void parse (int commandLine, QString string);
	QString getArgument (QString key, QString defaultValue = nullptr);
	QStringList getKeys();
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
		sceneryContents (sceneryContents), exportDirectoryUrl (QString::null)
	{}

	bool parse();
	void execute();
	void dump();

private :
	QString sceneryContents;
	QVector <SceneryCommand> commands;
	QString exportDirectoryUrl;

	QMap <QString, shared_ptr <FlashcardsDatabase> > databases;
	QMap <QString, shared_ptr <FlashcardsDeck> > decks;

	void executeCommand (SceneryCommand& cmd);
};

#endif // SCENERY_EXECUTOR_H