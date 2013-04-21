#include "HistoricalDatabase.h"
#include "DatabaseExporter.h"

int main()
{
	QFile file ("test.txt");
	file.open (QIODevice::ReadOnly | QIODevice::Text);
	QByteArray fileContents = file.readAll();
	file.close();

	return 0;
}