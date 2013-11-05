#include "FlashcardsDatabase.h"

SimpleFlashcard::SimpleFlashcard (QString tag, VariableStackState variableStack) :
    VariableStackStateHolder (variableStack), tag (tag)
{}

QString SimpleFlashcard::getVariableValue (QString variableName)
{
    return variableStack.getVariableValue (variableName);
}
