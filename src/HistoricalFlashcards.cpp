#include "HistoricalFlashcards.h"

HistoricalEventFlashcard::HistoricalEventFlashcard (ComplexDate eventDate, QString tag, VariableStackState variableStackState, QString eventName, QString eventDescription) :
    SimpleFlashcard (tag, variableStackState), eventDate (eventDate), eventName (eventName), eventDescription (eventDescription)
{}

QString HistoricalEventFlashcard::complexDateToStringLocalized (ComplexDate date)
{
    if (date.end.isSpecified())
        return simpleDateToStringLocalized (date.begin) + " &mdash; " + simpleDateToStringLocalized (date.end);
    else
        return simpleDateToStringLocalized (date.begin);
}

QString HistoricalEventFlashcard::simpleDateToStringLocalized (SimpleDate date)
{
    const QString space = "&nbsp";
    
    assert (date.month == SIMPLE_DATE_UNSPECIFIED || (date.month >= 1 && date.month <= 12));
    
    if (date.month == SIMPLE_DATE_UNSPECIFIED)
        return QString::number (date.year);
    
    QString iString = (date.month < 10 ? "0" : "") + QString::number (date.month);
    QString months = variableValue ("month_" + iString + "_output");
    QStringList twoWords = months.split (' ', QString::SkipEmptyParts);
    verify (twoWords.size() == 2, "Localization string 'month_" + iString + "_output' must contain exactly two space-separated month names.");
    
    if (date.day != SIMPLE_DATE_UNSPECIFIED)
        return QString::number (date.day) + space + twoWords[0] + space + QString::number (date.year);
    else
        return twoWords[1] + space + QString::number (date.year);
}

QString surroundDateHeader (QString dateHeader)
{
    return "<b>" + dateHeader + "</b>";
}

QPair <QString, QString> HistoricalEventFlashcard::getBothSides()
{
    QString cardFront = "", cardBack = "";
    
    QString eventExportMode = variableValue ("history-event-export-mode");
    QString preambleMiddle = "";
    
    if (eventExportMode == "date-to-name")
    {
        cardFront = complexDateToStringLocalized (eventDate);
        cardBack = eventName;
        
        preambleMiddle = "date_to_name";
    }
    else if (eventExportMode == "name-to-date")
    {
        cardFront = eventName;
        cardBack = complexDateToStringLocalized (eventDate);
        
        preambleMiddle = "name_to_date";
    }
    else if (eventExportMode == "name-to-date-and-definition")
    {
        cardFront = eventName;
        QString dateHeader = complexDateToStringLocalized (eventDate) + (eventDescription.isEmpty() ? "" : ":\n");
        cardBack = (eventDescription.isEmpty() ? dateHeader : surroundDateHeader (dateHeader) + eventDescription);
          
        preambleMiddle = (eventDescription.isEmpty() ? "name_to_date" : "name_to_date_and_definition");
    }
    else failure ("Invalid event export mode: '" + eventExportMode + "'.");
    
    QString preamble = variableValue (preambleMiddle.replace ("date", eventDate.end.isSpecified() ? "complex_date" : "simple_date") + "_preamble");
    if (!preamble.isEmpty())
        cardFront += preamble;
    
    return QPair <QString, QString> (cardFront, cardBack);
}

QString HistoricalEventFlashcard::getFrontSide()
{
    return getBothSides().first;
}

QString HistoricalEventFlashcard::getBackSide()
{
    return getBothSides().second;
}

HistoricalTermFlashcard::HistoricalTermFlashcard (QString tag, VariableStackState variableStackState, QString termName, QString termDefinition, QString inverseQuestion) :
    SimpleFlashcard (tag, variableStackState), termName (termName), termDefinition (termDefinition), inverseQuestion (inverseQuestion)
{}

QPair <QString, QString> HistoricalTermFlashcard::getBothSides()
{
    QString cardFront = "", cardBack = "";
    QString termExportMode = variableValue ("history-term-export-mode");
    
    if (termExportMode == "direct")
    {
        cardFront = termName;
        QString preamble = variableValue ("term_to_definition_preamble");
        if (!preamble.isEmpty())
            cardFront += preamble;
        
        cardBack = termDefinition;
        
        QString image = variableValue ("image");
        if (!image.isNull())
        {
            if (booleanVariableValue ("term_direct_back_use_image"))
            {
                verify (!"Images support is not ported yet.");
                //exportTo->setColumnValue ("Picture 2", exportTo->getResourceDeckPath (image));
            }
        }
    }
    else if (termExportMode == "inverse")
    {
        cardFront = inverseQuestion;
        QString preamble = variableValue ("definition_to_term_preamble");
        if (!preamble.isEmpty())
            cardFront += preamble;
        
        cardBack = termName;
    }
    else failure ("Invalid term export mode.");
    
    return QPair <QString, QString> (cardFront, cardBack);
}


QString HistoricalTermFlashcard::getFrontSide()
{
    return getBothSides().first;
}

QString HistoricalTermFlashcard::getBackSide()
{
    return getBothSides().second;
}

void HistoryBlockParser::updateMonthNames()
{
    for (unsigned i = 1; i <= 12; i++)
    {
        QString variableName = QString ("month_") + (i < 10 ? "0" : "") + QString::number (i) + "_input_variants";
        QString variants = currentDatabase->variableStack->currentState ().getVariableValue (variableName);
        if (variants.isNull())
            blockParseError (0, "Failed to start parsing block: variants for month " + QString::number (i) + " missing ('" + variableName + "' not defined).");
        
        QStringList monthNames = variants.split (' ', QString::SkipEmptyParts);
        assert (!monthNames.empty(), "Every month must have at least one name: missing month " + QString::number(i + 1));
        
        for (QString name: monthNames)
            monthNameToIndex[name.toLower()] = i;
    }
}

bool HistoryBlockParser::tryExtractSimpleDate (QString& line, SimpleDate& date)
{    
    QStringList monthes = monthNameToIndex.keys();
    QString monthesMatch = "";
    for (QString monthName: monthes)
        monthesMatch = (monthesMatch.isEmpty() ? monthName : monthesMatch + "|" + monthName);
    
    for (int numComponents = 3; numComponents >= 1; numComponents--)
    {
        QRegExp dateRegExp (numComponents == 3 ? "^(\\d+)\\.(\\d+|" + monthesMatch + ")\\.(\\d+)" :
        numComponents == 2 ? "^(\\d+|" + monthesMatch + ")\\.(\\d+)" :
        "^(\\d+)");
        dateRegExp.setCaseSensitivity (Qt::CaseInsensitive);
        dateRegExp.setPatternSyntax (QRegExp::RegExp2);
        
        int matchIndex = dateRegExp.indexIn (line);
        
        if (matchIndex != -1)
        {
            //qstdout << "Matched " << numComponents << " components in string '" << line << "'\n";
            QStringList dateComponents = dateRegExp.capturedTexts();
            assert (dateComponents.size() == 1 + numComponents);
            
            date.day   = (numComponents >= 3) ? dateComponents[dateComponents.size() - 3].toInt() : SIMPLE_DATE_UNSPECIFIED;
            date.year  = (numComponents >= 1) ? dateComponents[dateComponents.size() - 1].toInt() : SIMPLE_DATE_UNSPECIFIED;
            
            if (numComponents >= 2)
            {
                QString monthString = dateComponents[dateComponents.size() - 2].toLower();
                if (monthNameToIndex.find (monthString) != monthNameToIndex.end())
                    date.month = monthNameToIndex[monthString];
                else
                    date.month = monthString.toInt();
            }
            else
            {
                date.month = SIMPLE_DATE_UNSPECIFIED;
            }
            
            line = line.right (line.length() - dateComponents[0].length() - matchIndex);
            
            return true;
        }
    }
    
    date.day = date.month = date.year = SIMPLE_DATE_NOT_PARSED;
    return false;
}

bool HistoryBlockParser::tryExtractDate (QString& line, ComplexDate& date)
{
    updateMonthNames();
    if (!tryExtractSimpleDate (line, date.begin))
        return false;
    
    date.end = SimpleDate();
    
    if (line[0] == '-')
    {
        line = line.right (line.length() - 1);
        if (!tryExtractSimpleDate (line, date.end))
            blockParseError (0, "Interval end date expected after a dash.");
    }
    
    return true;
}

void extractDash (QString firstLine, int& dashPosition, QChar& punctuationMet, bool& multipleDashes)
{
    int numBracketsOpen = 0;
    bool escaped = false;
    
    for (int i = 0; i < (int)firstLine.size(); i++)
    {
        if (firstLine[i] == '(')
            numBracketsOpen++;
        else if (firstLine[i] == ')')
            numBracketsOpen--;
        else if (firstLine[i] == '-' && !escaped)
        {
            if (dashPosition != -1)
                multipleDashes = true;
            else
                dashPosition = i;
        }
        else if (dashPosition == -1 && (firstLine[i] == ',' || firstLine[i] == '.' || firstLine[i] == '?'))
            punctuationMet = firstLine[i];
        
        if (firstLine[i] == '\\')
            escaped = true;
        else
            escaped = false;
    }
}

bool HistoryBlockParser::acceptsBlock (const QVector <QString>& block)
{
    ComplexDate date;
    QString firstLine = block.first();
    if (tryExtractDate (firstLine, date))
        return true;
    
    int dashPosition = -1;
    QChar punctuationMet = 0;
    bool multipleDashes = false;
    extractDash (firstLine, dashPosition, punctuationMet, multipleDashes);
    
    return dashPosition != -1;
}

void HistoryBlockParser::parseBlock (QVector <QString>& block)
{
    // Try to treat as a dated event
    ComplexDate date;
    QString firstLine = block.first();
    //qstdout << firstLine << endl;
    bool extracted = tryExtractDate (firstLine, date);
    
    if (extracted)
    {
        //qstdout << "Got " << date << " and string '" << firstLine << "'" << endl;
        
        firstLine = firstLine.trimmed();
        if (firstLine.isEmpty() || firstLine[0] != '-')
            blockParseError (0, "Expected a dash followed by an event name after a date interval, " +
            (firstLine.isEmpty() ? "end of string" : QString ("'") + firstLine[0] + "'") + " found.");
        
        firstLine = firstLine.right (firstLine.length() - 1).trimmed();
        
        if (firstLine.isEmpty())
            blockParseError (0, "Empty event name.");
        
        firstLine[0] = firstLine[0].toUpper();
        
        if (isEndingSymbol (firstLine[firstLine.length() - 1]) && (firstLine.length() <= 1 || firstLine[firstLine.length() - 2] != '\\'))
            blockParseWarning (0, QString ("An event name ends in unescaped '") + firstLine[firstLine.length() - 1] + "'");
        
        QRegExp unescapedDash ("^-|[^\\\\]-");
        
        QString eventDescription = "";
        for (unsigned i = 0; i < block.size(); i++)
        {
            //if (unescapedDash.indexIn (i == 0 ? firstLine : currentBlock[i]) != -1)
            //  blockParseWarning (i, "Unescaped dash met outside of a date interval/definition construction.");
            
            if (i == 0) continue;
            eventDescription += (i > 1 ? "\n" : "") + block[i];
        }
        
        currentDatabase->entries.push_back (
            shared_ptr <SimpleFlashcard> (new HistoricalEventFlashcard (date, currentEntryTag, currentDatabase->variableStack->currentState(),
                                                                        applyReplacements (replaceEscapes (firstLine)), applyReplacements (replaceEscapes (eventDescription.trimmed())))));
    }
    else
    {
        // Try to treat as a term definition
        
        int dashPosition = -1;
        QChar punctuationMet = 0;
        bool multipleDashes = false;
        
        extractDash (firstLine, dashPosition, punctuationMet, multipleDashes);
        
        verify (dashPosition != -1, "parseBlock method assumes the block is accepted.");
        QString beforeDash = firstLine.left (dashPosition).trimmed();
        if (beforeDash.isEmpty())
            blockParseError (0, "Empty term name.");
        
        QString afterDash = firstLine.right (firstLine.length() - dashPosition - 1).trimmed();
        if (afterDash.isEmpty())
            blockParseError (0, "Empty term definition.");
        
        afterDash[0] = afterDash[0].toUpper();
        
        if (multipleDashes)
            blockParseWarning (0, "Unescaped dash in a term definition.");
        
        if (!punctuationMet.isNull())
            blockParseWarning (0, QString ("Unescaped punctuation symbol '") + punctuationMet + "' met outside of parentheses.");
        
        QChar definitionEnd = afterDash[afterDash.length() - 1];
        if ((afterDash.length() == 1 || afterDash[afterDash.length() - 2] != '\\') &&
            (!isEndingSymbol (definitionEnd) || definitionEnd == '?'))
            blockParseWarning (0, QString ("Term definition ends in unescaped '") + definitionEnd + "'.");
        
        QRegExp unescapedDash ("^-|[^\\\\]-");
        
        QString inverseQuestion = "";
        for (unsigned i = 1; i < block.size(); i++)
        {
            if (unescapedDash.indexIn (block[i]) != -1)
                blockParseWarning (i, "Unescaped dash met in a term inverse question.");
            
            inverseQuestion += (i > 1 ? "\n" : "") + block[i];
        }
        
        currentDatabase->entries.push_back
        (shared_ptr <SimpleFlashcard> (new HistoricalTermFlashcard (currentEntryTag, currentDatabase->variableStack->currentState(),
                                                                    applyReplacements (replaceEscapes (beforeDash)), applyReplacements (replaceEscapes (afterDash)),
                                                                    applyReplacements (replaceEscapes (inverseQuestion.trimmed())))));
    }
}
