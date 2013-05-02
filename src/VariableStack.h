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

	VariableStackState currentState();

private :
	QMap < QString, QVector <QString> > currentStackState;
	QMap < QString, QVector < QPair <int, QString> > > variableNameToVersionPairs;
	int currentVersion;
};

#endif // VARIABLE_STACK_H