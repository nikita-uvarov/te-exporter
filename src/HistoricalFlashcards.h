#ifndef HISTORICAL_FLASHCARDS_H
#define HISTORICAL_FLASHCARDS_H

#include "FlashcardsDatabase.h"
#include "FlashcardUtilities.h"
#include "FlashcardsDatabaseParser.h"

// Historical flashcards support module

class HistoricalEventFlashcard : public SimpleFlashcard
{
    ComplexDate eventDate;
    QString eventName, eventDescription;
    
    QPair <QString, QString> getBothSides();
    
    QString complexDateToStringLocalized (ComplexDate date);
    QString simpleDateToStringLocalized (SimpleDate date);
    
public :
    HistoricalEventFlashcard (ComplexDate eventDate, QString tag, VariableStackState variableStackState, QString eventName, QString eventDescription);
    
    QString getBackSide();
    QString getFrontSide();
};

class HistoricalTermFlashcard : public SimpleFlashcard
{
    QString termName, termDefinition, inverseQuestion;
    
    QPair <QString, QString> getBothSides();
    
public :
    HistoricalTermFlashcard (QString tag, VariableStackState variableStackState, QString termName, QString termDefinition, QString inverseQuestion);

    QString getBackSide();
    QString getFrontSide();
};

class HistoryBlockParser : public BlockParser
{
public :
    HistoryBlockParser (DatabaseParser* databaseParser) :
        BlockParser (databaseParser)
    {}
    
    bool acceptsBlock (const QVector <QString>& block);
    void parseBlock (QVector <QString>& block);
    
private :
    QMap <QString, int> monthNameToIndex;
    
    bool tryExtractDate (QString& line, ComplexDate& date);
    bool tryExtractSimpleDate (QString& line, SimpleDate& date);
    void updateMonthNames();
};

#endif // HISTORICAL_FLASHCARDS_H
