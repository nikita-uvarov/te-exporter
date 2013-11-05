#ifndef FLASHCARDS_DATABASE_PARSER_H
#define FLASHCARDS_DATABASE_PARSER_H

#include "FlashcardsDatabase.h"
#include "FlashcardUtilities.h"

extern const char* DATABASE_INCLUDE_DIRECTORIES_CONTEXT;

class DatabaseParser;

class BlockParser
{
protected :
    DatabaseParser* databaseParser;
    // interface
    FlashcardsDatabase*& currentDatabase;
    QString& currentEntryTag;
    
    void blockParseError (int blockLine, QString what);
    void blockParseWarning (int blockLine, QString what);
    
    QString applyReplacements (QString str);
    
public :
    BlockParser (DatabaseParser* databaseParser);
    
    virtual bool acceptsBlock (const QVector <QString>& block) = 0;
    virtual void parseBlock (QVector <QString>& block) = 0;
};

class DatabaseParser
{
    friend class BlockParser;
    
public :
    shared_ptr <FlashcardsDatabase> parseDatabase (QString fileName, const QString& fileContents, shared_ptr <FlashcardsDatabase> appendTo, shared_ptr <VariableStack> variableStack);
    
    template <class T>
    void registerBlockParser (QString name)
    {
        shared_ptr <BlockParser> parser (new T (this));
        registerBlockParser (parser, name);
    }
    
    void registerBlockParser (shared_ptr <BlockParser> parser, QString name);
    
private :    
    FlashcardsDatabase* currentDatabase;
    QVector <QString> currentBlock;
    int currentBlockFirstLine;
    QString currentFileName;
    QString currentEntryTag;
    
    //QMap <int, int> entryIndexToLine;
    
    QMap < QString, shared_ptr <BlockParser> > blockParsers;
    
    void processCurrentBlock();
    
    void blockParseError (int blockLine, QString what);
    void blockParseWarning (int blockLine, QString what);
    
    void processDirective (QString directive, shared_ptr <FlashcardsDatabase> database);
    
    QString applyReplacements (QString str);
};

#endif // FLASHCARDS_DATABASE_PARSER_H
