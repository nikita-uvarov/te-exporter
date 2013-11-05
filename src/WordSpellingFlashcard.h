#ifndef WORD_SPELLING_FLASHCARD_H
#define WORD_SPELLING_FLASHCARD_H

#include "FlashcardsDatabase.h"
#include "FlashcardsDatabaseParser.h"

#include <QString>

class RuleTree
{
public :
    QString comeBy, contents;
    QVector <RuleTree> children;
    
    RuleTree (QString comeBy = "") :
        comeBy (comeBy), contents ("")
    {}
    
    QString getRulePath (QString parentPath)
    {
        return parentPath.isEmpty() ? comeBy : parentPath + "." + comeBy;
    }
    
    QString formatSubtree (QString currentPoint, QString highlightPoint);
    
    RuleTree* walkTree (QString rulePoint, bool create);
    
    void dump (QString path = "");
    
    // For better detection of broken paired contructions, invoke on partial strings
    QString escapeRulePart (QString ruleString);
    
    QString replacePaired (QString string, QChar pairSymbol, QString oddReplaceWith, QString evenReplaceWith);
};

class WordSpellingBlockParser : public BlockParser
{
public :
    WordSpellingBlockParser (DatabaseParser* databaseParser) :
        BlockParser (databaseParser), ruleTreeRoot (".")
    {}
    
    bool acceptsBlock (const QVector <QString>& block);
    void parseBlock (QVector <QString>& block);
    
private :
    RuleTree ruleTreeRoot;
    QString currentRulePoint;
    
    void processRuleLine (QString ruleLine);
    QString getRuleReference (QString rulePoint);
    
    void parseWord (QString word, QString& outQuestion, QString& outAnswer, QString& outThirdSide);
};

#endif // WORD_SPELLING_FLASHCARD_H
