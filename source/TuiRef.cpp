
#include "TuiRef.h"

#include "TuiTable.h"
#include "TuiNumber.h"


TuiTable* TuiRef::createRootTable()
{
    TuiTable* rootTable = new TuiTable(nullptr);
    srand((unsigned)time(nullptr));
    
    TuiFunction* randomFunction = new TuiFunction([rootTable](TuiTable* args, TuiTable* state) {
        if(args->arrayObjects.size() > 0)
        {
            TuiRef* arg = args->arrayObjects[0];
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                const double& numValue = ((TuiNumber*)(arg))->value;
                if(numValue == floor(numValue))
                {
                    return new TuiNumber(floor(((double)rand() / RAND_MAX) * numValue));
                }
                return new TuiNumber(((double)rand() / RAND_MAX) * numValue);
            }
        }
        return new TuiNumber(((double)rand() / RAND_MAX));
    }, rootTable);
    
    rootTable->set("random", randomFunction);
    randomFunction->release();
    
    TuiFunction* printFunction = new TuiFunction([rootTable](TuiTable* args, TuiTable* state) {
        if(args->arrayObjects.size() > 0)
        {
            std::string printString = "";
            for(TuiRef* arg : args->arrayObjects)
            {
                printString += arg->getDebugStringValue();
            }
            TuiLog("%s", printString.c_str());
        }
        return nullptr;
    }, rootTable);
    
    rootTable->set("print", printFunction);
    printFunction->release();
    
    TuiFunction* requireFunction = new TuiFunction([rootTable](TuiTable* args, TuiTable* state) {
        if(args->arrayObjects.size() > 0)
        {
            return TuiRef::load(Tui::getResourcePath(args->arrayObjects[0]->getStringValue()), state);
        }
        return new TuiRef();
    }, rootTable);
    
    rootTable->set("require", requireFunction);
    requireFunction->release();
    
    
    
    TuiFunction* readStringFunction = new TuiFunction([rootTable](TuiTable* args, TuiTable* state) {
        std::string stringValue;
        std::getline(std::cin, stringValue);
        
        TuiDebugInfo debugInfo;
        debugInfo.fileName = "input";
        const char* cString = stringValue.c_str();
        char* endPtr;
        
        TuiRef* result = TuiTable::initUnknownTypeRefWithHumanReadableString(cString, &endPtr, state, &debugInfo);
        
        return result;
    }, rootTable);
    
    rootTable->set("readValue", readStringFunction);
    readStringFunction->release();
    
    
    return rootTable;
}

TuiRef* TuiRef::load(const std::string& inputString, const std::string& debugName, TuiTable* parent) {
    TuiDebugInfo debugInfo;
    debugInfo.fileName = debugName;
    const char* cString = inputString.c_str();
    char* endPtr;
    
    return TuiRef::load(cString, &endPtr, parent, &debugInfo);
}

TuiRef* TuiRef::load(const std::string& filename, TuiTable* parent) {
    
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    TuiDebugInfo debugInfo;
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
        return TuiRef::load(cString, &endPtr, parent, &debugInfo);
    }
    else
    {
        TuiError("File not found in TuiTable::initWithHumanReadableFilePath at:%s", filename.c_str());
    }
    return nullptr;
}

TuiRef* TuiRef::runScriptFile(const std::string& filename, bool debugLogging, TuiTable* parent)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    TuiDebugInfo debugInfo;
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
        TuiRef* resultRef = nullptr;
        TuiRef* table = TuiRef::load(cString, &endPtr, parent, &debugInfo, &resultRef);
        if(table)
        {
            if(debugLogging)
            {
                table->debugLog();
            }
            table->release();
        }
        return resultRef;
    }
    else
    {
        TuiError("File not found in TuiTable::initWithHumanReadableFilePath at:%s", filename.c_str());
    }
    return nullptr;
    //TuiRef** resultRef
}

TuiRef* TuiRef::load(const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef) {
    
    TuiTable* table = new TuiTable(parent);
    
    const char* s = tuiSkipToNextChar(str, debugInfo);
    
    //uint32_t integerIndex = 0;
    
    bool foundOpeningBracket = false;
    
    if(*s == '{' || *s == '[')
    {
        foundOpeningBracket = true;
        s++;
    }
        
    while(1)
    {
        s = tuiSkipToNextChar(s, debugInfo);
        
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
                TuiRef* object = table->arrayObjects[0];
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

TuiRef* TuiRef::loadVariableIfAvailable(TuiString* variableName, TuiRef* existingValue, const char* str, char** endptr, TuiTable* parentTable, TuiDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        TuiRef* newValueRef = parentTable->recursivelyFindVariable(variableName, debugInfo, true);
        if(newValueRef)
        {
            if(newValueRef->type() == Tui_ref_type_TABLE)
            {
                newValueRef->retain();
                return newValueRef;
            }
            else if(newValueRef->type() == Tui_ref_type_FUNCTION)
            {
                if(variableName->isValidFunctionString)
                {
                    const char* s = str;
                    TuiTable* argsArrayTable = TuiTable::initWithHumanReadableString(s, endptr, parentTable, debugInfo);
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    
                    TuiRef* result = ((TuiFunction*)newValueRef)->call(argsArrayTable, parentTable, existingValue);
                    *endptr = (char*)s;
                    
                    return result;
                }
                newValueRef->retain();
                return newValueRef;
            }
            else
            {
                //newValueRef = newValueRef->copy();
                newValueRef->retain();
                return newValueRef;
            }
        }
    }
    if(variableName->isValidFunctionString)
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to call missing function: %s()", variableName->value.c_str());
        const char* s = tuiSkipToNextChar(*endptr, debugInfo, true);
        if(*s == '(')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            if(*s == ')')
            {
                s++;
                s = tuiSkipToNextChar(s, debugInfo, true);
            }
        }
        *endptr = (char*)s;
    }
    return nullptr;
}


bool TuiRef::setVariable(TuiString* variableName,
                            TuiRef* value,
                               TuiTable* parentTable,
                               TuiDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        return parentTable->recursivelySetVariable(variableName, value, debugInfo);
    }
    return false;
}

TuiRef* loadValue(const char* str, char** endptr, TuiRef* existingValue, TuiTable* parentTable, TuiDebugInfo* debugInfo, bool allowNonVarStrings)
{
    const char* s = str;
    
    TuiRef* valueRef = TuiTable::initUnknownTypeRefWithHumanReadableString(s, endptr, parentTable, debugInfo);
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    
    if(valueRef->type() == Tui_ref_type_STRING)
    {
        TuiString* originalString = (TuiString*)valueRef;
        TuiRef* newValueRef = TuiRef::loadVariableIfAvailable(originalString, nullptr, s, endptr, parentTable, debugInfo);
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
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
            TuiParseWarn(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Uninitialized variable:%s", originalString->value.c_str());
            originalString->release();
            *endptr = (char*)s;
            return nullptr;
        }
        
    }
    
    *endptr = (char*)s;
    return valueRef;
}

TuiRef* TuiRef::recursivelyLoadValue(const char* str,
                            char** endptr,
                            TuiRef* existingValue,
                            TuiRef* leftValue,
                            TuiTable* parentTable,
                            TuiDebugInfo* debugInfo,
                            bool runLowOperators,
                            bool allowNonVarStrings)
{
    const char* s = str;
    
    if(!leftValue)
    {
        if(*s == '(')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            leftValue = TuiRef::recursivelyLoadValue(s, endptr, existingValue, nullptr, parentTable, debugInfo, true, allowNonVarStrings);
        }
        else
        {
            leftValue = loadValue(s, endptr, existingValue, parentTable, debugInfo, allowNonVarStrings);
        }
        
        if(!leftValue)
        {
            leftValue = existingValue;
        }
        
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
    }
    
    char operatorChar = *s;
    
    if(operatorChar == '\0')
    {
        *endptr = (char*)s;
        if(existingValue)
        {
            return nullptr;
        }
        return leftValue;
    }
    
    
    if(operatorChar == ')')
    {
        //s++;
        s = tuiSkipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        if(existingValue)
        {
            return nullptr;
        }
        return leftValue;
    }
    
    char secondOperatorChar = *(s + 1);
    
    
    if(TuiExpressionOperatorsSet.count(operatorChar) == 0 || (operatorChar == '=' && secondOperatorChar != '=') || (!runLowOperators && (operatorChar == '+' || operatorChar == '-')))
    {
        s = tuiSkipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        if(existingValue)
        {
            return nullptr;
        }
        return leftValue;
    }
    
    s++;
    
    if(secondOperatorChar == '=' && (operatorChar == '=' || operatorChar == '>' || operatorChar == '<') )
    {
        s++;
    }
    
    TuiRef* result = nullptr;
    
    if((operatorChar == '+' && (secondOperatorChar == '+' || secondOperatorChar == '=')) ||
       (operatorChar == '-' && (secondOperatorChar == '-' || secondOperatorChar == '=')) ||
       (operatorChar == '*' && secondOperatorChar == '=') ||
       (operatorChar == '/' && secondOperatorChar == '='))
    {
        s++;
        
        if(secondOperatorChar != '=') //x++/--. watch out, this could break if more operators added ^^^^
        {
            if(leftValue->type() == Tui_ref_type_NUMBER)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        ((TuiNumber*)leftValue)->value++;
                        result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array
                    }
                        break;
                    case '-':
                    {
                        ((TuiNumber*)leftValue)->value--;
                        result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid or unassigned value in expression:%s", leftValue->getDebugString().c_str());
            }
            
            
            leftValue->release();
            
            if(*s == ')')
            {
                s++;
                s = tuiSkipToNextChar(s, debugInfo, true);
            }
            else if(TuiExpressionOperatorsSet.count(*s) != 0)
            {
                if(runLowOperators || *s == '*' || *s == '/')
                {
                    result = recursivelyLoadValue(s, endptr, existingValue, result, parentTable, debugInfo, runLowOperators, allowNonVarStrings);
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                }
            }
            
            *endptr = (char*)s;
            
            return result;
        }
    }
        
    s = tuiSkipToNextChar(s, debugInfo, true);
    
    TuiRef* rightValue = recursivelyLoadValue(s, endptr, nullptr, nullptr, parentTable, debugInfo, false, true);
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    
    if(rightValue->type() == Tui_ref_type_STRING)
    {
        if(((TuiString*)rightValue)->allowAsVariableName)
        {
            TuiRef* newValueRef = parentTable->recursivelyFindVariable((TuiString*)rightValue, debugInfo, true);
            if(newValueRef)
            {
                delete rightValue;
                rightValue = newValueRef->copy();
            }
            else
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Running expression with uninitialized variable:%s", ((TuiString*)rightValue)->value.c_str());
            }
        }
    }
    
    
    if(operatorChar == '=')
    {
        result = new TuiBool(leftValue->isEqual(rightValue));
    }
    else
    {
        
        if(leftValue->type() == Tui_ref_type_NUMBER)
        {
            if(rightValue->type() == Tui_ref_type_NUMBER)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        if(secondOperatorChar == '=')
                        {
                            ((TuiNumber*)leftValue)->value += ((TuiNumber*)rightValue)->value;
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array
                        }
                        else
                        {
                            result = new TuiNumber(((TuiNumber*)leftValue)->value + ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '-':
                    {
                        if(secondOperatorChar == '=')
                        {
                            ((TuiNumber*)leftValue)->value -= ((TuiNumber*)rightValue)->value;
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array
                        }
                        else
                        {
                            result = new TuiNumber(((TuiNumber*)leftValue)->value - ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '*':
                    {
                        if(secondOperatorChar == '=')
                        {
                            ((TuiNumber*)leftValue)->value *= ((TuiNumber*)rightValue)->value;
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array
                        }
                        else
                        {
                            result = new TuiNumber(((TuiNumber*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(secondOperatorChar == '=')
                        {
                            ((TuiNumber*)leftValue)->value /= ((TuiNumber*)rightValue)->value;
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array
                        }
                        else
                        {
                            result = new TuiNumber(((TuiNumber*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '>':
                    {
                        if(secondOperatorChar == '=')
                        {
                            result = new TuiBool(((TuiNumber*)leftValue)->value >= ((TuiNumber*)rightValue)->value);
                        }
                        else
                        {
                            result = new TuiBool(((TuiNumber*)leftValue)->value > ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '<':
                    {
                        if(secondOperatorChar == '=')
                        {
                            result = new TuiBool(((TuiNumber*)leftValue)->value <= ((TuiNumber*)rightValue)->value);
                        }
                        else
                        {
                            result = new TuiBool(((TuiNumber*)leftValue)->value < ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_VEC2)
            {
                switch(operatorChar)
                {
                    case '*':
                    {
                        result = new TuiVec2(dvec2(((TuiNumber*)leftValue)->value) * ((TuiVec2*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec2(dvec2(((TuiNumber*)leftValue)->value) / ((TuiVec2*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_VEC3)
            {
                switch(operatorChar)
                {
                    case '*':
                    {
                        result = new TuiVec3(dvec3(((TuiNumber*)leftValue)->value) * ((TuiVec3*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec3(dvec3(((TuiNumber*)leftValue)->value) / ((TuiVec3*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_VEC4)
            {
                switch(operatorChar)
                {
                    case '*':
                    {
                        result = new TuiVec4(dvec4(((TuiNumber*)leftValue)->value) * ((TuiVec4*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec4(dvec4(((TuiNumber*)leftValue)->value) / ((TuiVec4*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
        }
        else if(leftValue->type() == Tui_ref_type_VEC2)
        {
            if(rightValue->type() == Tui_ref_type_NUMBER)
            {
                switch(operatorChar)
                {
                    case '*':
                    {
                        result = new TuiVec2(((TuiVec2*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec2(((TuiVec2*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_VEC2)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        result = new TuiVec2(((TuiVec2*)leftValue)->value + ((TuiVec2*)rightValue)->value);
                    }
                        break;
                    case '-':
                    {
                        result = new TuiVec2(((TuiVec2*)leftValue)->value - ((TuiVec2*)rightValue)->value);
                    }
                        break;
                    case '*':
                    {
                        result = new TuiVec2(((TuiVec2*)leftValue)->value * ((TuiVec2*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec2(((TuiVec2*)leftValue)->value / ((TuiVec2*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
        }
        else if(leftValue->type() == Tui_ref_type_VEC3)
        {
            if(rightValue->type() == Tui_ref_type_NUMBER)
            {
                switch(operatorChar)
                {
                    case '*':
                    {
                        result = new TuiVec3(((TuiVec3*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec3(((TuiVec3*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_VEC3)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        result = new TuiVec3(((TuiVec3*)leftValue)->value + ((TuiVec3*)rightValue)->value);
                    }
                        break;
                    case '-':
                    {
                        result = new TuiVec3(((TuiVec3*)leftValue)->value - ((TuiVec3*)rightValue)->value);
                    }
                        break;
                    case '*':
                    {
                        result = new TuiVec3(((TuiVec3*)leftValue)->value * ((TuiVec3*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec3(((TuiVec3*)leftValue)->value / ((TuiVec3*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
        }
        else if(leftValue->type() == Tui_ref_type_VEC4)
        {
            if(rightValue->type() == Tui_ref_type_NUMBER)
            {
                switch(operatorChar)
                {
                    case '*':
                    {
                        result = new TuiVec4(((TuiVec4*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec4(((TuiVec4*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_VEC4)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        result = new TuiVec4(((TuiVec4*)leftValue)->value + ((TuiVec4*)rightValue)->value);
                    }
                        break;
                    case '-':
                    {
                        result = new TuiVec4(((TuiVec4*)leftValue)->value - ((TuiVec4*)rightValue)->value);
                    }
                        break;
                    case '*':
                    {
                        result = new TuiVec4(((TuiVec4*)leftValue)->value * ((TuiVec4*)rightValue)->value);
                    }
                        break;
                    case '/':
                    {
                        result = new TuiVec4(((TuiVec4*)leftValue)->value / ((TuiVec4*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
        }
        else if(leftValue->type() == Tui_ref_type_STRING)
        {
            if(rightValue->type() == Tui_ref_type_STRING)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        result = new TuiString(((TuiString*)leftValue)->value + ((TuiString*)rightValue)->value);
                    }
                        break;
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
        }
        else
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid or unassigned value in expression:%s", leftValue->getDebugString().c_str());
        }
    }
    
    if(result)
    {
        leftValue->release();
        rightValue->release();
        
        if(TuiExpressionOperatorsSet.count(*s) != 0)
        {
            if(runLowOperators || *s == '*' || *s == '/')
            {
                result = recursivelyLoadValue(s, endptr, existingValue, result, parentTable, debugInfo, runLowOperators, allowNonVarStrings);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
        }
        
        *endptr = (char*)s;
        
        return result;
    }
    
    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid expression");
    return nullptr;
}
