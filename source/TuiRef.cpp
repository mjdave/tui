
#include "TuiRef.h"

#include "TuiTable.h"
#include "TuiNumber.h"

TuiTable* TuiRef::createRootTable()
{
    TuiTable* rootTable = new TuiTable(nullptr);
    srand((unsigned)time(nullptr));
    
    //random(max) provides a floating point value between 0 and max (default 1.0)
    rootTable->setFunction("random", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            TuiRef* arg = args->arrayObjects[0];
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                return new TuiNumber(((double)rand() / RAND_MAX) * ((TuiNumber*)(arg))->value);
            }
        }
        return new TuiNumber(((double)rand() / RAND_MAX));
    });
    
    
    //randomInt(max) provides an integer from 0 to (max - 1) with a default of 2.
    rootTable->setFunction("randomInt", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
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
    });
    
    
    // print(msg1, msg2, msg3, ...) print values, args are concatenated together
    rootTable->setFunction("print", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
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
    });
    
    //require(path) loads the given tui file, path is relative to the tui binary (for now)
    rootTable->setFunction("require", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            return TuiRef::load(Tui::getResourcePath(args->arrayObjects[0]->getStringValue()), state);
        }
        return nullptr;
    });
    
    //readValue() reads input from the command line, serializing just the first value, will call functions and load variables
    rootTable->setFunction("readValue",
                           [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        std::string stringValue;
        std::getline(std::cin, stringValue);
        
        TuiDebugInfo debugInfo;
        debugInfo.fileName = "input";
        const char* cString = stringValue.c_str();
        char* endPtr;
        
        TuiRef* enclosingRef = nullptr;
        std::string finalKey = "";
        int finalIndex = -1;
        
        TuiRef* result = TuiRef::loadValue(cString,
                                           &endPtr,
                                           nullptr,
                                           state,
                                           callingDebugInfo,
                                           false,
                                           &enclosingRef,
                                           &finalKey,
                                           &finalIndex);
        if(!result && !finalKey.empty())
        {
            result = new TuiString(finalKey);
        }
        
        return result;
    });
    
    //clear() clears the console
    rootTable->setFunction("clear", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
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
    });
    
    //type() returns the type name of the given object, eg. 'table', 'string', 'number', 'vec4', 'bool'
    rootTable->setFunction("type", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args->arrayObjects.size() > 0)
        {
            return new TuiString(args->arrayObjects[0]->getTypeName());
        }
        return nullptr;
    });
    
    //************
    //debug
    //************
    TuiTable* debugTable = new TuiTable(rootTable);
    rootTable->set("debug", debugTable);
    debugTable->release();
    
    
    //debug.getLineNumber() returns the line number in the current script file
    debugTable->setFunction("getLineNumber", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiNumber(callingDebugInfo->lineNumber);
    });
    
    //debug.getFileName() returns the current script file name or debug identifier string
    debugTable->setFunction("getFileName", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiString(callingDebugInfo->fileName);
    });
    
    //************
    //table
    //************
    
    TuiTable* tableTable = new TuiTable(rootTable);
    rootTable->set("table", tableTable);
    tableTable->release();
    
    //table.insert(table, index, value) to specify the index or table.insert(table,value) to add to the end
    tableTable->setFunction("insert", [](TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
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
                
                ((TuiTable*)tableRef)->insert(addIndex, addObject);
                
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
    });
    
    
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

TuiRef* TuiRef::runScriptFile(const std::string& filename, TuiTable* parent)
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
        TuiRef::load(cString, &endPtr, parent, &debugInfo, &resultRef);
        return resultRef;
    }
    else
    {
        TuiError("File not found in TuiRef::runScriptFile at:%s", filename.c_str());
    }
    return nullptr;
}

TuiRef* TuiRef::load(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef) {
    
    TuiTable* table = new TuiTable(parent);
    
    const char* s = tuiSkipToNextChar(str, debugInfo);
    
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

// parses a single token and returns the result eg: foo, bar(), array, [1+2], x, 42
static TuiRef* loadSingleValueInternal(const char* str,
                                       char** endptr,
                                       TuiRef* existingValue,
                                       TuiRef* parentRef,
                                       TuiRef* varChainParent,
                                       TuiDebugInfo* debugInfo,
                                       bool allowQuotedStringsAsVariableNames,
                                       
                                       std::string* onSetKey, //watch out, using the existance of this var to allow "x":5 json quoted key names only. For values, a quoted string is not a valid variable name
                                       int* onSetIndex) //index todo
{
    const char* s = str;
    
    TuiTable* parent = (TuiTable*)parentRef;
    if(parentRef->type() != Tui_ref_type_TABLE)
    {
        TuiError("Unimplemented");
    }
    
    if(!varChainParent)
    {
        if(*s == '(')
        {
            TuiRef* result = TuiRef::loadExpression(s,
                                                    endptr,
                                                    existingValue,
                                                 nullptr,
                                                    parent,
                                                 debugInfo);
            return result;
        }
        if(*s == '!')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            TuiRef* result = loadSingleValueInternal(s,
                                                     endptr,
                                                     nullptr,
                                                     parent,
                                                     nullptr,
                                                     debugInfo,
                                                     allowQuotedStringsAsVariableNames,
                                                     nullptr,
                                                     nullptr);
            if(result)
            {
                bool newValue = !result->boolValue();
                result->release();
                if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
                {
                    ((TuiBool*)existingValue)->value = newValue;
                    return nullptr;
                }
                
                return new TuiBool(newValue);
            }
            return new TuiBool(true);
        }
        
        if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            double value = strtod(s, endptr);
            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
            {
                ((TuiNumber*)existingValue)->value = value;
                return nullptr;
            }
            TuiNumber* number = new TuiNumber(value, parent);
            return number;
        }
        
        //todo pull out these functions, set existing values correctly
        TuiFunction* functionRef = TuiFunction::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(functionRef)
        {
            return functionRef;
        }
        
        TuiBool* boolRef = TuiBool::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(boolRef)
        {
            return boolRef;
        }
        
        TuiVec2* vec2Ref = TuiVec2::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(vec2Ref)
        {
            return vec2Ref;
        }
        
        TuiVec3* vec3Ref = TuiVec3::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(vec3Ref)
        {
            return vec3Ref;
        }
        
        TuiVec4* vec4Ref = TuiVec4::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(vec4Ref)
        {
            return vec4Ref;
        }
        
        if(*s == 'n'
           && *(s + 1) == 'i'
           && *(s + 2) == 'l')
        {
            s+=3;
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return new TuiRef(parent);
        }
        else if(*s == 'n'
                && *(s + 1) == 'u'
                && *(s + 2) == 'l'
                && *(s + 3) == 'l')
        {
            s+=4;
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            return new TuiRef(parent);
        }
        else if(*s == '{' || *s == '[') //watch out [ might be a json array, or accessing array element
        {
            return TuiRef::load(s, endptr, parent, debugInfo);
        }
    }
    else // varChainParent != nullptr
    {
        if(*s == '[')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            TuiRef* index = TuiRef::loadExpression(s,
                                                   endptr,
                                                   nullptr,
                                                   nullptr,
                                                   parent,
                                                   debugInfo);
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            s++; //']'
            *endptr = (char*)s;
            
            if(!index || index->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid table index:%s", (index ? index->getDebugString().c_str() : "nil"));
                return nullptr;
            }
            
            if(varChainParent->type() != Tui_ref_type_TABLE)
            {
                TuiError("Unimplemented");
            }
            
            int indexValue = ((TuiNumber*)index)->value;
            index->release();
            
            if(onSetIndex)
            {
                *onSetIndex = indexValue;
            }
            
            if(indexValue < 0 || indexValue >= ((TuiTable*)varChainParent)->arrayObjects.size())
            {
                if(!onSetIndex)
                {
                    TuiParseWarn(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Table index:%d beyond bounds:%d", indexValue, (int)((TuiTable*)varChainParent)->arrayObjects.size());
                }
                return new TuiRef(parent);
            }
            
            TuiRef* result = ((TuiTable*)varChainParent)->arrayObjects[indexValue];
            if(result)
            {
                result->retain();
            }
            return result;
        }
    }
    
    std::string stringBuffer;
    
    bool singleQuote = false;
    bool doubleQuote = false;
    bool escaped = false;
    bool foundSpace = false;
    
    bool allowAsVariableName = true;
    bool isFunctionCall = false;
    
    
    for(;; s++)
    {
        if(*s == '\0')
        {
            break;
        }
        else if(*s == '\'')
        {
            if(!escaped && !doubleQuote)
            {
                if(singleQuote)
                {
                    singleQuote = false;
                    s++;
                    break;
                }
                else
                {
                    if(stringBuffer.empty())
                    {
                        singleQuote = true;
                        if(!allowQuotedStringsAsVariableNames)
                        {
                            allowAsVariableName = false;
                        }
                    }
                }
            }
            else
            {
                stringBuffer += *s;
            }
        }
        else if(*s == '"')
        {
            if(!escaped && !singleQuote)
            {
                if(doubleQuote)
                {
                    doubleQuote = false;
                    s++;
                    break;
                }
                else
                {
                    if(stringBuffer.empty())
                    {
                        doubleQuote = true;
                        if(!allowQuotedStringsAsVariableNames)
                        {
                            allowAsVariableName = false;
                        }
                    }
                }
            }
            else
            {
                stringBuffer += *s;
            }
        }
        else if(*s == '\\')
        {
            if(!escaped)
            {
                escaped = true;
            }
            else
            {
                escaped = false;
                stringBuffer += *s;
            }
        }
        else if(escaped || singleQuote || doubleQuote)
        {
            stringBuffer += *s;
            escaped = false;
        }
        else if(*s == '.' && allowAsVariableName)
        {
            if(*(s+1) == '.') // .. syntax eg. ..foo.x //todo
            {
                TuiError("Unimplemented");
                TuiRef* result = parent;
                while(*(s + 1) == '.')
                {
                    s++;
                    if(!result->parent || result->parent->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented"); //this might be a userdata's member table or a custom type or something?
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "No parent found for '..' variable");
                        return nullptr;
                    }
                    
                    result = result->parent;
                }
                
                s = tuiSkipToNextChar(s, debugInfo, true);
                *endptr = (char*)s;
                
                result->retain();
                
                return result;
            }
            else
            {
                break;
            }
        }
        else if(*s == '(' && allowAsVariableName && !stringBuffer.empty())
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            isFunctionCall = true;
            break;
        }
        else if(*s == '[' && allowAsVariableName) //not sure about this one
        {
            break;
        }
        else if(isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || *s == ']' || TuiExpressionOperatorsSet.count(*s) != 0)
        {
            if(*s == '\n')
            {
                break;
            }
            if(!isspace(*s))
            {
                break;
            }
            else
            {
                foundSpace = true;
            }
        }
        else if(s == str && //start of string
                ((*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2)) ||
                 (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3))))
        {
            break;
        }
        else
        {
            if(foundSpace)
            {
                break;
            }
            stringBuffer += *s;
            escaped = false;
        }
        
    }
    
    s = tuiSkipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
    
    
    if(allowAsVariableName)
    {
        if(onSetKey)
        {
            *onSetKey = stringBuffer;
        }
        
        TuiRef* resultRef = nullptr;
        if(varChainParent && varChainParent->type() != Tui_ref_type_TABLE)
        {
            TuiError("Unimplemented");
        }
        TuiTable* searchTable = (varChainParent ? (TuiTable*)varChainParent :  parent); //todo if this is not a table eg userdata
        
        if(searchTable->objectsByStringKey.count(stringBuffer) != 0)
        {
            resultRef = searchTable->objectsByStringKey[stringBuffer];
        }
        
        while(!resultRef && searchTable->parent)
        {
            searchTable = searchTable->parent;
            if(searchTable->objectsByStringKey.count(stringBuffer) != 0)
            {
                resultRef = searchTable->objectsByStringKey[stringBuffer];
            }
        }
        
        if(resultRef)
        {
            if(isFunctionCall)
            {
                TuiTable* argsArrayTable = new TuiTable(parent);//TuiTable::initWithHumanReadableString(s, endptr, parent, debugInfo);
                
                while(*s != ')')
                {
                    TuiRef* valueRef = TuiRef::loadExpression(s,
                                                              endptr,
                                                              nullptr,
                                                              nullptr,
                                                              argsArrayTable,
                                                              debugInfo);
                    if(!valueRef)
                    {
                        valueRef = new TuiRef();
                    }
                    
                    argsArrayTable->arrayObjects.push_back(valueRef);
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo);
                    if(*s == ',')
                    {
                        s++;
                        s = tuiSkipToNextChar(s, debugInfo);
                    }
                    
                }
                
                s++; //')'
                
                s = tuiSkipToNextChar(s, debugInfo, true);
                *endptr = (char*)s;
                resultRef = ((TuiFunction*)resultRef)->call(argsArrayTable, parent, existingValue, debugInfo);
                if(argsArrayTable)
                {
                    argsArrayTable->release();
                }
            }
            else
            {
                resultRef->retain();
            }
            
            return resultRef;
        }
        else if(isFunctionCall)
        {
            TuiParseWarn(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Attempt to call missing function:%s", stringBuffer.c_str());
        }
        return nullptr;
    }
    
    if(!stringBuffer.empty())
    {
        TuiString* tuiString = new TuiString(stringBuffer, parent);
        return tuiString;
    }
    
    return nullptr;
}

// parses a variable chain and returns the result eg: foo.bar().array[1+2].x
// optionally stores the enclosing ref and the final variable name if found
TuiRef* TuiRef::loadValue(const char* str,
                          char** endptr,
                          TuiRef* existingValue,
                          TuiTable* parentTable,
                          TuiDebugInfo* debugInfo,
                          bool allowQuotedStringsAsVariableNames,
                          
                          //below are only passed if there is a chance we are finding a key in order to set its value, giving the caller quick access to the parent to set the value for an uninitialized variable
                          TuiRef** onSetEnclosingRef,
                          std::string* onSetKey,
                          int* onSetIndex) //index todo
{
    const char* s = str;
    
    TuiRef* varChainParent = nullptr;
    TuiRef* result = nullptr;
    
    while(1)
    {
        result = loadSingleValueInternal(s, endptr, existingValue, parentTable, varChainParent, debugInfo, allowQuotedStringsAsVariableNames, onSetKey, onSetIndex);
        s = *endptr;
        
        if(*s == '.')
        {
            if(varChainParent)
            {
                varChainParent->release();
            }
            varChainParent = result;
            s++;
        }
        else if(*s == '[')
        {
            if(varChainParent)
            {
                varChainParent->release();
            }
            varChainParent = result;
        }
        else
        {
            break;
        }
        
        //if we have another value, these are now invalid, we don't want to send one of them back
        if(onSetKey)
        {
            *onSetKey = "";
        }
        if(onSetIndex)
        {
            *onSetIndex = -1;
        }
    }
    
    
    if(onSetEnclosingRef)
    {
        *onSetEnclosingRef = (varChainParent ? varChainParent : parentTable);
    }
    
    return result;
}

TuiBool* TuiRef::logicalNot(TuiRef* value)
{
    return new TuiBool(!value || !value->boolValue());
}


TuiRef* TuiRef::loadExpression(const char* str,
                               char** endptr,
                               TuiRef* existingValue,
                               TuiRef* leftValue,
                               TuiTable* parentTable,
                               TuiDebugInfo* debugInfo,
                               int operatorLevel,
                               bool allowQuotedStringsAsVariableNames)// this is a hack to allow quoted strings as variable names for keys only, and is only observed for this purpose (ie only simple var names via the call to loadValue below).
{
    const char* s = str;
    
    
    if(!leftValue)
    {
        if(*s == '!')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            TuiRef* rightValue = TuiRef::loadExpression(s, endptr, existingValue, nullptr, parentTable, debugInfo, Tui_operator_level_not);
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
            leftValue = TuiRef::loadExpression(s, endptr, existingValue, nullptr, parentTable, debugInfo, Tui_operator_level_default);
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
            leftValue = TuiRef::loadValue(s, endptr, existingValue, parentTable, debugInfo, allowQuotedStringsAsVariableNames);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
        }
        
        if(!leftValue)
        {
            leftValue = existingValue;
            if(!leftValue)
            {
                leftValue = new TuiRef(parentTable);
            }
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
                        result = TuiRef::loadExpression(s, endptr, existingValue, result, parentTable, debugInfo, operatorLevel);
                        s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    }
                }
                else if(Tui_operator_level_and_or >= operatorLevel)
                {
                    bool newOperatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
                    bool newOperatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
                    if(newOperatorOr || newOperatorAnd)
                    {
                        result = TuiRef::loadExpression(s, endptr, existingValue, result, parentTable, debugInfo, operatorLevel);
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
    
    TuiRef* rightValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parentTable, debugInfo, newOperatorLevel);
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    
    bool existingValueWasAssigned = false;
    
    if(operatorChar == '=')
    {
        if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
        {
            ((TuiBool*)existingValue)->value = leftValue->isEqual(rightValue);
            existingValueWasAssigned = true;
        }
        else
        {
            result = new TuiBool(leftValue->isEqual(rightValue));
        }
    }
    else if(operatorChar == '!')
    {
        if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
        {
            ((TuiBool*)existingValue)->value = !leftValue->isEqual(rightValue);
            existingValueWasAssigned = true;
        }
        else
        {
            result = new TuiBool(!leftValue->isEqual(rightValue));
        }
    }
    else if(operatorOr)
    {
        if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
        {
            ((TuiBool*)existingValue)->value = leftValue->boolValue() || rightValue->boolValue();
            existingValueWasAssigned = true;
        }
        else
        {
            result = new TuiBool(leftValue->boolValue() || rightValue->boolValue());
        }
    }
    else if(operatorAnd)
    {
        if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
        {
            ((TuiBool*)existingValue)->value = leftValue->boolValue() && rightValue->boolValue();
            existingValueWasAssigned = true;
        }
        else
        {
            result = new TuiBool(leftValue->boolValue() && rightValue->boolValue());
        }
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
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array //todo this is a waste of an allocate
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                ((TuiNumber*)existingValue)->value = ((TuiNumber*)leftValue)->value + ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiNumber(((TuiNumber*)leftValue)->value + ((TuiNumber*)rightValue)->value);
                            }
                        }
                    }
                        break;
                    case '-':
                    {
                        if(secondOperatorChar == '=')
                        {
                            ((TuiNumber*)leftValue)->value -= ((TuiNumber*)rightValue)->value;
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array //todo no need to allocate this
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                ((TuiNumber*)existingValue)->value = ((TuiNumber*)leftValue)->value - ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiNumber(((TuiNumber*)leftValue)->value - ((TuiNumber*)rightValue)->value);
                            }
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
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                ((TuiNumber*)existingValue)->value = ((TuiNumber*)leftValue)->value * ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiNumber(((TuiNumber*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                            }
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
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                ((TuiNumber*)existingValue)->value = ((TuiNumber*)leftValue)->value / ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiNumber(((TuiNumber*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                            }
                        }
                    }
                        break;
                    case '>':
                    {
                        if(secondOperatorChar == '=')
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
                            {
                                ((TuiBool*)existingValue)->value = ((TuiNumber*)leftValue)->value >= ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiBool(((TuiNumber*)leftValue)->value >= ((TuiNumber*)rightValue)->value);
                            }
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
                            {
                                ((TuiBool*)existingValue)->value = ((TuiNumber*)leftValue)->value > ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiBool(((TuiNumber*)leftValue)->value > ((TuiNumber*)rightValue)->value);
                            }
                        }
                    }
                        break;
                    case '<':
                    {
                        if(secondOperatorChar == '=')
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
                            {
                                ((TuiBool*)existingValue)->value = ((TuiNumber*)leftValue)->value <= ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiBool(((TuiNumber*)leftValue)->value <= ((TuiNumber*)rightValue)->value);
                            }
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_BOOL)
                            {
                                ((TuiBool*)existingValue)->value = ((TuiNumber*)leftValue)->value < ((TuiNumber*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiBool(((TuiNumber*)leftValue)->value < ((TuiNumber*)rightValue)->value);
                            }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)rightValue)->value * ((TuiNumber*)leftValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)rightValue)->value * ((TuiNumber*)leftValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)rightValue)->value / ((TuiNumber*)leftValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)rightValue)->value / ((TuiNumber*)leftValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)rightValue)->value * ((TuiNumber*)leftValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)rightValue)->value * ((TuiNumber*)leftValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)rightValue)->value / ((TuiNumber*)leftValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)rightValue)->value / ((TuiNumber*)leftValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)rightValue)->value * ((TuiNumber*)leftValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)rightValue)->value * ((TuiNumber*)leftValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)rightValue)->value / ((TuiNumber*)leftValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)rightValue)->value / ((TuiNumber*)leftValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)leftValue)->value * ((TuiNumber*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)leftValue)->value / ((TuiNumber*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)leftValue)->value / ((TuiNumber*)rightValue)->value);
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
                    case '+':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)leftValue)->value + ((TuiVec2*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)leftValue)->value + ((TuiVec2*)rightValue)->value);
                        }
                    }
                        break;
                    case '-':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)leftValue)->value - ((TuiVec2*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)leftValue)->value - ((TuiVec2*)rightValue)->value);
                        }
                    }
                        break;
                    case '*':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)leftValue)->value * ((TuiVec2*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)leftValue)->value * ((TuiVec2*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)existingValue)->value = ((TuiVec2*)leftValue)->value / ((TuiVec2*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec2(((TuiVec2*)leftValue)->value / ((TuiVec2*)rightValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)leftValue)->value * ((TuiNumber*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)leftValue)->value / ((TuiNumber*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)leftValue)->value + ((TuiVec3*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)leftValue)->value + ((TuiVec3*)rightValue)->value);
                        }
                    }
                        break;
                    case '-':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)leftValue)->value - ((TuiVec3*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)leftValue)->value - ((TuiVec3*)rightValue)->value);
                        }
                    }
                        break;
                    case '*':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)leftValue)->value * ((TuiVec3*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)leftValue)->value * ((TuiVec3*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)existingValue)->value = ((TuiVec3*)leftValue)->value / ((TuiVec3*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec3(((TuiVec3*)leftValue)->value / ((TuiVec3*)rightValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)leftValue)->value * ((TuiNumber*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)leftValue)->value * ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)leftValue)->value / ((TuiNumber*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)leftValue)->value / ((TuiNumber*)rightValue)->value);
                        }
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
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)leftValue)->value + ((TuiVec4*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)leftValue)->value + ((TuiVec4*)rightValue)->value);
                        }
                    }
                        break;
                    case '-':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)leftValue)->value - ((TuiVec4*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)leftValue)->value - ((TuiVec4*)rightValue)->value);
                        }
                    }
                        break;
                    case '*':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)leftValue)->value * ((TuiVec4*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)leftValue)->value * ((TuiVec4*)rightValue)->value);
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)existingValue)->value = ((TuiVec4*)leftValue)->value / ((TuiVec4*)rightValue)->value;
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiVec4(((TuiVec4*)leftValue)->value / ((TuiVec4*)rightValue)->value);
                        }
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
                        if(secondOperatorChar == '=')
                        {
                            ((TuiString*)leftValue)->value += ((TuiString*)rightValue)->value;
                            result = new TuiRef(); //return a nil ref, otherwise the key is addded to an array //todo this is a waste of an allocate
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_STRING)
                            {
                                ((TuiString*)existingValue)->value = ((TuiString*)leftValue)->value + ((TuiString*)rightValue)->value;
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = new TuiString(((TuiString*)leftValue)->value + ((TuiString*)rightValue)->value);
                            }
                        }
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
    
    if(result || existingValueWasAssigned)
    {
        leftValue->release();
        rightValue->release();
        
        if(TuiExpressionOperatorsSet.count(*s) != 0)
        {
            int newOperatorLevel = TuiExpressionOperatorsToLevelMap[*s];
            if(newOperatorLevel >= operatorLevel)
            {
                result = TuiRef::loadExpression(s, endptr, existingValue, result, parentTable, debugInfo, operatorLevel);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
        }
        else if(Tui_operator_level_and_or >= operatorLevel)
        {
            bool newOperatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
            bool newOperatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
            if(newOperatorOr || newOperatorAnd)
            {
                result = TuiRef::loadExpression(s, endptr, existingValue, result, parentTable, debugInfo, operatorLevel);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
        }
        
        *endptr = (char*)s;
        
        return result;
    }
    
    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid expression");
    return nullptr;
}


/************************************************  TuiUserData methods *****************************************************/

TuiUserData::TuiUserData(void* value_, TuiTable* parent_)
: TuiRef(parent_)
{
    value = value_;
}

TuiUserData::~TuiUserData()
{
    if(members)
    {
        members->release();
    }
};
