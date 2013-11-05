#include "QuestionFlashcard.h"

bool QuestionBlockParser::acceptsBlock (const QVector <QString>&)
{
    return true;
}

void QuestionBlockParser::parseBlock (QVector <QString>& block)
{
    QString firstLine = block.first();
    QRegExp unescapedDash ("^-|[^\\\\]-");
    
    // Treat as a question
    QString answer = "";
    
    for (unsigned i = 0; i < block.size(); i++)
    {
        if (unescapedDash.indexIn (block[i]) != -1 && i == 0)
            blockParseWarning (i, "Unescaped dash met in a question entry.");
        
        if (i > 0)
            answer += (i > 1 ? "\n" : "") + block[i];
    }
    
    answer = replaceEscapes (answer).trimmed();
    if (answer == "")
        blockParseError (0, "Expected a non-empty answer in a question entry.");
    
    if (!isEndingSymbol (firstLine[firstLine.length() - 1]) && (firstLine.length() <= 1 || firstLine[firstLine.length() - 2] != '\\'))
        blockParseWarning (0, QString ("Question ends in unescaped '") + firstLine[firstLine.length() - 1] + "'.");
    
    currentDatabase->entries.push_back
    (shared_ptr <SimpleFlashcard> (new QuestionFlashcard (currentEntryTag, currentDatabase->variableStack->currentState(),
                                                          applyReplacements (replaceEscapes (firstLine)), applyReplacements (answer))));
}
