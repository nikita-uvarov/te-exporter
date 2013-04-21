#include "HistoricalDatabase.h"
#include "DatabaseExporter.h"

#include <QFile>

QString readContents (QString fileName)
{
	QFile file (fileName);
	assert (file.exists(), "File '" + file.fileName() + "' could not be opened");

	file.open (QIODevice::ReadOnly | QIODevice::Text);
	QByteArray fileContents = file.readAll();
	file.close();

	return QString::fromUtf8 (fileContents);
}

int main()
{
	initializeStandardStreams();

	QString monthNames = readContents ("../configs/months.txt");

	QString fileName = "../databases/test.txt";
	QString fileContents = readContents (fileName);

	shared_ptr <DatabaseParser> parser (new DatabaseParser);
	parser->parseMonthNames (monthNames);
	shared_ptr <HistoricalDatabase> database = parser->parseDatabase (fileName, fileContents);

	shared_ptr <DatabaseExporter> exporter (new DatabaseExporter (database.get()));

	qstdout << "\nParser messages:" << endl;
	exporter->printMessages();

	qstdout << "\nDatabase dump:" << endl;
	exporter->dump();

	qstdout.flush();
	qstderr.flush();

	return 0;
}