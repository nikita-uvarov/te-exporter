#include "Util.h"

#include <cstdio>

#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QDir>

QTextStream qstdin, qstdout, qstderr;
QFile stdinFile, stdoutFile, stderrFile;
bool streamsInitialized = false;

void initializeStandardStreams()
{
	assert (!streamsInitialized, "The standard streams have been initialized already");
	streamsInitialized = true;

	stdoutFile.open (stdout, QIODevice::OpenMode (QIODevice::WriteOnly));
	qstdout.setDevice (&stdoutFile);
	qstdout.setCodec ("UTF-8");

	stderrFile.open (stderr, QIODevice::OpenMode (QIODevice::WriteOnly));
	qstderr.setDevice (&stderrFile);
	qstderr.setCodec ("UTF-8");

	stdinFile.open (stdin, QIODevice::OpenMode (QIODevice::ReadOnly));
	qstdin.setDevice (&stdinFile);
}

void _condition_failure_handler (QString failureTypeString, QString failedCondition, QString file, int line, QString reason)
{
	if (!failedCondition.isEmpty())
		qstderr << failureTypeString << ": '" << failedCondition << "' at line " << line << " of '" << file << "'\n";
	else
		qstderr << failureTypeString << " at line " << line << " of '" << file << "'\n";

	if (!reason.isEmpty())
		qstderr << reason << endl;
	else
		qstderr.flush();
}

void _halt()
{
#	ifdef DEBUG_SOFT_HALT
		exit (1);
#	else
		abort();
#	endif
}

bool FileLocationMessage::operator< (const FileLocationMessage& other) const
{
	if (fileName != other.fileName)
		return fileName < other.fileName;

	if (line != other.line)
		return line < other.line;

	return false;
}

int FileReaderSingletone::pushFileSearchPath (QString fileName, QString context)
{
	includePaths.push_back (QPair <QString, QString> (fileName, context));
	return includePaths.size();
}

void FileReaderSingletone::popFileSearchPath (int pushId)
{
	verify (pushId > 0, "Non-positive push id: " + QString::number (pushId));
	verify (includePaths.size() == pushId, "Path search stack push/pop mismatch (size is " + QString::number (includePaths.size()) + ", pop " + QString::number (pushId) + " requested).");
	includePaths.pop_back();
}

QString FileReaderSingletone::expandPathMacros (QString path)
{
	if (!path.isEmpty() && path[0] == '~')
		path = QDir::homePath() + path.right (path.length() - 1);

	return path;
}

QPair <QString, QString> FileReaderSingletone::readContents (QString fileName, QString context)
{
	fileName = expandPathMacros (fileName);
	QString resultingPath = "";

	QString searched = "";

	QFileInfo pathInfo (fileName);
	if (pathInfo.isAbsolute())
	{
		resultingPath = pathInfo.absoluteFilePath();
	}
	else
	{
		for (int i = includePaths.size() - 1; i >= 0; i--)
			if (includePaths[i].second == context)
			{
				QDir includeDirectory (includePaths[i].first);
				if (includeDirectory.exists())
				{
					if (!searched.isEmpty()) searched += ", ";
					searched += "'" + includeDirectory.absolutePath() + "'";
					resultingPath = includeDirectory.filePath (fileName);
					if (QFile (resultingPath).exists())
						break;
					resultingPath = "";
				}
			}
	}

	verify (resultingPath != "", "File '" + fileName + "' could not be found in search locations for context '" + context + "' (searched " + searched + ").");

	QFile file (resultingPath);
	assert (file.exists(), "File '" + resultingPath + "' could not be opened.");

	file.open (QIODevice::ReadOnly | QIODevice::Text);
	QByteArray fileContents = file.readAll();
	file.close();

	return QPair <QString, QString> (QString::fromUtf8 (fileContents), QFileInfo (resultingPath).absoluteFilePath());
}

FileReaderSingletone& FileReaderSingletone::instance()
{
	static FileReaderSingletone single;
	return single;
}