
#include "MJRef.h"

#include "MJTable.h"
#include "MJNumber.h"


MJTable* MJRef::createRootTable()
{
    MJTable* root = new MJTable(nullptr);
    srand((unsigned)time(nullptr));
    
    MJFunction* randomFunction = new MJFunction([root](MJTable* args, MJTable* state) {
        if(args->arrayObjects.size() > 0)
        {
            MJRef* arg = args->arrayObjects[0];
            if(arg->type() == MJREF_TYPE_NUMBER)
            {
                const double& numValue = ((MJNumber*)(arg))->value;
                if(numValue == floor(numValue))
                {
                    return new MJNumber(floor(((double)rand() / RAND_MAX) * numValue));
                }
                return new MJNumber(((double)rand() / RAND_MAX) * numValue);
            }
        }
        return new MJNumber(((double)rand() / RAND_MAX));
    }, root);
    
    root->set("random", randomFunction);
    randomFunction->release();
    
    MJFunction* printFunction = new MJFunction([root](MJTable* args, MJTable* state) {
        if(args->arrayObjects.size() > 0)
        {
            std::string printString = "";
            for(MJRef* arg : args->arrayObjects)
            {
                printString += arg->getStringValue();
            }
            MJLog("%s", printString.c_str());
        }
        return nullptr;
    }, root);
    
    root->set("print", printFunction);
    printFunction->release();
    
    
    return root;
}

MJRef* MJRef::load(const std::string& inputString, const std::string& debugName, MJTable* parent) {
    MJDebugInfo debugInfo;
    debugInfo.fileName = debugName;
    const char* cString = inputString.c_str();
    char* endPtr;
    
    return MJRef::load(cString, &endPtr, parent, &debugInfo);
}

MJRef* MJRef::load(const std::string& filename, MJTable* parent) {
    
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    MJDebugInfo debugInfo;
    debugInfo.fileName = filename;
    if(in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        const char* cString = contents.c_str();
        char* endPtr;
        return MJRef::load(cString, &endPtr, parent, &debugInfo);
    }
    else
    {
        MJError("File not found in MJTable::initWithHumanReadableFilePath at:%s", filename.c_str());
    }
    return nullptr;
}

MJRef* MJRef::runScriptFile(const std::string& filename, MJTable* parent)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    MJDebugInfo debugInfo;
    debugInfo.fileName = filename;
    if(in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        const char* cString = contents.c_str();
        char* endPtr;
        MJRef* resultRef = nullptr;
        MJRef* table = MJRef::load(cString, &endPtr, parent, &debugInfo, &resultRef);
        //todo if debug logging
        table->debugLog();
        return resultRef;
    }
    else
    {
        MJError("File not found in MJTable::initWithHumanReadableFilePath at:%s", filename.c_str());
    }
    return nullptr;
    //MJRef** resultRef
}

MJRef* MJRef::load(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo, MJRef** resultRef) {
    
    MJTable* table = new MJTable(parent);
    
    const char* s = skipToNextChar(str, debugInfo);
    
    //uint32_t integerIndex = 0;
    
    bool foundOpeningBracket = false;
    
    if(*s == '{' || *s == '[')
    {
        foundOpeningBracket = true;
        s++;
    }
        
    while(1)
    {
        s = skipToNextChar(s, debugInfo);
        
        if(*s == '\0')
        {
            break;
        }
        
        if(foundOpeningBracket && (*s == '}' || *s == ']'))
        {
            s++;
            *endptr = (char*)s;
            break;
        }
        
        if(!table->addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
        {
            break;
        }
        s = *endptr;
    }
    
    if(!foundOpeningBracket)
    {
        if(table->objectsByStringKey.empty() && table->objectsByNumberKey.empty())
        {
            int arrayObjectCount = (int)table->arrayObjects.size();
            if(arrayObjectCount == 1)
            {
                MJRef* object = table->arrayObjects[0];
                object->retain();
                table->release();
                return object;
            }
            else if(arrayObjectCount == 0)
            {
                table->release();
                return nullptr;
            }
        }
    }
    
    return table;
}

MJRef* loadVariableIfAvailable(MJString* variableName, const char* str, char** endptr, MJTable* parentTable, MJDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        MJRef* newValueRef = parentTable->recursivelyFindVariable(variableName, debugInfo);
        if(newValueRef)
        {
            if(newValueRef->type() == MJREF_TYPE_TABLE)
            {
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
                    return result;
                }
                newValueRef->retain();
                return newValueRef;
            }
            else
            {
                newValueRef = newValueRef->copy();
                return newValueRef;
            }
        }
    }
    if(variableName->isValidFunctionString)
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to call missing function: %s()", variableName->value.c_str());
        const char* s = skipToNextChar(*endptr, debugInfo, true);
        if(*s == '(')
        {
            s++;
            s = skipToNextChar(s, debugInfo);
            if(*s == ')')
            {
                s++;
                s = skipToNextChar(s, debugInfo, true);
            }
        }
        *endptr = (char*)s;
    }
    return nullptr;
}


bool setVariable(MJString* variableName,
                            MJRef* value,
                               MJTable* parentTable,
                               MJDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        return parentTable->recursivelySetVariable(variableName, value, debugInfo);
    }
    return false;
}

MJRef* loadValue(const char* str, char** endptr, MJTable* parentTable, MJDebugInfo* debugInfo, bool allowNonVarStrings)
{
    const char* s = str;
    
    MJRef* valueRef = MJTable::initUnknownTypeRefWithHumanReadableString(s, endptr, parentTable, debugInfo);
    s = skipToNextChar(*endptr, debugInfo, true);
    
    if(valueRef->type() == MJREF_TYPE_STRING)
    {
        MJString* originalString = (MJString*)valueRef;
        MJRef* newValueRef = loadVariableIfAvailable(originalString, s, endptr, parentTable, debugInfo);
        
        s = skipToNextChar(*endptr, debugInfo, true);
        
        if(newValueRef)
        {
            originalString->release();
            valueRef = newValueRef;
        }
        else if(originalString->isValidFunctionString)
        {
            originalString->release();
            *endptr = (char*)s;
            return nullptr;
        }
        else if(!allowNonVarStrings)
        {
            MJSWarn(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Uninitialized variable:%s", originalString->value.c_str());
            originalString->release();
            *endptr = (char*)s;
            return nullptr;
        }
        
    }
    
    *endptr = (char*)s;
    return valueRef;
}

MJRef* recursivelyLoadValue(const char* str,
                            char** endptr,
                            MJRef* leftValue,
                            MJTable* parentTable,
                            MJDebugInfo* debugInfo,
                            bool runLowOperators,
                            bool allowNonVarStrings)
{
    const char* s = str;
    
    if(!leftValue)
    {
        if(*s == '(')
        {
            s++;
            s = skipToNextChar(s, debugInfo, true);
            leftValue = recursivelyLoadValue(s, endptr, nullptr, parentTable, debugInfo, true, allowNonVarStrings);
        }
        else
        {
            leftValue = loadValue(s, endptr, parentTable, debugInfo, allowNonVarStrings);
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
    
    if(secondOperatorChar == '=' && (operatorChar == '=' || operatorChar == '>' || operatorChar == '<') )
    {
        s++;
    }
    
    s = skipToNextChar(s, debugInfo, true);
    
    MJRef* rightValue = recursivelyLoadValue(s, endptr, nullptr, parentTable, debugInfo, false, false);
    s = skipToNextChar(*endptr, debugInfo, true);
    
    if(rightValue->type() == MJREF_TYPE_STRING)
    {
        if(((MJString*)rightValue)->allowAsVariableName)
        {
            MJRef* newValueRef = parentTable->recursivelyFindVariable((MJString*)rightValue, debugInfo);
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
                result = recursivelyLoadValue(s, endptr, result, parentTable, debugInfo, runLowOperators, allowNonVarStrings);
                s = skipToNextChar(*endptr, debugInfo, true);
            }
        }
        
        *endptr = (char*)s;
        
        return result;
    }
    
    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid expression");
    return nullptr;
}
