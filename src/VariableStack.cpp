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
		for (QVector < QPair <int, QString> >& v: variableNameToVersionPairs)
			if (!v.empty() && v.back().first == currentVersion)
			{
				QVector <QString>& valueStack = currentStackState[v.back().second];
				assert (!valueStack.empty());
				valueStack.pop_back();

				v.push_back (QPair <int, QString> (++currentVersion, valueStack.isEmpty() ? QString::null : valueStack.back()));
				return true;
			}

		return false;
	}
	else
	{
		if (variableNameToVersionPairs.count (name) == 0)
			return false;

		QVector <QString>& valueStack = currentStackState[name];
		if (valueStack.empty())
			return false;

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

			if (name.isEmpty())
				failure ("Invalid value '" + original + "' for variable '" + subName + "': unescaped dollar sign found.");

			QString subWith = currentState().getVariableValue (name);
			if (subWith.isNull()) subWith = "";

			value = value.left (trimFrom) + subWith + (i < value.length() ? value.right (value.length() - i - 1) : "");
			i = trimFrom + subWith.length() - 1;
		}
	}

	//qstderr << "Set variable '" + name + "' to '" + value + "'" << endl;

	currentStackState[name].push_back (value);
	variableNameToVersionPairs[name].push_back (QPair <int, QString> (++currentVersion, value));
}
