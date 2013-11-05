#include "FlashcardsDatabaseParser.h"

const char* DATABASE_INCLUDE_DIRECTORIES_CONTEXT = "database-include-directories";

class DatabaseParserException : public std::exception
{
public :
    DatabaseParserException ()
    {}

    const char* what()
    {
        return "Database parser exception: current block is invalid, see error messages for details";
    }
};

void DatabaseParser::blockParseWarning (int blockLine, QString what)
{
    currentDatabase->messages.push_back (FileLocationMessage (currentFileName, currentBlockFirstLine + blockLine, FileLocationMessageType::WARNING, what));
}

void DatabaseParser::blockParseError (int blockLine, QString what)
{
    currentDatabase->messages.push_back (FileLocationMessage (currentFileName, currentBlockFirstLine + blockLine, FileLocationMessageType::ERROR, what));
    throw DatabaseParserException();
}

void DatabaseParser::registerBlockParser (shared_ptr <BlockParser> parser, QString name)
{
    verify (blockParsers.count (name) == 0, "'" + name + "' block parser already registered.");
    blockParsers[name] = parser;
}

#include "HistoricalFlashcards.h"
#include "QuestionFlashcard.h"

void DatabaseParser::processCurrentBlock()
{
    assert (!currentBlock.isEmpty());
    QString firstLine = currentBlock[0];
    assert (!firstLine.isEmpty());

    // Some constructions can be detected easily
    if (currentDatabase->variableStack->currentState().getVariableValue ("disableTags").isNull() && firstLine[0] == '[')
    {
        if (firstLine[firstLine.size() - 1] != ']')
            blockParseError (0, "Unclosed square braces in a tag entry: '" + firstLine + "'");

        if (currentBlock.size() != 1)
            blockParseError (1, "Multi-line tag entry.");

        QString newTagName = firstLine.mid (1, firstLine.length() - 2);
        if (newTagName.isEmpty())
            blockParseError (0, "Empty tag name.");

        for (QChar c: newTagName)
            if (!(c == '-' || (c.toLower() >= 'a' && c.toLower() <= 'z')))
                blockParseError (0, QString ("Tag name contains illegal character '") + c + "' (only '-' and latin letters are allowed).");

        currentEntryTag = newTagName;
        return;
    }
    
    QString parsersList = currentDatabase->variableStack->currentState().getVariableValue ("parsers");
    QStringList parsers = parsersList.split (",", QString::SkipEmptyParts);
    
    for (QString parserName: parsers)
    {
        verify (blockParsers.count (parserName) != 0, "Parser '" + parserName + "' is not registered.");
        
        shared_ptr <BlockParser> blockParser = blockParsers[parserName];
        verify (blockParser);
        if (!blockParser->acceptsBlock (currentBlock))
            continue;
        
        blockParser->parseBlock (currentBlock);
        return;
    }
    
    blockParseError (0, "No parser ('" + parsersList + "') accepts found block.");
}

void DatabaseParser::processDirective (QString directive, shared_ptr <FlashcardsDatabase> database)
{
    const QString includeDirectivePrefix = "#include ",
                  pushDirectivePrefix = "#push ",
                  popDirectivePrefix = "#pop",
                  imageDirectivePrefix = "#image ",
                  pushReplacementPrefix = "#push_replacement",
                  popReplacementPrefix = "#pop_replacement";

    if (directive.startsWith (includeDirectivePrefix))
    {
        QString includePath = directive.right (directive.length() - includeDirectivePrefix.length());
        shared_ptr <DatabaseParser> subParser (new DatabaseParser);
        subParser->blockParsers = blockParsers;
        QPair <QString, QString> contentsNamePair = FileReaderSingletone::instance().readContents (includePath, DATABASE_INCLUDE_DIRECTORIES_CONTEXT);
        subParser->parseDatabase (contentsNamePair.second, contentsNamePair.first, database, nullptr);
    }
    else if (directive.startsWith (pushDirectivePrefix))
    {
        directive = directive.right (directive.length() - pushDirectivePrefix.length());

        QString name = "", value = "";

        int valueColonIndex = directive.indexOf (':');
        if (valueColonIndex != -1)
        {
            if (directive.length() <= valueColonIndex + 1 || directive[valueColonIndex + 1] != ' ')
                blockParseError (0, "Space expected after a colon in a variable push directive.");

            name = directive.left (valueColonIndex).trimmed();
            value = directive.right (directive.length() - valueColonIndex - 2);
        }
        else
        {
            name = directive.trimmed();
        }

        if (name.isEmpty())
            blockParseError (0, "Variable name is empty.");

        for (QChar c: name)
            if (!c.isLetterOrNumber() && c != '_')
                blockParseError (0, QString ("Invalid character '") + c + "' in variable name '" + name + "'.");

        //qstderr << "Push variable '" << name << "' with value '" << value << "'" << endl;
        currentDatabase->variableStack->pushVariable (name, currentDatabase->variableStack->getVariableExpansion (value));
    }
    else if (directive.startsWith (popDirectivePrefix) && !directive.startsWith (popReplacementPrefix))
    {
        directive = directive.right (directive.length() - popDirectivePrefix.length()).trimmed();

        if (!currentDatabase->variableStack->popVariable (directive))
            blockParseError (0, directive.isEmpty() ? "Failed to pop variable: the stack is empty." : "Failed to pop variable '" + directive + "': not found on stack.");
    }
    else if (directive.startsWith (imageDirectivePrefix))
    {
        directive = directive.right (directive.length() - imageDirectivePrefix.length()).trimmed();
        directive = FileReaderSingletone::instance().getAbsolutePath (directive, DATABASE_INCLUDE_DIRECTORIES_CONTEXT);

        currentDatabase->variableStack->pushVariable ("image", directive);
    }
    else if (directive.startsWith (pushReplacementPrefix))
    {
        directive = directive.right (directive.length() - pushReplacementPrefix.length());

        QChar delimiter = directive.isEmpty() ? QChar ('.') : directive[0];

        if (directive.isEmpty())
            blockParseError (0, "Directive '" + pushReplacementPrefix + "' expects a delimiter character.");
        else
            directive = directive.right(directive.size() - 1);

        QStringList parts = directive.split (delimiter, QString::KeepEmptyParts);

        if (parts.size() != 3)
            blockParseError (0, QString ("Delimiter '") + delimiter + "' breaks '" + directive + "' into " + QString::number (parts.size()) + " parts, 3 expected.");

        if (!parts[2].isEmpty())
            blockParseError (0, "Non-empty last part of a '" + pushReplacementPrefix + "' directive: '" + parts[2] + "'.");
        
        currentDatabase->replacements.push_back (QPair <QString, QString> (parts[0], currentDatabase->variableStack->getVariableExpansion (parts[1])));
    }
    else if (directive.startsWith (popReplacementPrefix))
    {
        if (directive != popReplacementPrefix)
            blockParseError (0, "Directive '" + popReplacementPrefix + "' has no parameters.");

        if (currentDatabase->replacements.empty())
            blockParseError (0, "Replacements stack is empty.");

        currentDatabase->replacements.pop_back();
    }
    else
    {
        blockParseError (0, "Unknown directive: '" + directive + "'.");
    }
}

shared_ptr <FlashcardsDatabase> DatabaseParser::parseDatabase (QString fileName, const QString& fileContents, shared_ptr <FlashcardsDatabase> appendTo, shared_ptr <VariableStack> variableStack)
{
    if (!appendTo)
        appendTo.reset (new FlashcardsDatabase (variableStack));

    currentDatabase = appendTo.get();

    int currentDirectoryPushId = FileReaderSingletone::instance().pushFileSearchPath (QFileInfo (fileName).absolutePath(), DATABASE_INCLUDE_DIRECTORIES_CONTEXT);

    QStringList lines = fileContents.split ('\n');

    currentBlock.clear();
    currentFileName = fileName;
    currentEntryTag = "no-tag";
    
    bool cPlusPlusCommentOpen = false;

    for (int lineNumber = 0; lineNumber < lines.size(); lineNumber++)
    {
        QString& line = lines[lineNumber];

        if (cPlusPlusCommentOpen)
        {
            int commentClosing = line.indexOf ("*/");
            if (commentClosing != -1)
            {
                line = line.right (line.size() - commentClosing - 2);
                cPlusPlusCommentOpen = false;
            }
            else
            {
                line = "";
            }
        }

        bool directive = false;

        if (!line.isEmpty() && line[0] == '#')
        {
            directive = true;
        }
        else
        {
            int lineCommentOpening = line.indexOf ("//");

            if (lineCommentOpening != -1)
                line = line.left (lineCommentOpening);

            int cPlusPlusCommentOpening = line.indexOf ("/*");

            if (cPlusPlusCommentOpening != -1)
            {
                line = line.left (cPlusPlusCommentOpening);
                cPlusPlusCommentOpen = true;
            }

            line = line.trimmed();

            if (!line.isEmpty())
            {
                if (currentBlock.isEmpty())
                    currentBlockFirstLine = lineNumber + 1;

                currentBlock.push_back (line);
            }
        }

        if ((directive || line.isEmpty() || lineNumber + 1 == lines.size()) && !currentBlock.empty())
        {
            try
            {
                processCurrentBlock();
            }
            catch (DatabaseParserException&) {}

            currentBlock.clear();
        }

        if (directive)
        {
            currentBlockFirstLine = lineNumber + 1;
            try
            {
                processDirective (line, appendTo);
            }
            catch (DatabaseParserException&) {}
        }
    }

    FileReaderSingletone::instance().popFileSearchPath (currentDirectoryPushId);
    currentDatabase = nullptr;
    return appendTo;
}

QString DatabaseParser::applyReplacements (QString str)
{
    for (int i = 0; i < currentDatabase->replacements.size(); i++)
        str = str.replace (QRegExp (currentDatabase->replacements[i].first), currentDatabase->replacements[i].second);
    return str;
}

BlockParser::BlockParser (DatabaseParser* databaseParser) :
    databaseParser (databaseParser), currentDatabase (databaseParser->currentDatabase), currentEntryTag (databaseParser->currentEntryTag)
{}

void BlockParser::blockParseError (int blockLine, QString what)
{
    databaseParser->blockParseError (blockLine, what);
}

void BlockParser::blockParseWarning (int blockLine, QString what)
{
    databaseParser->blockParseWarning (blockLine, what);
}

QString BlockParser::applyReplacements (QString str)
{
    return databaseParser->applyReplacements (str);
}


