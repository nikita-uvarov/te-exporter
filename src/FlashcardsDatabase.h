#ifndef HISTORICAL_DATABASE_H
#define HISTORICAL_DATABASE_H

#include <memory>
#include <QString>
#include <QVector>
#include <QMap>
#include <QStringList>
#include <QFileInfo>

#include "Util.h"
#include "VariableStack.h"

using std::shared_ptr;

class SimpleFlashcard : public VariableStackStateHolder
{
public :
    virtual ~SimpleFlashcard() {}
    
    virtual QString getTag()
    {
        return tag;
    }
    
    virtual QString getFrontSide() = 0;
    virtual QString getBackSide() = 0;
    
    virtual bool thirdSidePresent() { return false; }
    virtual QString getThirdSide() { return nullptr; }
    
    virtual VariableStackState getVariableStackState()
    {
        return variableStack;
    }
    
    virtual void dump (QTextStream& to)
    {
        to << "Flashcard tag: " << tag << ", dump function not overriden." << endl;
    }
    
private :
    QString tag;
    
protected :
    SimpleFlashcard (QString tag, VariableStackState variableStack);
    
    QString getVariableValue (QString variableName);
};

class FlashcardsDatabase
{
public :
	QVector <FileLocationMessage> messages;
	QVector < shared_ptr <SimpleFlashcard> > entries;

	shared_ptr <VariableStack> variableStack;
	QVector < QPair <QString, QString> > replacements;

    FlashcardsDatabase (shared_ptr <VariableStack> variableStack) :
		variableStack (variableStack)
	{}
};

#endif // HISTORICAL_DATABASE_H