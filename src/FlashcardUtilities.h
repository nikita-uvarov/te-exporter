#ifndef FLASHCARD_UTILITIES_H
#define FLASHCARD_UTILITIES_H

#include <QTextStream>
#include <QString>
#include <QVector>

const int SIMPLE_DATE_UNSPECIFIED = -1,
          SIMPLE_DATE_NOT_PARSED  = -2;

struct SimpleDate
{
    // Month is greater than zero
    int day, month, year;
    
    SimpleDate() :
        day (SIMPLE_DATE_UNSPECIFIED), month (SIMPLE_DATE_UNSPECIFIED),
        year (SIMPLE_DATE_UNSPECIFIED)
    {}
    
    bool isParsed() const;
    bool isSpecified() const;
};

QTextStream& operator<< (QTextStream& stream, const SimpleDate& date);

struct ComplexDate
{
    SimpleDate begin, end;
    
    QString toString();
};

QTextStream& operator<< (QTextStream& stream, const ComplexDate& date);

bool isEndingSymbol (QChar c);
QString replaceEscapes (QString s);

#endif // FLASHCARD_UTILITIES_H