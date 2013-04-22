#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QTextStream>

/* Standard streams wrappers */

extern QTextStream qstdin, qstdout, qstderr;

void initializeStandardStreams();

/* Custom assertion macros */

void _condition_failure_handler (QString failureTypeString, QString failedCondition, QString file, int line, QString reason = "");

#define DEBUG_SOFT_HALT
void _halt();

#define verify(condition, ...)    ((void)(!(condition) && (_condition_failure_handler ("Verification failed", #condition, __FILE__, __LINE__, ##__VA_ARGS__), 1) && (_halt(), 1)))
#define failure(...)              ((void)(!(false)     && (_condition_failure_handler ("Internal failure"   , ""        , __FILE__, __LINE__, ##__VA_ARGS__), 1) && (_halt(), 1)))

#ifndef DEBUG_DISABLE_ASSERTIONS
#	define assert(condition, ...) ((void)(!(condition) && (_condition_failure_handler ("Assertion failed"   , #condition, __FILE__, __LINE__, ##__VA_ARGS__), 1) && (_halt(), 1)))
#else
#	define assert(condition, ...) ((void)sizeof (condition))
#endif // DEBUG_DISABLE_ASSERTIONS

/* Common messages structures */

enum class FileLocationMessageType
{
	INFO,
	WARNING,
	ERROR
};

class FileLocationMessage
{
public :
	QString fileName;
	int line;

	FileLocationMessageType type;
	QString message;

	FileLocationMessage() = default;

	FileLocationMessage (QString fileName, int line, FileLocationMessageType type, QString message) :
		fileName (fileName), line (line), type (type), message (message)
	{}

	bool operator< (const FileLocationMessage& other) const;
};

QString readContents (QString fileName);

/* By-hand localization support */

class LocalizationSettings
{
public :
	void parse (QString contents);
	const QString& getLocalizedString (const QString& key);

private :
	QMap <QString, QString> strings;
};

#endif // UTIL_H