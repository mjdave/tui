
#include "TuiRef.h"

#include "TuiTable.h"
#include "TuiNumber.h"

TuiRef* TuiRef::initUnknownTypeRefWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo)
{
    const char* s = tuiSkipToNextChar(str, debugInfo);
    
    if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
    {
        return TuiNumber::initWithHumanReadableString(s, endptr, parent, debugInfo);
    }
    
    TuiFunction* functionRef = TuiFunction::initWithHumanReadableString(str, endptr, parent, debugInfo);
    if(functionRef)
    {
        return functionRef;
    }
    
    TuiBool* boolRef = TuiBool::initWithHumanReadableString(str, endptr, parent, debugInfo);
    if(boolRef)
    {
        return boolRef;
    }
    
    TuiVec2* vec2Ref = TuiVec2::initWithHumanReadableString(str, endptr, parent, debugInfo);
    if(vec2Ref)
    {
        return vec2Ref;
    }
    
    TuiVec3* vec3Ref = TuiVec3::initWithHumanReadableString(str, endptr, parent, debugInfo);
    if(vec3Ref)
    {
        return vec3Ref;
    }
    
    TuiVec4* vec4Ref = TuiVec4::initWithHumanReadableString(str, endptr, parent, debugInfo);
    if(vec4Ref)
    {
        return vec4Ref;
    }
    
    if(*s == 'n'
        && *(s + 1) == 'i'
        && *(s + 2) == 'l')
    {
        s+=3;
        *endptr = (char*)s;
        return new TuiRef(parent);
    }
    else if(*s == 'n'
            && *(s + 1) == 'u'
            && *(s + 2) == 'l'
            && *(s + 3) == 'l')
    {
        s+=4;
        *endptr = (char*)s;
        return new TuiRef(parent);
    }
    else if(*s == '{' || *s == '[')
    {
        return TuiTable::initWithHumanReadableString(s, endptr, parent, debugInfo);
    }
    
    return TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo, Tui_variable_load_type_runExpressions);
}


TuiTable* TuiRef::createRootTable()
{
    TuiTable* rootTable = new TuiTable(nullptr);
    srand((unsigned)time(nullptr));
    
    
    //random(max) provides a floating point value between 0 and max (default 1.0)
    TuiFunction* randomFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            TuiRef* arg = args->arrayObjects[0];
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                return new TuiNumber(((double)rand() / RAND_MAX) * ((TuiNumber*)(arg))->value);
            }
        }
        return new TuiNumber(((double)rand() / RAND_MAX));
    }, rootTable);
    rootTable->set("random", randomFunction);
    randomFunction->release();
    
    
    //randomInt(max) provides an integer from 0 to (max - 1) with a default of 2.
    TuiFunction* randomIntFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            TuiRef* arg = args->arrayObjects[0];
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                double flooredValue = floor(((TuiNumber*)(arg))->value);
                return new TuiNumber(min(flooredValue - 1.0, floor(((double)rand() / RAND_MAX) * flooredValue)));
            }
        }
        return new TuiNumber(min(1.0, floor(((double)rand() / RAND_MAX) * 2)));
    }, rootTable);
    rootTable->set("randomInt", randomIntFunction);
    randomIntFunction->release();
    
    
    // print(msg1, msg2, msg3, ...) print values, args are concatenated together
    TuiFunction* printFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
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
    
    //require(path) loads the given tui file, path is relative to the tui binary (for now)
    TuiFunction* requireFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            return TuiRef::load(Tui::getResourcePath(args->arrayObjects[0]->getStringValue()), state);
        }
        return nullptr;
    }, rootTable);
    rootTable->set("require", requireFunction);
    requireFunction->release();
    
    //readValue() reads input from the command line, serializing just the first value, doesn't (shouldn't!) call functions or load variables
    TuiFunction* readStringFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        std::string stringValue;
        std::getline(std::cin, stringValue);
        
        TuiDebugInfo debugInfo;
        debugInfo.fileName = "input";
        const char* cString = stringValue.c_str();
        char* endPtr;
        
        TuiRef* result = TuiRef::initUnknownTypeRefWithHumanReadableString(cString, &endPtr, state, &debugInfo);
        
        return result;
    }, rootTable);
    
    rootTable->set("readValue", readStringFunction);
    readStringFunction->release();
    
    //clear() clears the console
    TuiFunction* clearFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#if defined _WIN32
        system("cls");
    //clrscr(); // including header file : conio.h
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
        system("clear");
    //std::cout<< u8"\033[2J\033[1;1H"; //Using ANSI Escape Sequences
#elif defined (__APPLE__)
        system("clear");
#endif
        return nullptr;
    }, rootTable);
    
    rootTable->set("clear", clearFunction);
    clearFunction->release();
    
    //type() returns the type name of the given object, eg. 'table', 'string', 'number', 'vec4', 'bool'
    TuiFunction* typeFunction = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            return new TuiString(args->arrayObjects[0]->getTypeName());
        }
        return nullptr;
    }, rootTable);
    rootTable->set("type", typeFunction);
    typeFunction->release();
    
    //************
    //debug
    //************
    TuiTable* debugTable = new TuiTable(rootTable);
    rootTable->set("debug", debugTable);
    debugTable->release();
    
    
    //debug.getLineNumber() returns the line number in the current script file
    TuiFunction* getLineNumber = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiNumber(callingDebugInfo->lineNumber);
    }, debugTable);
    debugTable->set("getLineNumber", getLineNumber);
    getLineNumber->release();
    
    
    //debug.getFileName() returns the current script file name or debug identifier string
    TuiFunction* getFileName = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiString(callingDebugInfo->fileName);
    }, debugTable);
    debugTable->set("getFileName", getFileName);
    getFileName->release();
    
    
    //************
    //table
    //************
    
    TuiTable* tableTable = new TuiTable(rootTable);
    rootTable->set("table", tableTable);
    tableTable->release();
    
    //table.insert(table, index, value) to specify the index or table.insert(table,value) to add to the end
    TuiFunction* tableInsert = new TuiFunction([](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() >= 2)
        {
            TuiRef* tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.insert expected table for first argument");
                return nullptr;
            }
            
            if(args->arrayObjects.size() >= 3)
            {
                TuiRef* indexObject = args->arrayObjects[1];
                if(indexObject->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.insert expected index for second argument. (object to add is third)");
                    return nullptr;
                }
                int addIndex = ((TuiNumber*)indexObject)->value;
                TuiRef* addObject = args->arrayObjects[2];
                
                if(addIndex < ((TuiTable*)tableRef)->arrayObjects.size())
                {
                    addObject->retain();
                    ((TuiTable*)tableRef)->arrayObjects.insert(((TuiTable*)tableRef)->arrayObjects.begin() + addIndex, addObject);
                }
                else if(addObject && addObject->type() != Tui_ref_type_NIL)
                {
                    ((TuiTable*)tableRef)->arrayObjects.resize(addIndex + 1);
                    addObject->retain();
                    ((TuiTable*)tableRef)->arrayObjects[addIndex] = addObject;
                }
                
            }
            else
            {
                TuiRef* addObject = args->arrayObjects[1];
                addObject->retain();
                args->arrayObjects.push_back(addObject);
            }
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.insert expected 2-3 args.");
        }
        return nullptr;
    }, tableTable);
    tableTable->set("insert", tableInsert);
    tableInsert->release();
    
    
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
        TuiError("File not found in TuiRef::load at:%s", filename.c_str());
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
        TuiError("File not found in TuiRef::runScriptFile at:%s", filename.c_str());
    }
    return nullptr;
    //TuiRef** resultRef
}

TuiRef* TuiRef::load(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef) {
    
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

TuiRef* TuiRef::loadVariableIfAvailable(TuiString* variableName, TuiRef* existingValue, const char* str, char** endptr, TuiTable* parentTable, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo)
{
    if(variableName->allowAsVariableName && parentTable)
    {
        TuiRef* newValueRef = parentTable->recursivelyFindVariable(variableName, true, parentTable, tokenMap, locals, debugInfo);
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
                    
                    TuiRef* result = ((TuiFunction*)newValueRef)->call(argsArrayTable, parentTable, existingValue, debugInfo);
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

TuiRef* TuiRef::loadValue(const char* str,
                          char** endptr,
                          TuiRef* existingValue,
                          TuiTable* parentTable,
                          TuiTokenMap* tokenMap,
                          std::map<uint32_t,TuiRef*>* locals,
                          TuiDebugInfo* debugInfo,
                          bool allowNonVarStrings)
{
    const char* s = str;
    
    TuiRef* valueRef = TuiRef::initUnknownTypeRefWithHumanReadableString(s, endptr, parentTable, debugInfo);
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    
    if(valueRef->type() == Tui_ref_type_STRING)
    {
        TuiString* originalString = (TuiString*)valueRef;
        TuiRef* newValueRef = TuiRef::loadVariableIfAvailable(originalString, nullptr, s, endptr, parentTable, tokenMap, locals, debugInfo);
        
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
        else if(!allowNonVarStrings || originalString->allowAsVariableName)
        {
            TuiParseWarn(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Uninitialized variable:%s", originalString->value.c_str());
            originalString->release();
            *endptr = (char*)s;
            return new TuiRef();
        }
        
    }
    
    *endptr = (char*)s;
    return valueRef;
}


TuiBool* TuiRef::logicalNot(TuiRef* value)
{
    return new TuiBool(!value || !value->boolValue());
}

TuiRef* TuiRef::recursivelyLoadValue(const char* str,
                                     char** endptr,
                                     TuiRef* existingValue,
                                     TuiRef* leftValue,
                                     TuiTable* parentTable,
                                     TuiTokenMap* tokenMap,
                                     std::map<uint32_t,TuiRef*>* locals,
                                     TuiDebugInfo* debugInfo,
                                     int operatorLevel,
                                     bool allowNonVarStrings)
{
    const char* s = str;
    
    
    if(!leftValue)
    {
        if(*s == '!')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            TuiRef* rightValue = TuiRef::recursivelyLoadValue(s, endptr, existingValue, nullptr, parentTable, tokenMap, locals, debugInfo, Tui_operator_level_not, allowNonVarStrings);
            if(!rightValue)
            {
                rightValue = existingValue;
            }
            leftValue = TuiRef::logicalNot(rightValue);
            rightValue->release();
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
        }
        else if(*s == '(')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            leftValue = TuiRef::recursivelyLoadValue(s, endptr, existingValue, nullptr, parentTable, tokenMap, locals, debugInfo, Tui_operator_level_default, allowNonVarStrings);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            if(*s != ')')
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected ')'");
                return nullptr;
            }
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
        }
        else
        {
            leftValue = TuiRef::loadValue(s, endptr, existingValue, parentTable, tokenMap, locals, debugInfo, allowNonVarStrings);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
        }
        
        if(!leftValue)
        {
            leftValue = existingValue;
        }
        
        
    }
    
    char operatorChar = *s;
    
    if(operatorChar == '\0' || operatorChar == ')' || operatorChar == ']')
    {
        *endptr = (char*)s;
        if(existingValue)
        {
            return nullptr;
        }
        return leftValue;
    }
    
    char secondOperatorChar = *(s + 1);
    
    bool operatorOr = (operatorChar == 'o' && secondOperatorChar == 'r' && checkSymbolNameComplete(s + 2));
    bool operatorAnd = (operatorChar == 'a' && secondOperatorChar == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
    
    if(operatorOr || operatorAnd)
    {
        if(Tui_operator_level_and_or <= operatorLevel)
        {
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            if(existingValue)
            {
                return nullptr;
            }
            return leftValue;
        }
    }
    else
    {
        if(((TuiExpressionOperatorsSet.count(operatorChar) == 0)) ||
           (operatorChar == '=' && secondOperatorChar != '=') ||
           TuiExpressionOperatorsToLevelMap[operatorChar] <= operatorLevel)
        {
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            if(existingValue)
            {
                return nullptr;
            }
            return leftValue;
        }
    }
    
    s++;
    
    if(secondOperatorChar == '=' && (operatorChar == '!' || operatorChar == '=' || operatorChar == '>' || operatorChar == '<'))
    {
        s++;
    }
    else if(operatorOr)
    {
        s++;
    }
    else if(operatorAnd)
    {
        s+=2;
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
            else
            {
                if(TuiExpressionOperatorsSet.count(*s) != 0)
                {
                    if(TuiExpressionOperatorsToLevelMap[*s] >= operatorLevel)//operatorLevel == 0 || *s == '*' || *s == '/')
                    {
                        result = recursivelyLoadValue(s, endptr, existingValue, result, parentTable, tokenMap, locals, debugInfo, operatorLevel, allowNonVarStrings);
                        s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    }
                }
                else if(Tui_operator_level_and_or >= operatorLevel)
                {
                    bool newOperatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
                    bool newOperatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
                    if(newOperatorOr || newOperatorAnd)
                    {
                        result = recursivelyLoadValue(s, endptr, existingValue, result, parentTable, tokenMap, locals, debugInfo, operatorLevel, allowNonVarStrings);
                        s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    }
                }
            }
            
            *endptr = (char*)s;
            
            return result;
        }
    }
        
    s = tuiSkipToNextChar(s, debugInfo, true);
    
    int newOperatorLevel = TuiExpressionOperatorsToLevelMap[operatorChar];
    
    TuiRef* rightValue = recursivelyLoadValue(s, endptr, nullptr, nullptr, parentTable, tokenMap, locals, debugInfo, newOperatorLevel, true);
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    
    if(rightValue->type() == Tui_ref_type_STRING)
    {
        if(((TuiString*)rightValue)->allowAsVariableName)
        {
            TuiRef* newValueRef = parentTable->recursivelyFindVariable((TuiString*)rightValue, true, parentTable, tokenMap, locals, debugInfo);
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
    else if(operatorChar == '!')
    {
        result = new TuiBool(!leftValue->isEqual(rightValue));
    }
    else if(operatorOr)
    {
        result = new TuiBool(leftValue->boolValue() || rightValue->boolValue());
    }
    else if(operatorAnd)
    {
        result = new TuiBool(leftValue->boolValue() && rightValue->boolValue());
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
            int newOperatorLevel = TuiExpressionOperatorsToLevelMap[*s];
            if(newOperatorLevel >= operatorLevel)
            {
                result = recursivelyLoadValue(s, endptr, existingValue, result, parentTable, tokenMap, locals, debugInfo, operatorLevel, allowNonVarStrings);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
        }
        else if(Tui_operator_level_and_or >= operatorLevel)
        {
            bool newOperatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
            bool newOperatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
            if(newOperatorOr || newOperatorAnd)
            {
                result = recursivelyLoadValue(s, endptr, existingValue, result, parentTable, tokenMap, locals, debugInfo, operatorLevel, allowNonVarStrings);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
        }
        
        *endptr = (char*)s;
        
        return result;
    }
    
    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid expression");
    return nullptr;
}
