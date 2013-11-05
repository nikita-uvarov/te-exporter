#ifndef QUESTION_FLASHCARD_H
#define QUESTION_FLASHCARD_H

#include "FlashcardsDatabase.h"
#include "FlashcardsDatabaseParser.h"

#include <QString>

class QuestionFlashcard : public SimpleFlashcard
{
    QString question, answer, thirdSide;
    
public :
    QuestionFlashcard (QString tag, VariableStackState stackState, QString question, QString answer, QString thirdSide = nullptr) :
        SimpleFlashcard (tag, stackState), question (question), answer (answer), thirdSide (thirdSide)
    {}
    
    QString getFrontSide()
    {
        return question;
    }
    
    QString getBackSide()
    {
        return answer;
    }
    
    QString getThirdSide()
    {
        return thirdSide;
    }
    
    bool thirdSidePresent()
    {
        return thirdSide != nullptr;
    }
};

class QuestionBlockParser : public BlockParser
{
public :
    QuestionBlockParser (DatabaseParser* databaseParser) :
        BlockParser (databaseParser)
    {}
    
    bool acceptsBlock (const QVector <QString>& block);
    void parseBlock (QVector <QString>& block);
};

#endif // QUESTION_FLASHCARD_H
