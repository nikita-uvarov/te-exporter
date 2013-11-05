#include "DatabaseExporter.h"

#include <QFile>
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <QProcess>

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
	for (shared_ptr <SimpleFlashcard> e: database->entries)
	{
        e->dump (qstdout);
	}
}

void FlashcardsDeck::writeDeck (QTextStream& stream)
{
	stream.setCodec("UTF-8");

	for (unsigned i = 0; i < usedColumns.size(); i++)
		stream << usedColumns[i] << ((i + 1) == usedColumns.size() ? "\n" : "\t");

	for (std::map <int, QString>& row : exportData)
		for (unsigned i = 0; i < usedColumns.size(); i++)
			stream << row[i] << ((i + 1) == usedColumns.size() ? "\n" : "\t");
}

void FlashcardsDeck::setColumnValue (QString columnName, QString columnValue)
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

void FlashcardsDeck::submitRow()
{
	exportData.push_back (currentRow);
	currentRow.clear();
}

void FlashcardsDeck::removeDuplicates()
{
	// Could be done with unique, but don't mess up the ordering.

	std::vector < std::map <int, QString> > exportDataOld = exportData;
	exportData.clear();

	std::set < std::map <int, QString> > met;

	for (unsigned i = 0; i < exportDataOld.size(); i++)
		if (met.count (exportDataOld[i]) == 0)
		{
			met.insert (exportDataOld[i]);
			exportData.push_back (exportDataOld[i]);
		}
}

QString FlashcardsDeck::getResourceDeckPath (QString resourceAbsolutePath)
{
	if (resourceAbsoluteToDeckPathMap.count (resourceAbsolutePath) > 0)
		return resourceAbsoluteToDeckPathMap[resourceAbsolutePath];

	int dotPosition = resourceAbsolutePath.lastIndexOf ('.');
	verify (dotPosition != -1, "Resource '" + resourceAbsolutePath + "' has no extension.");
	QString extension = resourceAbsolutePath.right (resourceAbsolutePath.length() - dotPosition);
	QString newName = QString::number (resourceAbsoluteToDeckPathMap.size() + 1) + extension;
	resourceAbsoluteToDeckPathMap[resourceAbsolutePath] = newName;
	return newName;
}

void FlashcardsDeck::setMediaDirectoryName (QString name)
{
	mediaDirectoryName = name;
}

void FlashcardsDeck::saveResources (QString deckFileName, bool verbose)
{
	if (resourceAbsoluteToDeckPathMap.empty()) return;
	verify (!(mediaDirectoryName.isEmpty() || mediaDirectoryName.isNull()), "Media directory not specified or empty.");

	QDir createMediaIn = QFileInfo (deckFileName).dir();
	verify (createMediaIn.exists(), "Failed to access parent directory of '" + deckFileName + "'.");

	verify (createMediaIn.mkpath (mediaDirectoryName), "Failed to create media subdirectory '" + mediaDirectoryName + "' for deck file '" + deckFileName +"'.");

	for (std::pair <QString, QString> absoluteToRelative : resourceAbsoluteToDeckPathMap.toStdMap())
	{
		QString destination = createMediaIn.absolutePath() + "/" + mediaDirectoryName + "/" + absoluteToRelative.second;

		QFileInfo sourceFile (absoluteToRelative.first), destinationFile (destination);
		if (destinationFile.exists() && sourceFile.lastModified() == destinationFile.lastModified() && sourceFile.size() == destinationFile.size())
		{
			if (verbose)
				qstderr << "File '" << absoluteToRelative.first << "' has same last accessed time as '" << destination << "': skipping." << endl;
			continue;
		}

		//bool copied = QFile::copy (absoluteToRelative.first, destination);

		// Copy preserving timestamps via 'cp'
		bool copied = false;

#ifdef  Q_OS_LINUX
		QProcess process;
		process.start ("cp", QStringList() << sourceFile.absoluteFilePath() << destinationFile.absoluteFilePath() << "--preserve=timestamps");
		verify (process.waitForFinished(), "Failed to wait for 'cp' to finish.");
		copied = process.exitCode() == 0;
#else
#error This platform is not supported. Add more cases or test if the existing code works.
#endif

		verify (copied, "Failed to copy '" + absoluteToRelative.first + "' to '" + destination + "'.");

		if (verbose)
			qstderr << "Copied resource '" + absoluteToRelative.first + "' to '" + destination + "'." << endl;
	}
	if (verbose)
		qstderr << resourceAbsoluteToDeckPathMap.size() << " resources saved." << endl;
}

void DatabaseExporter::exportDatabase (FlashcardsDeck* exportTo, VariableStackState exportArguments)
{
	for (shared_ptr <SimpleFlashcard> e: database->entries)
    {
        e->setFallbackVariableStackState (exportArguments);
		exportEntry (exportTo, e.get());
        e->removeFallbackVariableStackState();
    }
}

QString stringToFlashcardsFormat (QString string)
{
	return string.replace ('\n', '|').replace ('\t', "    ").replace (QRegExp ("`([^`])"), "\\1&#769;");
}

void DatabaseExporter::exportEntry (FlashcardsDeck* exportTo, SimpleFlashcard* entry)
{
	QString cardFront = "", cardBack = "";

    cardFront = entry->getFrontSide();
    cardBack = entry->getBackSide();

	cardFront = stringToFlashcardsFormat (cardFront);
	cardBack = stringToFlashcardsFormat (cardBack);

	exportTo->setColumnValue ("Text 1", cardFront);
	exportTo->setColumnValue ("Text 2", cardBack);
    
    if (entry->thirdSidePresent())
    {
        QString cardThird = entry->getThirdSide();
        cardThird = stringToFlashcardsFormat (cardThird);
        
        exportTo->setColumnValue ("Text 3", cardThird);
    }
    
	exportTo->submitRow();
}

/*QString DatabaseExporter::complexDateToStringLocalized (ComplexDate date, SimpleFlashcard* entry)
{
	if (date.end.isSpecified())
		return simpleDateToStringLocalized (date.begin, entry) + " &mdash; " + simpleDateToStringLocalized (date.end, entry);
	else
		return simpleDateToStringLocalized (date.begin, entry);
}

QString DatabaseExporter::simpleDateToStringLocalized (SimpleDate date, SimpleFlashcard* entry)
{
	const QString space = "&nbsp";

	assert (date.month == SIMPLE_DATE_UNSPECIFIED || (date.month >= 1 && date.month <= 12));

	if (date.month == SIMPLE_DATE_UNSPECIFIED)
		return QString::number (date.year);

	QString iString = (date.month < 10 ? "0" : "") + QString::number (date.month);
	QString months = variableValue ("month_" + iString + "_output", entry);
	QStringList twoWords = months.split (' ', QString::SkipEmptyParts);
	verify (twoWords.size() == 2, "Localization string 'month_" + iString + "_output' must contain exactly two space-separated month names.");

	if (date.day != SIMPLE_DATE_UNSPECIFIED)
		return QString::number (date.day) + space + twoWords[0] + space + QString::number (date.year);
	else
		return twoWords[1] + space + QString::number (date.year);
}

QString DatabaseExporter::variableValue (QString name, SimpleFlashcard* entry)
{
	QString value = entry->getVariableStackState().getVariableValue (name);
	verify (!value.isNull(), "Variable '" + name + "' not defined (required by exporter).");
	return value;
}

bool DatabaseExporter::booleanVariableValue (QString name, SimpleFlashcard* entry)
{
	QString value = variableValue (name, entry);
	verify (value == "true" || value == "false", "Variable '" + name + "' has non-boolean value '" + value + "'.");
	return value == "true";
}*/
