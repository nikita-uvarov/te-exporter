#include "SceneryExecutor.h"
#include "Util.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <memory>

using std::shared_ptr;

int main (int argc, char** argv)
{
	initializeStandardStreams();
	verify (argc == 2, QString ("Usage: ") + argv[0] + " scenery-file");

	QString sceneryContents = readContents (argv[1]);

	QDir::setCurrent (QFileInfo (argv[1]).absolutePath());

	shared_ptr <SceneryExecutor> sceneryExecutor (new SceneryExecutor (sceneryContents));
	if (!sceneryExecutor->parse())
	{
		qstdout << "There were parse errors, exiting." << endl;
		return 1;
	}

	sceneryExecutor->dump();
	sceneryExecutor->execute();

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

	qstdout.flush();
	qstderr.flush();

	return 0;
}