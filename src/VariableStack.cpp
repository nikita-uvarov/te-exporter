#include "VariableStack.h"
#include "Util.h"

VariableStackState VariableStack::currentState()
{
	VariableStackState current = { currentVersion, this };
	return current;
}

QString VariableStackState::getVariableValue (QString name)
{
	QMap < QString, QVector < QPair <int, QString> > >& variableNameToVersionPairs = stack->variableNameToVersionPairs;
	if (variableNameToVersionPairs.count (name) == 0)
		return QString::null;

	QVector < QPair <int, QString> >& versionPairs = variableNameToVersionPairs[name];

	// Search for last entry with version <= our

	int min = 0, max = versionPairs.size() - 1;
	while (min < max)
	{
		int middle = (min + max + 1) / 2;

		if (versionPairs[middle].first > version)
			max = middle - 1;
		else
			min = middle;
	}

	if (versionPairs.empty() || versionPairs[min].first > version || versionPairs[min].second.isNull())
		return QString::null;

	return versionPairs[min].second;
}

bool VariableStack::popVariable (QString name)
{
	if (name == "")
	{
		if (stackVariables.empty())
			return false;

		QString last = stackVariables.back();
		stackVariables.pop_back();

		QVector <QString>& valueStack = currentStackState[last];
		assert (!valueStack.empty());
		valueStack.pop_back();

		variableNameToVersionPairs[last].push_back (QPair <int, QString> (++currentVersion, valueStack.isEmpty() ? QString::null : valueStack.back()));

		return true;
	}
	else
	{
		if (variableNameToVersionPairs.count (name) == 0)
			return false;

		QVector <QString>& valueStack = currentStackState[name];
		if (valueStack.empty())
			return false;

		bool erased = false;
		for (int i = stackVariables.size() - 1; i >= 0; i--)
			if (stackVariables[i] == name)
			{
				stackVariables.erase (stackVariables.begin() + i);
				erased = true;
				break;
			}

		assert (erased);

		valueStack.pop_back();
		variableNameToVersionPairs[name].push_back (QPair <int, QString> (++currentVersion, valueStack.isEmpty() ? QString::null : valueStack.back()));
		return true;
	}
}

void VariableStack::pushVariable (QString name, QString value)
{
	QString original = value;
	// Bash-like substitute variables in value string

	for (int i = 0; i < value.length(); i++)
	{
		if (value[i] == '\\')
		{
			value = value.left (i) + value.right (value.length() - i - 1);
			continue;
		}

		if (value[i] == '$')
		{
			int trimFrom = i;
			i++;
			QString subName = "";

			while (i < value.length() && value[i] != ' ')
				subName += value[i++];

			if (subName.isEmpty())
				failure ("Invalid value '" + original + "' for variable '" + subName + "': unescaped dollar sign found.");

			QString subWith = currentState().getVariableValue (subName);
			if (subWith.isNull()) subWith = "";

			value = value.left (trimFrom) + subWith + (i < value.length() ? value.right (value.length() - i - 1) : "");
			i = trimFrom + subWith.length() - 1;
		}
	}

	//qstderr << "Set variable '" + name + "' to '" + value + "'" << endl;

	stackVariables.push_back (name);
	currentStackState[name].push_back (value);
	variableNameToVersionPairs[name].push_back (QPair <int, QString> (++currentVersion, value));
}

void VariableStack::dump()
{
	for (auto it = variableNameToVersionPairs.begin(); it != variableNameToVersionPairs.end(); it++)
	{
		qstderr << "Variable '" << it.key() << "':" << endl;
		for (auto pair = it.value().begin(); pair != it.value().end(); pair++)
			qstderr << "Version " << pair->first << " value '" << pair->second << "'." << endl;
		qstderr << endl;
	}
}

