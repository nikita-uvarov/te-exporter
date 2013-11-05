#ifndef VARIABLE_STACK_H
#define VARIABLE_STACK_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QPair>

class VariableStack;

class VariableStackState
{
public :
	int version;
	VariableStack* stack;

	QString getVariableValue (QString name);
};

class VariableStack
{
	friend class VariableStackState;

public :
	VariableStack() :
		currentVersion (0)
	{}

	// Takes care of variable expansion in value
	void pushVariable (QString name, QString value);

	// Returns false if there were no variable. Empty name means pop last.
	bool popVariable (QString name);
    
    // Replaces $varname with its value
    QString getVariableExpansion (QString value);

	VariableStackState currentState();

	void dump();

private :
	QVector <QString> stackVariables;
	QMap < QString, QVector <QString> > currentStackState;
	QMap < QString, QVector < QPair <int, QString> > > variableNameToVersionPairs;
	int currentVersion;
};

// Provides a nice variable stack interface to children classes
class VariableStackStateHolder 
{
public :
    void setFallbackVariableStackState (VariableStackState fallback);
    void removeFallbackVariableStackState();
    
protected :
    VariableStackStateHolder (VariableStackState state) :
        variableStack (state), fallbackStackPresent (false)
    {}
    
    QString variableValue (QString name);
    bool booleanVariableValue (QString name);
    
    VariableStackState variableStack;
    
    bool fallbackStackPresent;
    VariableStackState fallbackVariableStack;
};

#endif // VARIABLE_STACK_H