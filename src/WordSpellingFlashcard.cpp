#include "WordSpellingFlashcard.h"
#include "QuestionFlashcard.h"

bool isVowel (QChar c)
{
    c = c.toLower();
    
    char16_t vowels[] = u"ауоыиэяюёе";
    for (char16_t x : vowels)
        if (c == x)
            return true;
        
    return false;
}

void WordSpellingBlockParser::parseWord (QString word, QString& outQuestion, QString& outAnswer, QString& outThirdSide)
{
    struct Word
    {
        QString test, answer;
        QList <int> insertStressPositions;

        Word (const QString& test_ = "", const QString& answer_ = "") :
            test (test_), answer (answer_)
        {}

        Word& append (const QString& a, const QString& b)
        {
            test += a;
            answer += b;
            return *this;
        }

        Word& append (const QChar& x)
        {
            test += x;
            answer += x;
            return *this;
        }

        Word& append (const Word& other)
        {
            append (other.test, other.answer);

            for (int i = 0; i < other.insertStressPositions.size(); i++)
                insertStressPositions.push_back (test.size() + other.insertStressPositions[i]);

            return *this;
        }

        void insertStress()
        {
            insertStressPositions.push_back (test.size() - 1);
        }

        QString trimTest (QChar expected)
        {
            verify (test.size() > 0);
            verify (test[0] == expected);

            return test.right (test.size() - 1);
        }
    };

    std::vector <Word> stack;
    stack.push_back (Word ("", ""));

    bool escaped = false;
    
    int answerCommentBegin = word.indexOf ("*");
    QString answerComment = "";
    if (answerCommentBegin >= 0)
    {
        answerComment = word.right (word.size() - answerCommentBegin - 1);
        word = word.left (answerCommentBegin);
    }
    
    for (unsigned i = 0; i < word.length(); i++)
    {
        if (word[i] == '\\')
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            stack.back().append (word[i]);
            escaped = false;
            continue;
        }

        if (word[i] == '(')
        {
            stack.push_back (Word ("(", ""));
        }
        else if (word[i] == ')')
        {
            verify (stack.size() > 1);
            QString inside = stack.back().answer;
            stack.pop_back();
            stack.back().append ("<font color=#FF0000>..</font>", inside);
            outAnswer += inside;
        }
        else if (word[i] == '[')
        {
            stack.push_back (Word ("[", ""));
        }
        else if (word[i] == ']')
        {
            verify (i + 1 < word.length());
            verify (stack.size() > 1);

            QString inside = stack.back().trimTest ('['), answer = stack.back().answer;
            stack.pop_back();

            QChar next = word[++i];
            if (next != ' ' && next != '-')
            {
                stack.back().append (Word ("(", answer).append (inside, "").append (") ", ""));
                i--;
            }
            else
            {
                stack.back().append (Word ("(", answer + next).append (inside, "").append (") ", ""));
            }
        }
        else if (word[i] == '{')
        {
            stack.push_back (Word ("{", ""));
        }
        else if (word[i] == '}')
        {
            verify (stack.size() > 1);

            QString inside = stack.back().trimTest ('{');
            stack.pop_back();
            stack.back().append (Word ("<i><font color=#969696>(", "").append (inside, "").append (")</font></i>", ""));
        }
        else if (word[i] == '`')
        {
            verify (i + 1 < word.length());
            QChar stressed = word[++i];
            stack.back().append (Word (QString (stressed), "<font color=#FF0000>").append ("", QString (stressed)).append ("", "&#769;</font>"));
            stack.back().insertStress();
        }
        else if (word[i] == '!')
        {
            stack.back().append ("<font color=#FF0000>..</font>", "");
        }
        else if (word[i] == '^')
        {
            verify (i + 1 < word.length());
            QChar capitalized = word[++i];
            bool isCapitalized = capitalized != capitalized.toLower();
            Word prefix = Word ("<font color=#969696>", (isCapitalized ? "<font color=#FF0000>" : ""));
            stack.back().append (prefix.append (QString (capitalized.toLower()), QString (capitalized)).append (Word ("</font>", isCapitalized ? "</font>" : "")));
            stack.back().insertStress();
        }
        else
        {
            stack.back().append (word[i]);
        }
    }

    verify (stack.size() == 1);
    //Word output = Word ("<font size=20>", "").append (stack.back()).append ("</font>", "");
    Word output = Word ("<center>", "").append (stack.back()).append ("</center>", "");
    if (!answerComment.isEmpty())
    {
        if (answerComment[0] == ' ')
            output = output.append ("", "<i><font color=#969696>(" + answerComment.trimmed() + ")</font></i>");
        else
        {
            // TODO: commas support
            outThirdSide = getRuleReference (answerComment);
        }
    }
    outQuestion = output.test.trimmed();
    outAnswer = output.answer.trimmed();
}

bool WordSpellingBlockParser::acceptsBlock (const QVector <QString>&)
{
    return true;
}

void WordSpellingBlockParser::parseBlock (QVector <QString>& block)
{
    for (QString line: block)
    {
        if (!line.isEmpty() && line[0] == '*')
        {
            processRuleLine (line);
            continue;
        }
        
        QString question, answer, thirdSide;
        parseWord (line, question, answer, thirdSide);
        
        QString preamble = currentDatabase->variableStack->currentState().getVariableValue ("russian_word_spelling_preamble");
        if (!preamble.isEmpty())
            question += preamble;
        
        currentDatabase->entries.push_back (shared_ptr <SimpleFlashcard> (new QuestionFlashcard (currentEntryTag, currentDatabase->variableStack->currentState(), question, answer, thirdSide)));
    }
}

void WordSpellingBlockParser::processRuleLine (QString ruleLine)
{
    bool attemptPoint = !ruleLine.isEmpty() && ruleLine[0] == '*';
    
    if (attemptPoint)
    {
        ruleLine = ruleLine.trimmed();
        ruleLine = ruleLine.right (ruleLine.length() - 1);
        
        int firstSpace = ruleLine.indexOf (' ');
        if (firstSpace != -1)
        {        
            QString rulePoint = ruleLine.left (firstSpace);
        
            if (!rulePoint.isEmpty() && rulePoint[0].isDigit())
            {        
                QString rest = ruleLine.right (ruleLine.length() - firstSpace - 1);
                
                qstdout << "Proceeding to point " << rulePoint << endl;
                currentRulePoint = rulePoint;
                processRuleLine (rest);
                return;
            }
        }
    }
    
    qstdout << ruleLine << endl;
    
    ruleTreeRoot.dump();
    RuleTree* node = ruleTreeRoot.walkTree (currentRulePoint, true);
    Q_ASSERT (node);
    
    if (node->contents.isEmpty())
        node->contents = ruleLine;
    else
        node->contents = node->contents + "\n" + ruleLine;
}

QString WordSpellingBlockParser::getRuleReference (QString rulePoint)
{
    ruleTreeRoot.dump();
    
    int dotIndex = rulePoint.indexOf ('.');
    QString walkFrom = (dotIndex == -1 ? rulePoint : rulePoint.left (dotIndex));
    QString remainingRulePoint = (dotIndex == -1 ? "" : rulePoint.right (rulePoint.length() - dotIndex - 1));
    RuleTree* node = ruleTreeRoot.walkTree (walkFrom, false);
    Q_ASSERT (node);
    QString rule = node->formatSubtree ("", remainingRulePoint);
    rule = "<size .3>" + rule + "</size>";
    return rule;
}

QString RuleTree::formatSubtree (QString currentPoint, QString highlightPoint)
{
    QString subtree = contents + (contents.isEmpty() ? "" : "\n");
    subtree = escapeRulePart (subtree);
    
    // Print highlighted entries first  
    for (int iteration = 0; iteration < 2; iteration++)
        for (RuleTree& child: children)
        {
            QString childPath = child.getRulePath (currentPoint);
            if (highlightPoint.startsWith (childPath) == 1 - iteration)
                subtree += child.formatSubtree (child.getRulePath (currentPoint), highlightPoint);
        
        }
    if (!currentPoint.isEmpty())
    {
        subtree = "<b>" + currentPoint + ".</b> " + subtree;
    }
    
    if (!highlightPoint.startsWith (currentPoint))
        subtree = "<font color=#969696>" + subtree + "</font>";
        
    return subtree;
}

QString RuleTree::replacePaired (QString string, QChar pairSymbol, QString oddReplaceWith, QString evenReplaceWith)
{
    int nSymbols = string.count (pairSymbol);
    if (nSymbols % 2 != 0)
        failure (QString ("Expected even number of symbols '") + pairSymbol + "', " + QString::number (nSymbols) + " found.");
    
    bool nowOdd = true;
    QString newString = "";
    
    for (QChar c: string)
        if (c == pairSymbol)
        {
            newString += (nowOdd ? oddReplaceWith : evenReplaceWith);
            nowOdd = !nowOdd;
        }
        else
        {
            newString += c;
        }
        
    return newString;
}


QString RuleTree::escapeRulePart (QString ruleString)
{
    QChar doubleAngleQuotationMarkLeft  = QChar (171),
          doubleAngleQuotationMarkRight = QChar (187),
          nonBreakingHyphen             = QChar (8209),
          longEmDash                    = QChar (2014);
    
    ruleString = replacePaired (ruleString, '"', doubleAngleQuotationMarkLeft, doubleAngleQuotationMarkRight);
    ruleString = replacePaired (ruleString, '+', "<b>", "<CLOSETAGb>");
    ruleString = replacePaired (ruleString, '/', "<i>", "<CLOSETAGi>");
    
    ruleString = ruleString.replace (",-", longEmDash);
    ruleString = ruleString.replace (", -", longEmDash);
    ruleString = ruleString.replace (" - ", longEmDash);
    
    ruleString = ruleString.replace ('-', nonBreakingHyphen);
    
    ruleString = ruleString.replace ("CLOSETAG", "/");
    
    return ruleString;
}

RuleTree* RuleTree::walkTree (QString rulePoint, bool create)
{
    if (rulePoint.isEmpty())
        return this;    
    
    int firstDot = rulePoint.indexOf ('.');
    QString proceed = "";
    if (firstDot != -1)
    {
        proceed = rulePoint.right (rulePoint.size() - firstDot - 1);
        rulePoint = rulePoint.left (firstDot);
    }
    
    qstdout << "Finding " << rulePoint << endl;
    qstdout << "Has " << children.size() << " children" << endl;
    for (RuleTree& subtree: children)
        if (subtree.comeBy == rulePoint)
        {
            qstdout << "Proceeding to " << rulePoint << endl;
            return subtree.walkTree (proceed, create);
        }
    
    qstdout << "Creating " << rulePoint << endl;
    
    if (create)
    {
        children.push_back (RuleTree (rulePoint));
        return children.back().walkTree (proceed, create);
    }
    else
    {
        return nullptr;
    }
}

#include "Util.h"

void RuleTree::dump (QString path)
{
    qstderr << path << ". " << contents << " " << children.size() << " children\n";
    qstderr.flush();
    
    for (RuleTree& child: children)
        child.dump (child.getRulePath (path));
}
