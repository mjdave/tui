
#include "TuiRef.h"

#include "TuiTable.h"
#include "TuiNumber.h"


TuiBool* TUI_TRUE = new TuiBool(true);
TuiBool* TUI_FALSE = new TuiBool(false);


TuiRef* TuiRef::loadString(const std::string& inputString, const std::string& debugName, TuiTable* parent) {
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


TuiRef* TuiRef::loadBinaryString(const char* inputString, int* currentOffset, TuiTable* parent)
{
    uint8_t type = inputString[(*currentOffset)++];
    switch (type) {
        case Tui_binary_type_NIL:
        {
            return TUI_NIL;
        }
            break;
        case Tui_binary_type_BOOL_TRUE:
        {
            return TUI_TRUE;
        }
            break;
        case Tui_binary_type_BOOL_FALSE:
        {
            return TUI_FALSE;
        }
            break;
        case Tui_binary_type_NUMBER:
        {
            double value;
            memcpy(&value, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            return new TuiNumber(value);
        }
            break;
        case Tui_binary_type_VEC2:
        {
            dvec2 value;
            memcpy(&value.x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            return new TuiVec2(value);
        }
            break;
        case Tui_binary_type_VEC3:
        {
            dvec3 value;
            memcpy(&value.x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.z, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            return new TuiVec3(value);
        }
            break;
        case Tui_binary_type_VEC4:
        {
            dvec4 value;
            memcpy(&value.x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.z, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.w, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            return new TuiVec4(value);
        }
            break;
        case Tui_binary_type_STRING:
        {
            uint32_t stringLength;
            memcpy(&stringLength, &inputString[(*currentOffset)], 4);
            (*currentOffset)+=4;
            
            TuiString* stringRef = new TuiString("");
            stringRef->value.resize(stringLength);
            memcpy(stringRef->value.data(), &inputString[(*currentOffset)], stringLength);
            (*currentOffset)+=stringLength;
            
            return stringRef;
        }
            break;
        case Tui_binary_type_TABLE:
        {
            TuiTable* table = new TuiTable(parent);
            while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
            {
                TuiRef* arrayObject = TuiRef::loadBinaryString(inputString, currentOffset);
                table->arrayObjects.push_back(arrayObject);
            }
            (*currentOffset)++;
            
            //todo objectsByNumberKey
            
            while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
            {
                TuiRef* keyObject = TuiRef::loadBinaryString(inputString, currentOffset);
                TuiRef* valueObject = TuiRef::loadBinaryString(inputString, currentOffset);
                table->set(((TuiString*)keyObject)->value, valueObject);
                keyObject->release();
                valueObject->release();
            }
            (*currentOffset)++;
            
            return table;
        }
            break;
            
        default:
            break;
    }
    
    return nullptr;
}

TuiRef* TuiRef::loadBinaryString(const std::string& inputString, TuiTable* parent)
{
    int currentOffset = 0;
    return loadBinaryString(inputString.c_str(), &currentOffset);
}

TuiRef* TuiRef::loadBinary(const std::string& path, TuiTable* parent)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    TuiDebugInfo debugInfo;
    debugInfo.fileName = path;
    if(in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        int currentOffset = 0;
        return loadBinaryString(contents.c_str(), &currentOffset);
    }
    else
    {
        TuiError("File not found in TuiRef::loadBinary at:%s", path.c_str());
    }
    return nullptr;
}

TuiRef* TuiRef::runScriptFile(const std::string& path, TuiTable* parent)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    TuiDebugInfo debugInfo;
    debugInfo.fileName = path;
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
        TuiError("File not found in TuiRef::runScriptFile at:%s", path.c_str());
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
                                       int* onSetIndex, //index todo
                                       bool* accessedParentVariable)
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
                                                     nullptr,
                                                     accessedParentVariable);
            if(result)
            {
                bool newValue = !result->boolValue();
                result->release();
                return TUI_BOOL(newValue);
            }
            return TUI_TRUE;
        }
        
        if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            double value = strtod(s, endptr);
            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
            {
                ((TuiNumber*)existingValue)->value = value;
                return nullptr;
            }
            TuiNumber* number = new TuiNumber(value);
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
            
            return TUI_NIL;
        }
        else if(*s == 'n'
                && *(s + 1) == 'u'
                && *(s + 2) == 'l'
                && *(s + 3) == 'l')
        {
            s+=4;
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            return TUI_NIL;
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
                return TUI_NIL;
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
                escaped = false;
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
                escaped = false;
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
        else if(escaped)
        {
            escaped = false;
            switch(*s)
            {
                case 'n':
                    stringBuffer += '\n';
                    break;
                case 'r':
                    stringBuffer += '\r';
                    break;
                case 't':
                    stringBuffer += '\t';
                    break;
                case 'a':
                    stringBuffer += '\a';
                    break;
                case 'v':
                    stringBuffer += '\v';
                    break;
                default:
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Unrecognized escape code:%c", *s)
                    break;
            }
        }
        else if(singleQuote || doubleQuote)
        {
            stringBuffer += *s;
            escaped = false;
        }
        else if(*s == '.' && allowAsVariableName)
        {
            break;
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
        else if(isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || *s == ':' || *s == ']' || TuiExpressionOperatorsSet.count(*s) != 0)
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
        if(onSetKey && !isFunctionCall) // !isFunctionCall prevents {func()} from adding 'func' to an array, bit of a hack
        {
            *onSetKey = stringBuffer;
        }
        
        TuiRef* resultRef = nullptr;
        if(varChainParent && varChainParent->type() != Tui_ref_type_TABLE)
        {
            switch(varChainParent->type())
            {
                case Tui_ref_type_VEC2:
                {
                    switch(stringBuffer[0])
                    {
                        case 'x':
                            resultRef = new TuiNumber(((TuiVec2*)varChainParent)->value.x);
                            break;
                        case 'y':
                            resultRef = new TuiNumber(((TuiVec2*)varChainParent)->value.y);
                            break;
                        default:
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid value");
                            break;
                    }
                }
                    break;
                case Tui_ref_type_VEC3:
                {
                    switch(stringBuffer[0])
                    {
                        case 'x':
                            resultRef = new TuiNumber(((TuiVec3*)varChainParent)->value.x);
                            break;
                        case 'y':
                            resultRef = new TuiNumber(((TuiVec3*)varChainParent)->value.y);
                            break;
                        case 'z':
                            resultRef = new TuiNumber(((TuiVec3*)varChainParent)->value.z);
                            break;
                        default:
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid value");
                            break;
                    }
                }
                    break;
                case Tui_ref_type_VEC4:
                {
                    switch(stringBuffer[0])
                    {
                        case 'x':
                            resultRef = new TuiNumber(((TuiVec4*)varChainParent)->value.x);
                            break;
                        case 'y':
                            resultRef = new TuiNumber(((TuiVec4*)varChainParent)->value.y);
                            break;
                        case 'z':
                            resultRef = new TuiNumber(((TuiVec4*)varChainParent)->value.z);
                            break;
                        case 'w':
                            resultRef = new TuiNumber(((TuiVec4*)varChainParent)->value.w);
                            break;
                        default:
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid value");
                            break;
                    }
                }
                    break;
            }
        }
        else
        {
            TuiTable* searchTable = (varChainParent ? (TuiTable*)varChainParent :  parent);
            
            if(searchTable->objectsByStringKey.count(stringBuffer) != 0)
            {
                resultRef = searchTable->objectsByStringKey[stringBuffer];
            }
            
            while(!resultRef && searchTable->parentTable)
            {
                searchTable = searchTable->parentTable;
                if(searchTable->objectsByStringKey.count(stringBuffer) != 0)
                {
                    if(accessedParentVariable)
                    {
                        *accessedParentVariable = true;
                    }
                    resultRef = searchTable->objectsByStringKey[stringBuffer];
                }
            }
            
            if(resultRef)
            {
                resultRef->retain();
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
                                                              parent,
                                                              debugInfo);
                    if(!valueRef)
                    {
                        valueRef = TUI_NIL;
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
                TuiRef* newRef = ((TuiFunction*)resultRef)->call(argsArrayTable, existingValue, debugInfo);
                resultRef->release();
                resultRef = newRef;
                if(argsArrayTable)
                {
                    argsArrayTable->release();
                }
            }
            
            return resultRef;
        }
        else if(isFunctionCall)
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Attempt to call missing function:%s", stringBuffer.c_str());
            return nullptr;
        }
        
        return nullptr;
    }
    
    if(!stringBuffer.empty())
    {
        TuiString* tuiString = new TuiString(stringBuffer);
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
                          int* onSetIndex, //index todo
                          bool* accessedParentVariable)
{
    const char* s = str;
    
    TuiRef* varChainParent = nullptr;
    TuiRef* result = nullptr;
    
    if(*s == '.')
    {
            
        const char* sTmp = s;
        sTmp++;
        std::string keyString;
        bool done = false;
        while(!done)
        {
            switch (*sTmp) {
                case '\0':
                case '.':
                case '(':
                case '[':
                case ' ':
                case '=':
                case ',':
                case '\n':
                {
                    done = true;
                    break;
                }
                    break;
                    
                default:
                {
                    keyString += *sTmp;
                }
                    break;
            }
            sTmp++;
        }
        
        if(keyString.empty())
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Something went wrong");
        }
        
        TuiTable* parentToUse = parentTable->parentTable;
        
        s++;
        s = tuiSkipToNextChar(s, debugInfo);
        
        
        while(parentToUse->parentTable)
        {
            if(parentToUse->objectsByStringKey.count(keyString) != 0)
            {
                break;
            }
            
            parentToUse = parentToUse->parentTable;
        }
        
        
        varChainParent = parentToUse;
        varChainParent->retain();
        
        /*result = loadSingleValueInternal(s,
                                                 endptr,
                                                 nullptr,
                                                 parentToUse,
                                                 nullptr,
                                                 debugInfo,
                                                 allowQuotedStringsAsVariableNames,
                                                 
                                                 nullptr,
                                                 nullptr,
                                                 accessedParentVariable);*/
    }

    while(1)
    {
        result = loadSingleValueInternal(s, endptr, existingValue, parentTable, varChainParent, debugInfo, allowQuotedStringsAsVariableNames, onSetKey, onSetIndex, accessedParentVariable);
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
        (*onSetEnclosingRef)->retain();
    }
    
    if(varChainParent)
    {
        varChainParent->release();
    }
    
    return result;
}

TuiBool* TuiRef::logicalNot(TuiRef* value)
{
    return TUI_BOOL(!value || !value->boolValue());
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
            TuiRef* rightValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parentTable, debugInfo, Tui_operator_level_not);
            if(rightValue)
            {
                leftValue = TuiRef::logicalNot(rightValue);
                rightValue->release();
            }
            else
            {
                leftValue = TUI_TRUE;
            }
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
        else if(*s == '-')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            TuiRef* rightValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parentTable, debugInfo, Tui_operator_level_not);
            if(rightValue)
            {
                switch (rightValue->type()) {
                    case Tui_ref_type_NUMBER:
                    {
                        leftValue = new TuiNumber(-((TuiNumber*)rightValue)->value);
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        leftValue = new TuiVec2(-((TuiVec2*)rightValue)->value);
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        leftValue = new TuiVec3(-((TuiVec3*)rightValue)->value);
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        leftValue = new TuiVec4(-((TuiVec4*)rightValue)->value);
                    }
                        break;
                        
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number or vector, got:%s", (rightValue ? rightValue->getDebugString().c_str() : "nil"));
                        break;
                }
                
                rightValue->release();
            }
            else
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number or vector, got:%s", (rightValue ? rightValue->getDebugString().c_str() : "nil"));
            }
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
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
                leftValue = TUI_NIL;
            }
            else
            {
                leftValue->retain();
            }
        }
    }
    
    char operatorChar = *s;
    
    if(operatorChar == '\0' || operatorChar == ')' || operatorChar == ']')
    {
        *endptr = (char*)s;
        if(existingValue)
        {
            leftValue->release();
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
                leftValue->release();
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
                        result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array
                    }
                        break;
                    case '-':
                    {
                        ((TuiNumber*)leftValue)->value--;
                        result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array
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
            
            leftValue->release();
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
        result = TUI_BOOL(leftValue->isEqual(rightValue));
    }
    else if(operatorChar == '!')
    {
        result = TUI_BOOL(!leftValue->isEqual(rightValue));
    }
    else if(operatorOr)
    {
        result = TUI_BOOL(leftValue->boolValue() || rightValue->boolValue());
    }
    else if(operatorAnd)
    {
        result = TUI_BOOL(leftValue->boolValue() && rightValue->boolValue());
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
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array //todo this is a waste of an allocate
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
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array //todo no need to allocate this
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
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array
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
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array
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
                    case '%':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                        {
                            ((TuiNumber*)existingValue)->value = ((int)((TuiNumber*)leftValue)->value) % ((int)((TuiNumber*)rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiNumber(((int)((TuiNumber*)leftValue)->value) % ((int)((TuiNumber*)rightValue)->value));
                        }
                    }
                        break;
                    case '>':
                    {
                        if(secondOperatorChar == '=')
                        {
                            result = TUI_BOOL(((TuiNumber*)leftValue)->value >= ((TuiNumber*)rightValue)->value);
                        }
                        else
                        {
                            result = TUI_BOOL(((TuiNumber*)leftValue)->value > ((TuiNumber*)rightValue)->value);
                        }
                    }
                        break;
                    case '<':
                    {
                        if(secondOperatorChar == '=')
                        {
                            result = TUI_BOOL(((TuiNumber*)leftValue)->value <= ((TuiNumber*)rightValue)->value);
                        }
                        else
                        {
                            result = TUI_BOOL(((TuiNumber*)leftValue)->value < ((TuiNumber*)rightValue)->value);
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
            switch(operatorChar)
            {
                case '+':
                {
                    if(secondOperatorChar == '=')
                    {
                        ((TuiString*)leftValue)->value += rightValue->getStringValue();
                        result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array //todo this is a waste of an allocate
                    }
                    else
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_STRING)
                        {
                            ((TuiString*)existingValue)->value = ((TuiString*)leftValue)->value + rightValue->getStringValue();
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = new TuiString(((TuiString*)leftValue)->value + rightValue->getStringValue());
                        }
                    }
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

TuiUserData::TuiUserData(void* value_)
: TuiRef()
{
    value = value_;
}
