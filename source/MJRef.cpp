
#include "MJRef.h"

#include "MJTable.h"
#include "MJNumber.h"

MJRef* loadVariableIfAvailable(MJString* variableName, const char* str, char** endptr, MJTable* parentTable, MJDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        MJRef* newValueRef = parentTable->recursivelyFindVariable(variableName);
        if(newValueRef)
        {
            if(newValueRef->type() == MJREF_TYPE_TABLE)
            {
                delete variableName;
                newValueRef->retain();
                return newValueRef;
            }
            else if(newValueRef->type() == MJREF_TYPE_FUNCTION)
            {
                if(variableName->isValidFunctionString)
                {
                    const char* s = str;
                    MJTable* argsArrayTable = MJTable::initWithHumanReadableString(s, endptr, parentTable, debugInfo);
                    s = skipToNextChar(*endptr, debugInfo, true);
                    
                    MJRef* result = ((MJFunction*)newValueRef)->call(argsArrayTable, parentTable);
                    
                    *endptr = (char*)s;
                    delete variableName;
                    return result;
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid call to function:%s", variableName->value.c_str());
                    delete variableName;
                    return nullptr;
                }
            }
            else
            {
                delete variableName;
                newValueRef = newValueRef->copy();
                return newValueRef;
            }
        }
    }
    return variableName;
}


bool setVariable(MJString* variableName,
                            MJRef* value,
                               MJTable* parentTable,
                               MJDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        return parentTable->recursivelySetVariable(variableName, value);
    }
    return false;
}

MJRef* loadValue(const char* str, char** endptr, MJTable* parentTable, MJDebugInfo* debugInfo)
{
    const char* s = str;
    
    MJRef* valueRef = MJTable::initUnknownTypeRefWithHumanReadableString(s, endptr, parentTable, debugInfo);
    s = skipToNextChar(*endptr, debugInfo, true);
    
    if(valueRef->type() == MJREF_TYPE_STRING)
    {
        valueRef = loadVariableIfAvailable((MJString*)valueRef, s, endptr, parentTable, debugInfo);
        s = skipToNextChar(*endptr, debugInfo, true);
    }
    
    *endptr = (char*)s;
    return valueRef;
}

MJRef* recursivelyLoadValue(const char* str,
                            char** endptr,
                            MJRef* leftValue,
                            MJTable* parentTable,
                            MJDebugInfo* debugInfo,
                            bool runLowOperators)
{
    const char* s = str;
    
    if(!leftValue)
    {
        if(*s == '(')
        {
            s++;
            s = skipToNextChar(s, debugInfo, true);
            leftValue = recursivelyLoadValue(s, endptr, nullptr, parentTable, debugInfo, true);
        }
        else
        {
            leftValue = loadValue(s, endptr, parentTable, debugInfo);
        }
        
        
        s = skipToNextChar(*endptr, debugInfo, true);
    }
    
    char operatorChar = *s;
    char secondOperatorChar = *(s + 1);
    
    if(operatorChar == ')')
    {
        //s++;
        s = skipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        return leftValue;
    }
    
    if(MJExpressionOperatorsSet.count(operatorChar) == 0 || (operatorChar == '=' && secondOperatorChar != '=') || (!runLowOperators && (operatorChar == '+' || operatorChar == '-')))
    {
        s = skipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        return leftValue;
    }
    
    s++;
    s = skipToNextChar(s, debugInfo, true);
    
    MJRef* rightValue = recursivelyLoadValue(s, endptr, nullptr, parentTable, debugInfo, false);
    s = skipToNextChar(*endptr, debugInfo, true);
    
    if(rightValue->type() == MJREF_TYPE_STRING)
    {
        if(((MJString*)rightValue)->allowAsVariableName)
        {
            MJRef* newValueRef = parentTable->recursivelyFindVariable((MJString*)rightValue);
            if(newValueRef)
            {
                delete rightValue;
                rightValue = newValueRef->copy();
            }
            else
            {
                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Running expression with uninitialized variable:%s", ((MJString*)rightValue)->value.c_str());
            }
        }
    }
    
    MJRef* result = nullptr;
    
    if(leftValue->type() == MJREF_TYPE_NUMBER)
    {
        if(rightValue->type() == MJREF_TYPE_NUMBER)
        {
            switch(operatorChar)
            {
                case '+':
                {
                    result = new MJNumber(((MJNumber*)leftValue)->value + ((MJNumber*)rightValue)->value);
                }
                break;
                case '-':
                {
                    result = new MJNumber(((MJNumber*)leftValue)->value - ((MJNumber*)rightValue)->value);
                }
                break;
                case '*':
                {
                    result = new MJNumber(((MJNumber*)leftValue)->value * ((MJNumber*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJNumber(((MJNumber*)leftValue)->value / ((MJNumber*)rightValue)->value);
                }
                break;
                case '>':
                {
                    if(secondOperatorChar == '=')
                    {
                        result = new MJBool(((MJNumber*)leftValue)->value >= ((MJNumber*)rightValue)->value);
                    }
                    else
                    {
                        result = new MJBool(((MJNumber*)leftValue)->value > ((MJNumber*)rightValue)->value);
                    }
                }
                break;
                case '<':
                {
                    if(secondOperatorChar == '=')
                    {
                        result = new MJBool(((MJNumber*)leftValue)->value <= ((MJNumber*)rightValue)->value);
                    }
                    else
                    {
                        result = new MJBool(((MJNumber*)leftValue)->value < ((MJNumber*)rightValue)->value);
                    }
                }
                break;
                case '=':
                {
                    result = new MJBool(((MJNumber*)leftValue)->value == ((MJNumber*)rightValue)->value);
                }
                break;
            }
        }
        else if(rightValue->type() == MJREF_TYPE_VEC2)
        {
            switch(operatorChar)
            {
                case '*':
                {
                    result = new MJVec2(dvec2(((MJNumber*)leftValue)->value) * ((MJVec2*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec2(dvec2(((MJNumber*)leftValue)->value) / ((MJVec2*)rightValue)->value);
                }
                break;
            }
        }
        else if(rightValue->type() == MJREF_TYPE_VEC3)
        {
            switch(operatorChar)
            {
                case '*':
                {
                    result = new MJVec3(dvec3(((MJNumber*)leftValue)->value) * ((MJVec3*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec3(dvec3(((MJNumber*)leftValue)->value) / ((MJVec3*)rightValue)->value);
                }
                break;
            }
        }
        else if(rightValue->type() == MJREF_TYPE_VEC4)
        {
            switch(operatorChar)
            {
                case '*':
                {
                    result = new MJVec4(dvec4(((MJNumber*)leftValue)->value) * ((MJVec4*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec4(dvec4(((MJNumber*)leftValue)->value) / ((MJVec4*)rightValue)->value);
                }
                break;
            }
        }
    }
    else if(leftValue->type() == MJREF_TYPE_VEC2)
    {
        if(rightValue->type() == MJREF_TYPE_NUMBER)
        {
            switch(operatorChar)
            {
                case '*':
                {
                    result = new MJVec2(((MJVec2*)leftValue)->value * ((MJNumber*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec2(((MJVec2*)leftValue)->value / ((MJNumber*)rightValue)->value);
                }
                break;
            }
        }
        else if(rightValue->type() == MJREF_TYPE_VEC2)
        {
            switch(operatorChar)
            {
                case '+':
                {
                    result = new MJVec2(((MJVec2*)leftValue)->value + ((MJVec2*)rightValue)->value);
                }
                break;
                case '-':
                {
                    result = new MJVec2(((MJVec2*)leftValue)->value - ((MJVec2*)rightValue)->value);
                }
                break;
                case '*':
                {
                    result = new MJVec2(((MJVec2*)leftValue)->value * ((MJVec2*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec2(((MJVec2*)leftValue)->value / ((MJVec2*)rightValue)->value);
                }
                break;
            }
        }
    }
    else if(leftValue->type() == MJREF_TYPE_VEC3)
    {
        if(rightValue->type() == MJREF_TYPE_NUMBER)
        {
            switch(operatorChar)
            {
                case '*':
                {
                    result = new MJVec3(((MJVec3*)leftValue)->value * ((MJNumber*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec3(((MJVec3*)leftValue)->value / ((MJNumber*)rightValue)->value);
                }
                break;
            }
        }
        else if(rightValue->type() == MJREF_TYPE_VEC3)
        {
            switch(operatorChar)
            {
                case '+':
                {
                    result = new MJVec3(((MJVec3*)leftValue)->value + ((MJVec3*)rightValue)->value);
                }
                break;
                case '-':
                {
                    result = new MJVec3(((MJVec3*)leftValue)->value - ((MJVec3*)rightValue)->value);
                }
                break;
                case '*':
                {
                    result = new MJVec3(((MJVec3*)leftValue)->value * ((MJVec3*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec3(((MJVec3*)leftValue)->value / ((MJVec3*)rightValue)->value);
                }
                break;
            }
        }
    }
    else if(leftValue->type() == MJREF_TYPE_VEC4)
    {
        if(rightValue->type() == MJREF_TYPE_NUMBER)
        {
            switch(operatorChar)
            {
                case '*':
                {
                    result = new MJVec4(((MJVec4*)leftValue)->value * ((MJNumber*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec4(((MJVec4*)leftValue)->value / ((MJNumber*)rightValue)->value);
                }
                break;
            }
        }
        else if(rightValue->type() == MJREF_TYPE_VEC4)
        {
            switch(operatorChar)
            {
                case '+':
                {
                    result = new MJVec4(((MJVec4*)leftValue)->value + ((MJVec4*)rightValue)->value);
                }
                break;
                case '-':
                {
                    result = new MJVec4(((MJVec4*)leftValue)->value - ((MJVec4*)rightValue)->value);
                }
                break;
                case '*':
                {
                    result = new MJVec4(((MJVec4*)leftValue)->value * ((MJVec4*)rightValue)->value);
                }
                break;
                case '/':
                {
                    result = new MJVec4(((MJVec4*)leftValue)->value / ((MJVec4*)rightValue)->value);
                }
                break;
            }
        }
    }
    else
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid or unassigned value in expression:%s", leftValue->getDebugString().c_str());
    }
    
    if(result)
    {
        delete leftValue;
        delete rightValue;
        
        if(*s == ')')
        {
            s++;
            s = skipToNextChar(s, debugInfo, true);
        }
        else if(MJExpressionOperatorsSet.count(*s) != 0)
        {
            if(runLowOperators || *s == '*' || *s == '/')
            {
                result = recursivelyLoadValue(s, endptr, result, parentTable, debugInfo, runLowOperators);
                s = skipToNextChar(*endptr, debugInfo, true);
            }
        }
        
        *endptr = (char*)s;
        
        return result;
    }
    
    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid expression");
    return nullptr;
}
