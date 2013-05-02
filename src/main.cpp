#include "SceneryExecutor.h"
#include "Util.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <memory>
#include <QCoreApplication>

using std::shared_ptr;

int main (int argc, char** argv)
{
	// We don't need an event loop. Allows to use its functions without warnings
	QCoreApplication application (argc, argv);

	initializeStandardStreams();
	verify (argc == 2, QString ("Usage: ") + argv[0] + " scenery-file");

	QString invokePath = QDir::currentPath();
	FileReaderSingletone::instance().pushFileSearchPath (invokePath, "global");

	QDir applicationDir (QCoreApplication::applicationDirPath());
	if (applicationDir.exists())
	{
		for (int t = 0; t < 2; t++)
		{
			if (QDir (applicationDir.filePath ("headers")).exists())
				FileReaderSingletone::instance().pushFileSearchPath (applicationDir.filePath ("headers"), DATABASE_INCLUDE_DIRECTORIES_CONTEXT);
			if (!applicationDir.cdUp()) break;
		}
	}

	QPair <QString, QString> sceneryContents = FileReaderSingletone::instance().readContents (argv[1], "global");

	//QString sceneryFilePath = QFileInfo (argv[1]).absolutePath();

	{
		shared_ptr <SceneryExecutor> sceneryExecutor (new SceneryExecutor (sceneryContents.first));
		if (!sceneryExecutor->parse())
		{
			qstdout << "There were parse errors, exiting." << endl;
			return 1;
		}

		//sceneryExecutor->dump();
		sceneryExecutor->execute();
		qstdout.flush();
		qstderr.flush();
	}

	/*QString monthNames = readContents ("../configs/months.txt");

	QString fileName = "../databases/test.txt";
	QString fileContents = readContents (fileName);

	shared_ptr <DatabaseParser> parser (new DatabaseParser);
	parser->parseMonthNames (monthNames);
	shared_ptr <HistoricalDatabase> database = parser->parseDatabase (fileName, fileContents);

	shared_ptr <DatabaseExporter> exporter (new DatabaseExporter (database.get()));

	qstdout << "\nParser messages:" << endl;
	exporter->printMessages();

	qstdout << "\nDatabase dump:" << endl;
	exporter->dump();*/


	return 0;
}