#include "Util.h"

#include <QFile>
#include <cstdio>

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

QString readContents (QString fileName)
{
	QFile file (fileName);
	assert (file.exists(), "File '" + file.fileName() + "' could not be opened");

	file.open (QIODevice::ReadOnly | QIODevice::Text);
	QByteArray fileContents = file.readAll();
	file.close();

	return QString::fromUtf8 (fileContents);
}
