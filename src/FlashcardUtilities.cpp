#include "FlashcardUtilities.h"

QTextStream& operator<< (QTextStream& stream, const SimpleDate& date)
{
    if (!date.isParsed())
        stream << "(not parsed)";
    else if (date.day != SIMPLE_DATE_UNSPECIFIED)
        stream << date.day << "." << date.month << "." << date.year;
    else if (date.month != SIMPLE_DATE_UNSPECIFIED)
        stream << date.month << "." << date.year;
    else if (date.year != SIMPLE_DATE_UNSPECIFIED)
        stream << date.year;
    else
        stream << "(not specified)";
    
    return stream;
}

QTextStream& operator<< (QTextStream& stream, const ComplexDate& date)
{
    stream << "(c-date: ";
    if (date.begin.isSpecified())
    {
        if (date.end.isSpecified())
            stream << date.begin << "-" << date.end;
        else
            stream << date.begin;
    }
    else
    {
        stream << "not specified";
    }
    stream << ")";
    
    return stream;
}

QString ComplexDate::toString()
{
    QString str = "";
    QTextStream stream (&str);
    stream << *this;
    return str;
}

bool isEndingSymbol (QChar c)
{
    return c == '.' || c == '?' || c == '!' || c == ')';
}

QString replaceEscapes (QString s)
{
    return s.replace ("\\n", "\n").replace ('\\', "");
}

bool SimpleDate::isParsed() const
{
    return year != SIMPLE_DATE_NOT_PARSED && month != SIMPLE_DATE_NOT_PARSED && day != SIMPLE_DATE_NOT_PARSED;
}

bool SimpleDate::isSpecified() const
{
    return year != SIMPLE_DATE_UNSPECIFIED;
}