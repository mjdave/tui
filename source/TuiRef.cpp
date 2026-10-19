
#include "TuiRef.h"
#include "TuiTypes.h"

#include "TuiFileUtils.h"
#include "TuiTable.h"
#include "TuiNumber.h"


Tui::boolean TUI_TRUE = Tui::create<TuiBool>(true);
Tui::boolean TUI_FALSE = Tui::create<TuiBool>(false);


Tui::ref TuiRef::loadString(const std::string& inputString, const std::string& debugName, Tui::table parent) {
    TuiDebugInfo debugInfo;
    TuiDebugInfoPush(&debugInfo, debugName, 1);
    const char* cString = inputString.c_str();
    char* endPtr;
    
    return TuiRef::load(cString, &endPtr, parent, &debugInfo);
}

Tui::ref TuiRef::loadString(const std::string& inputString, Tui::table parent, TuiDebugInfo* callingDebugInfo)
{
    TuiDebugInfo debugInfo;
    TuiDebugInfoCopy(callingDebugInfo, &debugInfo);
    const char* cString = inputString.c_str();
    char* endPtr;
    
    return TuiRef::load(cString, &endPtr, parent, &debugInfo);
}

Tui::ref TuiRef::runScriptFile(const std::string& path, Tui::table parent, TuiDebugInfo* callingDebugInfo, Tui::ref resultRef)
{
    // Get absolute path from path
    std::string absolutePath = Tui::getAbsolutePath(path);

    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    TuiDebugInfo debugInfo;
    if(callingDebugInfo)
    {
        TuiDebugInfoCopy(callingDebugInfo, &debugInfo);
    }
    TuiDebugInfoPush(&debugInfo, absolutePath, 1);
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
        return TuiRef::load(cString, &endPtr, parent, &debugInfo, &resultRef);
    }
    else
    {
        TuiError("File not found in TuiRef::runScriptFile at:%s", path.c_str());
    }
    return nullptr;
}

Tui::ref TuiRef::require(const std::string& path, Tui::table rootTable)
{
    Tui::function requireFunction = rootTable->getFunction("require");
    Tui::string arg = Tui::create<TuiString>(path);
    Tui::ref result = requireFunction->call("require", arg);
    return result;
}

Tui::ref TuiRef::loadBinaryString(const char* inputString, int* currentOffset, Tui::table parent)
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
            return Tui::create<TuiNumber>(value);
        }
            break;
        case Tui_binary_type_NUMBER_8:
        {
            uint8_t value;
            memcpy(&value, &inputString[(*currentOffset)], 1);
            (*currentOffset)+=1;
            return Tui::create<TuiNumber8>(value);
        }
            break;
        case Tui_binary_type_NUMBER_16:
        {
            uint16_t value;
            memcpy(&value, &inputString[(*currentOffset)], 2);
            (*currentOffset)+=2;
            return Tui::create<TuiNumber16>(value);
        }
            break;
        case Tui_binary_type_NUMBER_32:
        {
            uint32_t value;
            memcpy(&value, &inputString[(*currentOffset)], 4);
            (*currentOffset)+=4;
            return Tui::create<TuiNumber32>(value);
        }
            break;
        case Tui_binary_type_NUMBER_64:
        {
            uint64_t value;
            memcpy(&value, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            return Tui::create<TuiNumber64>(value);
        }
            break;
        case Tui_binary_type_VEC2:
        {
            dvec2 value;
            memcpy(&value.x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value.y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            return Tui::create<TuiVec2>(value);
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
            return Tui::create<TuiVec3>(value);
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
            return Tui::create<TuiVec4>(value);
        }
            break;
        case Tui_binary_type_MAT3:
        {
            dmat3 value;
            memcpy(&value[0].x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[0].y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[0].z, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[1].x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[1].y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[1].z, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[2].x, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[2].y, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            memcpy(&value[2].z, &inputString[(*currentOffset)], 8);
            (*currentOffset)+=8;
            
            return Tui::create<TuiMat3>(value);
        }
            break;
        case Tui_binary_type_STRING:
        {
            uint32_t stringLength;
            memcpy(&stringLength, &inputString[(*currentOffset)], 4);
            (*currentOffset)+=4;
            
            Tui::string stringRef = Tui::create<TuiString>("");
            stringRef->value.resize(stringLength);
            memcpy(&stringRef->value[0], &inputString[(*currentOffset)], stringLength);
            (*currentOffset)+=stringLength;
            
            return stringRef;
        }
            break;
        case Tui_binary_type_TABLE:
        {
            Tui::table table = Tui::create<TuiTable>(parent);
            
            if(inputString[(*currentOffset)] == Tui_binary_type_NUMBER_8_SET)
            {
                (*currentOffset)++;
                while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
                {
                    uint8_t value;
                    memcpy(&value, &inputString[(*currentOffset)], sizeof(value));
                    (*currentOffset)+=sizeof(value);
                    table->set8.insert(value);
                }
                (*currentOffset)++;
            }
            
            if(inputString[(*currentOffset)] == Tui_binary_type_NUMBER_16_SET)
            {
                (*currentOffset)++;
                while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
                {
                    uint16_t value;
                    memcpy(&value, &inputString[(*currentOffset)], sizeof(value));
                    (*currentOffset)+=sizeof(value);
                    table->set16.insert(value);
                }
                (*currentOffset)++;
            }
            
            if(inputString[(*currentOffset)] == Tui_binary_type_NUMBER_32_SET)
            {
                (*currentOffset)++;
                while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
                {
                    uint32_t value;
                    memcpy(&value, &inputString[(*currentOffset)], sizeof(value));
                    (*currentOffset)+=sizeof(value);
                    table->set32.insert(value);
                }
                (*currentOffset)++;
            }
            
            if(inputString[(*currentOffset)] == Tui_binary_type_NUMBER_64_SET)
            {
                (*currentOffset)++;
                while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
                {
                    uint64_t value;
                    memcpy(&value, &inputString[(*currentOffset)], sizeof(value));
                    (*currentOffset)+=sizeof(value);
                    table->set64.insert(value);
                }
                (*currentOffset)++;
            }
            
            while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
            {
                Tui::ref arrayObject = TuiRef::loadBinaryString(inputString, currentOffset);
                table->arrayObjects.push_back(arrayObject);
            }
            (*currentOffset)++;
            
            while(inputString[(*currentOffset)] != Tui_binary_type_END_MARKER)
            {
                Tui::ref keyObject = TuiRef::loadBinaryString(inputString, currentOffset);
                Tui::ref valueObject = TuiRef::loadBinaryString(inputString, currentOffset);
                if(keyObject->type() == Tui_ref_type_NUMBER_32)
                {
                    table->set(Tui::cast<TuiNumber32>(keyObject)->value, valueObject);
                }
                else
                {
                    table->set(Tui::cast<TuiString>(keyObject)->value, valueObject);
                }
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

Tui::ref TuiRef::loadBinaryString(const std::string& inputString, Tui::table parent)
{
    int currentOffset = 0;
    return loadBinaryString(inputString.c_str(), &currentOffset);
}

Tui::ref TuiRef::loadBinary(const std::string& path, Tui::table parent)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
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


Tui::ref TuiRef::load(const char* str, char** endptr, Tui::table parent, TuiDebugInfo* debugInfo, Tui::ref* resultRef) {
    
    Tui::table table = Tui::create<TuiTable>(parent);
    
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
                Tui::ref object = table->arrayObjects[0];
                return object;
            }
            else if(arrayObjectCount == 0)
            {
                return nullptr;
            }
        }
    }
    
    return table;
}

// parses a single token and returns the result eg: foo, bar(), array, [1+2], x, 42
static Tui::ref loadSingleValueInternal(const char* str,
                                       char** endptr,
                                       Tui::ref existingValue,
                                       Tui::ref parentRef,
                                       Tui::ref varChainParent,
                                       TuiDebugInfo* debugInfo,
                                       
                                       std::string* onSetKey, //watch out, using the existance of this var to allow "x":5 json quoted key names only. For values, a quoted string is not a valid variable name
                                       int* onSetIndex,
                                       bool* accessedParentVariable,
                                       std::string& stringBuffer,
                                       std::string debugVarChainParentName)
{
    const char* s = str;
    
    Tui::table parent = Tui::cast<TuiTable>(parentRef);
    if(parentRef->type() != Tui_ref_type_TABLE)
    {
        TuiError("Unimplemented");
    }
    
    if(!varChainParent)
    {
        if(*s == '(')
        {
            Tui::ref result = TuiRef::loadExpression(s,
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
            std::string subStringBuffer;
            Tui::ref result = loadSingleValueInternal(s,
                                                     endptr,
                                                     nullptr,
                                                     parent,
                                                     nullptr,
                                                     debugInfo,
                                                     
                                                     nullptr,
                                                     nullptr,
                                                     accessedParentVariable,
                                                     subStringBuffer,
                                                     "");
            if(result)
            {
                bool newValue = !result->boolValue();
                return TUI_BOOL(newValue);
            }
            return TUI_TRUE;
        }
        
        if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            double value = strtod(s, endptr);
            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
            {
                Tui::cast<TuiNumber>(existingValue)->value = value;
                return nullptr;
            }
            Tui::number number = Tui::create<TuiNumber>(value);
            return number;
        }
        
        if(*s == 't' && *(s + 1) == 'h' && *(s + 2) == 'i' && *(s + 3) == 's' && checkSymbolNameComplete(s + 4))
        {
            *endptr = (char*)(s + 4);
            return parentRef;
        }
        
        //todo pull out these functions, set existing values correctly
        Tui::function functionRef = TuiFunction::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(functionRef)
        {
            return functionRef;
        }
        
        Tui::boolean boolRef = TuiBool::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(boolRef)
        {
            return boolRef;
        }
        
        Tui::vec2 vec2Ref = TuiVec2::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(vec2Ref)
        {
            return vec2Ref;
        }
        
        Tui::vec3 vec3Ref = TuiVec3::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(vec3Ref)
        {
            return vec3Ref;
        }
        
        Tui::vec4 vec4Ref = TuiVec4::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(vec4Ref)
        {
            return vec4Ref;
        }
        
        Tui::mat3 mat3Ref = TuiMat3::initWithHumanReadableString(s, endptr, parent, debugInfo);
        if(mat3Ref)
        {
            return mat3Ref;
        }
        
        if(*s == 'n'
           && *(s + 1) == 'i'
           && *(s + 2) == 'l' && checkSymbolNameComplete(s + 3))
        {
            s+=3;
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return TUI_NIL;
        }
        else if(*s == 'n'
                && *(s + 1) == 'u'
                && *(s + 2) == 'l'
                && *(s + 3) == 'l' && checkSymbolNameComplete(s + 4))
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
            Tui::ref index = TuiRef::loadExpression(s,
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
                TuiParseError(debugInfo, "Invalid table index:%s", (index ? index->getDebugString().c_str() : "nil"));
                return nullptr;
            }
            
            if(varChainParent->type() != Tui_ref_type_TABLE)
            {
                TuiError("Unimplemented");
            }
            
            int64_t indexValue = (Tui::cast<TuiNumber>(index)->value);
            
            if(onSetIndex)
            {
                *onSetIndex = (uint32_t)indexValue;
            }
            
            if(indexValue >= 0 && indexValue < Tui::cast<TuiTable>(varChainParent)->arrayObjects.size())
            {
                Tui::ref result = (Tui::cast<TuiTable>(varChainParent)->arrayObjects[indexValue]);
                if(result)
                {
                }
                return result;
            }
            
            if(Tui::cast<TuiTable>(varChainParent)->objectsByNumberKey.count((uint32_t)indexValue) != 0)
            {
                Tui::ref result = (Tui::cast<TuiTable>(varChainParent)->objectsByNumberKey[(uint32_t)indexValue]);
                return result;
            }
            
            if(Tui::cast<TuiTable>(varChainParent)->set8.count((uint8_t)indexValue) != 0)
            {
                return TUI_TRUE;
            }
            
            if(Tui::cast<TuiTable>(varChainParent)->set16.count((uint16_t)indexValue) != 0)
            {
                return TUI_TRUE;
            }
            
            if(Tui::cast<TuiTable>(varChainParent)->set32.count((uint32_t)indexValue) != 0)
            {
                return TUI_TRUE;
            }
            
            if(Tui::cast<TuiTable>(varChainParent)->set64.count(indexValue) != 0)
            {
                return TUI_TRUE;
            }
            
            
            return TUI_NIL;
        }
    }
    
    bool singleQuote = false;
    bool doubleQuote = false;
    bool escaped = false;
    bool foundSpace = false;
    
    bool allowAsVariableName = true;
    bool isFunctionCall = false;
    
    bool foundAnyQuote = false;
    
    
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
                        foundAnyQuote = true;
                        allowAsVariableName = false;
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
                        allowAsVariableName = false;
                        foundAnyQuote = true;
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
                    TuiParseError(debugInfo, "Unrecognized escape code:%c", *s)
                    break;
            }
        }
        else if(singleQuote || doubleQuote)
        {
            stringBuffer += *s;
            escaped = false;
            if(*s == '\n')
            {
                if(debugInfo)
                {
                    debugInfo->currentLine->lineNumber++;
                }
            }
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
        else if(isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || *s == ':' || *s == ']' || *s == '}' || TuiExpressionOperatorsSet.count(*s) != 0)
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
    
    
    if(onSetKey && !isFunctionCall) // !isFunctionCall prevents {func()} from adding 'func' to an array, bit of a hack
    {
        *onSetKey = stringBuffer;
    }
    
    if(allowAsVariableName)
    {
        
        Tui::ref resultRef = nullptr;
        if(varChainParent && varChainParent->type() != Tui_ref_type_TABLE)
        {
            switch(varChainParent->type())
            {
                case Tui_ref_type_VEC2:
                {
                    switch(stringBuffer[0])
                    {
                        case 'x':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec2>(varChainParent)->value.x));
                            break;
                        case 'y':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec2>(varChainParent)->value.y));
                            break;
                        default:
                            TuiParseError(debugInfo, "Invalid value");
                            break;
                    }
                }
                    break;
                case Tui_ref_type_VEC3:
                {
                    switch(stringBuffer[0])
                    {
                        case 'x':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec3>(varChainParent)->value.x));
                            break;
                        case 'y':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec3>(varChainParent)->value.y));
                            break;
                        case 'z':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec3>(varChainParent)->value.z));
                            break;
                        default:
                            TuiParseError(debugInfo, "Invalid value");
                            break;
                    }
                }
                    break;
                case Tui_ref_type_VEC4:
                {
                    switch(stringBuffer[0])
                    {
                        case 'x':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(varChainParent)->value.x));
                            break;
                        case 'y':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(varChainParent)->value.y));
                            break;
                        case 'z':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(varChainParent)->value.z));
                            break;
                        case 'w':
                            resultRef = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(varChainParent)->value.w));
                            break;
                        default:
                            TuiParseError(debugInfo, "Invalid value");
                            break;
                    }
                }
                    break;
            }
        }
        else
        {
            Tui::table searchTable = (varChainParent ? Tui::cast<TuiTable>(varChainParent) :  parent);
            
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
            }
        }
        
        if(resultRef)
        {
            if(isFunctionCall)
            {
                if(resultRef->type() != Tui_ref_type_FUNCTION)
                {
                    std::string extraInfo = "";
                    if(!debugVarChainParentName.empty())
                    {
                        extraInfo = " '" + debugVarChainParentName + "." + stringBuffer + "' is not a function";
                    }
                    TuiParseError(debugInfo, "Expected function '%s', found %s.%s", stringBuffer.c_str(), resultRef->getTypeName().c_str(), extraInfo.c_str());
                    return nullptr;
                }
                Tui::table argsArrayTable = Tui::create<TuiTable>(parent);//TuiTable::initWithHumanReadableString(s, endptr, parent, debugInfo);
                
                while(*s != ')')
                {
                    Tui::ref valueRef = TuiRef::loadExpression(s,
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
                Tui::ref newRef = Tui::cast<TuiFunction>(resultRef)->call(argsArrayTable, existingValue, nullptr, debugInfo);
                resultRef = newRef;
            }
            
            return resultRef;
        }
        else if(isFunctionCall)
        {
            std::string extraInfo = "";
            if(!debugVarChainParentName.empty())
            {
                if(!varChainParent)
                {
                    extraInfo = " '" + debugVarChainParentName + "' is nil";
                }
                else
                {
                    extraInfo = " '" + debugVarChainParentName + "' is valid, but '" + debugVarChainParentName + "." + stringBuffer + "' is nil";
                }
            }
            TuiParseError(debugInfo, "Attempt to call missing function '%s'.%s", stringBuffer.c_str(), extraInfo.c_str());
            return nullptr;
        }
        
        return nullptr;
    }
    
    if(!stringBuffer.empty() || foundAnyQuote)
    {
        Tui::string tuiString = Tui::create<TuiString>(stringBuffer);
        return tuiString;
    }
    
    return nullptr;
}

// parses a variable chain and returns the result eg: foo.bar().array[1+2].x
// optionally stores the enclosing ref and the final variable name if found
Tui::ref TuiRef::loadValue(const char* str,
                          char** endptr,
                          Tui::ref existingValue,
                          Tui::table parentTable,
                          TuiDebugInfo* debugInfo,
                          
                          //below are only passed if there is a chance we are finding a key in order to set its value, giving the caller quick access to the parent to set the value for an uninitialized variable
                          Tui::ref* onSetEnclosingRef,
                          std::string* onSetKey,
                          int* onSetIndex,
                          bool* accessedParentVariable)
{
    const char* s = str;
    
    Tui::ref varChainParent = nullptr;
    Tui::ref result = nullptr;
    
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
            TuiParseError(debugInfo, "Something went wrong");
        }
        
        Tui::table parentToUse = parentTable->parentTable;
        
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
        
        /*result = loadSingleValueInternal(s,
                                                 endptr,
                                                 nullptr,
                                                 parentToUse,
                                                 nullptr,
                                                 debugInfo,
                                                 
                                                 nullptr,
                                                 nullptr,
                                                 accessedParentVariable);*/
    }

    std::string debugVarChainParentName = "";
    while(1)
    {
        std::string stringBuffer;
        result = loadSingleValueInternal(s, endptr, existingValue, parentTable, varChainParent, debugInfo, onSetKey, onSetIndex, accessedParentVariable, stringBuffer, debugVarChainParentName);
        s = *endptr;
        
        if(*s == '.')
        {
            
            if(!result)
            {
                std::string extraInfo = " '";
                if(!debugVarChainParentName.empty())
                {
                    extraInfo += debugVarChainParentName + ".";
                }
                extraInfo += stringBuffer + "'";
                
                TuiParseError(debugInfo, "Attempt to use '.' to access from nil object%s", extraInfo.c_str());
                return nullptr;
            }
            varChainParent = result;
            debugVarChainParentName = stringBuffer;
            s++;
        }
        else if(*s == '[')
        {
            if(varChainParent)
            {
            } //todo else
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

Tui::boolean TuiRef::logicalNot(Tui::ref value)
{
    return TUI_BOOL(!value || !value->boolValue());
}


Tui::ref TuiRef::loadExpression(const char* str,
                               char** endptr,
                               Tui::ref existingValue,
                               Tui::ref leftValue,
                               Tui::table parentTable,
                               TuiDebugInfo* debugInfo,
                               int operatorLevel)
{
    const char* s = str;
    
    
    if(!leftValue)
    {
        if(*s == '!')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            Tui::ref rightValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parentTable, debugInfo, Tui_operator_level_not);
            if(rightValue)
            {
                leftValue = TuiRef::logicalNot(rightValue);
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
                TuiParseError(debugInfo, "Expected ')'");
                return nullptr;
            }
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
        }
        else if(*s == '-')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            Tui::ref rightValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parentTable, debugInfo, Tui_operator_level_not);
            if(rightValue)
            {
                switch (rightValue->type()) {
                    case Tui_ref_type_NUMBER:
                    {
                        leftValue = Tui::create<TuiNumber>(-(Tui::cast<TuiNumber>(rightValue)->value));
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        leftValue = Tui::create<TuiVec2>(-(Tui::cast<TuiVec2>(rightValue)->value));
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        leftValue = Tui::create<TuiVec3>(-(Tui::cast<TuiVec3>(rightValue)->value));
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        leftValue = Tui::create<TuiVec4>(-(Tui::cast<TuiVec4>(rightValue)->value));
                    }
                        break;
                        
                    default:
                        TuiParseError(debugInfo, "expected number or vector, got:%s", (rightValue ? rightValue->getDebugString().c_str() : "nil"));
                        break;
                }
            }
            else
            {
                TuiParseError(debugInfo, "expected number or vector, got:%s", (rightValue ? rightValue->getDebugString().c_str() : "nil"));
            }
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
        }
        else
        {
            leftValue = TuiRef::loadValue(s, endptr, existingValue, parentTable, debugInfo);
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
    
    Tui::ref result = nullptr;
    
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
                        (Tui::cast<TuiNumber>(leftValue)->value++);
                        result = TUI_NIL; //todo returns a nil ref, otherwise the key is addded to an array
                    }
                        break;
                    case '-':
                    {
                        (Tui::cast<TuiNumber>(leftValue)->value--);
                        result = TUI_NIL; //todo returns a nil ref, otherwise the key is addded to an array
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else
            {
                TuiParseError(debugInfo, "Invalid or unassigned value:%s for:%s", leftValue->getDebugString().c_str(), getVariableNameForDebug(str).c_str());
            }
            
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
    
    Tui::ref rightValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parentTable, debugInfo, newOperatorLevel);
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
                            Tui::cast<TuiNumber>(leftValue)->value += (Tui::cast<TuiNumber>(rightValue)->value);
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array //todo this is a waste of an allocate
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                Tui::cast<TuiNumber>(existingValue)->value = Tui::cast<TuiNumber>(leftValue)->value + (Tui::cast<TuiNumber>(rightValue)->value);
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftValue)->value + (Tui::cast<TuiNumber>(rightValue)->value));
                            }
                        }
                    }
                        break;
                    case '-':
                    {
                        if(secondOperatorChar == '=')
                        {
                            Tui::cast<TuiNumber>(leftValue)->value -= (Tui::cast<TuiNumber>(rightValue)->value);
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array //todo no need to allocate this
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                Tui::cast<TuiNumber>(existingValue)->value = Tui::cast<TuiNumber>(leftValue)->value - (Tui::cast<TuiNumber>(rightValue)->value);
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftValue)->value - (Tui::cast<TuiNumber>(rightValue)->value));
                            }
                        }
                    }
                        break;
                    case '*':
                    {
                        if(secondOperatorChar == '=')
                        {
                            Tui::cast<TuiNumber>(leftValue)->value *= (Tui::cast<TuiNumber>(rightValue)->value);
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                Tui::cast<TuiNumber>(existingValue)->value = Tui::cast<TuiNumber>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value);
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value));
                            }
                        }
                    }
                        break;
                    case '/':
                    {
                        if(secondOperatorChar == '=')
                        {
                            Tui::cast<TuiNumber>(leftValue)->value /= (Tui::cast<TuiNumber>(rightValue)->value);
                            result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array
                        }
                        else
                        {
                            if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                            {
                                Tui::cast<TuiNumber>(existingValue)->value = Tui::cast<TuiNumber>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value);
                                existingValueWasAssigned = true;
                            }
                            else
                            {
                                result = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value));
                            }
                        }
                    }
                        break;
                    case '%':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_NUMBER)
                        {
                            Tui::cast<TuiNumber>(existingValue)->value = ((int)Tui::cast<TuiNumber>(leftValue)->value) % ((int)(Tui::cast<TuiNumber>(rightValue)->value));
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiNumber>(((int)Tui::cast<TuiNumber>(leftValue)->value) % ((int)(Tui::cast<TuiNumber>(rightValue)->value)));
                        }
                    }
                        break;
                    case '>':
                    {
                        if(secondOperatorChar == '=')
                        {
                            result = TUI_BOOL(Tui::cast<TuiNumber>(leftValue)->value >= (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                        else
                        {
                            result = TUI_BOOL(Tui::cast<TuiNumber>(leftValue)->value > (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    case '<':
                    {
                        if(secondOperatorChar == '=')
                        {
                            result = TUI_BOOL(Tui::cast<TuiNumber>(leftValue)->value <= (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                        else
                        {
                            result = TUI_BOOL(Tui::cast<TuiNumber>(leftValue)->value < (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(rightValue)->value * (Tui::cast<TuiNumber>(leftValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(rightValue)->value * (Tui::cast<TuiNumber>(leftValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(rightValue)->value / (Tui::cast<TuiNumber>(leftValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(rightValue)->value / (Tui::cast<TuiNumber>(leftValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(rightValue)->value * (Tui::cast<TuiNumber>(leftValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(rightValue)->value * (Tui::cast<TuiNumber>(leftValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(rightValue)->value / (Tui::cast<TuiNumber>(leftValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(rightValue)->value / (Tui::cast<TuiNumber>(leftValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(rightValue)->value * (Tui::cast<TuiNumber>(leftValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(rightValue)->value * (Tui::cast<TuiNumber>(leftValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(rightValue)->value / (Tui::cast<TuiNumber>(leftValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(rightValue)->value / (Tui::cast<TuiNumber>(leftValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
                        break;
                }
            }
            else if(rightValue->type() == Tui_ref_type_STRING)
            {
                switch(operatorChar)
                {
                    case '+':
                    {
                        if(secondOperatorChar == '=')
                        {
                            TuiParseError(debugInfo, "number += string not supported");
                            break;
                        }
                        else
                        {
                            result = Tui::create<TuiString>(leftValue->getStringValue() + (Tui::cast<TuiString>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(leftValue)->value + (Tui::cast<TuiVec2>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftValue)->value + (Tui::cast<TuiVec2>(rightValue)->value));
                        }
                    }
                        break;
                    case '-':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(leftValue)->value - (Tui::cast<TuiVec2>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftValue)->value - (Tui::cast<TuiVec2>(rightValue)->value));
                        }
                    }
                        break;
                    case '*':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(leftValue)->value * (Tui::cast<TuiVec2>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftValue)->value * (Tui::cast<TuiVec2>(rightValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC2)
                        {
                            Tui::cast<TuiVec2>(existingValue)->value = Tui::cast<TuiVec2>(leftValue)->value / (Tui::cast<TuiVec2>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftValue)->value / (Tui::cast<TuiVec2>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(leftValue)->value + (Tui::cast<TuiVec3>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftValue)->value + (Tui::cast<TuiVec3>(rightValue)->value));
                        }
                    }
                        break;
                    case '-':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(leftValue)->value - (Tui::cast<TuiVec3>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftValue)->value - (Tui::cast<TuiVec3>(rightValue)->value));
                        }
                    }
                        break;
                    case '*':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(leftValue)->value * (Tui::cast<TuiVec3>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftValue)->value * (Tui::cast<TuiVec3>(rightValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC3)
                        {
                            Tui::cast<TuiVec3>(existingValue)->value = Tui::cast<TuiVec3>(leftValue)->value / (Tui::cast<TuiVec3>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftValue)->value / (Tui::cast<TuiVec3>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftValue)->value * (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftValue)->value / (Tui::cast<TuiNumber>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(leftValue)->value + (Tui::cast<TuiVec4>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftValue)->value + (Tui::cast<TuiVec4>(rightValue)->value));
                        }
                    }
                        break;
                    case '-':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(leftValue)->value - (Tui::cast<TuiVec4>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftValue)->value - (Tui::cast<TuiVec4>(rightValue)->value));
                        }
                    }
                        break;
                    case '*':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(leftValue)->value * (Tui::cast<TuiVec4>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftValue)->value * (Tui::cast<TuiVec4>(rightValue)->value));
                        }
                    }
                        break;
                    case '/':
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_VEC4)
                        {
                            Tui::cast<TuiVec4>(existingValue)->value = Tui::cast<TuiVec4>(leftValue)->value / (Tui::cast<TuiVec4>(rightValue)->value);
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftValue)->value / (Tui::cast<TuiVec4>(rightValue)->value));
                        }
                    }
                        break;
                    default:
                        TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
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
                        Tui::cast<TuiString>(leftValue)->value += rightValue->getStringValue();
                        result = TUI_NIL; //return a nil ref, otherwise the key is addded to an array //todo this is a waste of an allocate
                    }
                    else
                    {
                        if(existingValue && existingValue->type() == Tui_ref_type_STRING)
                        {
                            Tui::cast<TuiString>(existingValue)->value = Tui::cast<TuiString>(leftValue)->value + rightValue->getStringValue();
                            existingValueWasAssigned = true;
                        }
                        else
                        {
                            result = Tui::create<TuiString>(Tui::cast<TuiString>(leftValue)->value + rightValue->getStringValue());
                        }
                    }
                }
                    break;
                case '>':
                {
                    if(secondOperatorChar == '=')
                    {
                        result = TUI_BOOL(Tui::cast<TuiString>(leftValue)->value >= (Tui::cast<TuiString>(rightValue)->value));
                    }
                    else
                    {
                        result = TUI_BOOL(Tui::cast<TuiString>(leftValue)->value > (Tui::cast<TuiString>(rightValue)->value));
                    }
                }
                    break;
                case '<':
                {
                    if(secondOperatorChar == '=')
                    {
                        result = TUI_BOOL(Tui::cast<TuiString>(leftValue)->value <= (Tui::cast<TuiString>(rightValue)->value));
                    }
                    else
                    {
                        result = TUI_BOOL(Tui::cast<TuiString>(leftValue)->value < (Tui::cast<TuiString>(rightValue)->value));
                    }
                }
                    break;
                default:
                    TuiParseError(debugInfo, "Invalid operator:%c", operatorChar);
                    break;
            }
        }
        else
        {
            TuiParseError(debugInfo, "Invalid or unassigned value:%s for:%s", leftValue->getDebugString().c_str(), getVariableNameForDebug(str).c_str());
        }
    }
    
    if(result || existingValueWasAssigned)
    {
        
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
    
    TuiParseError(debugInfo, "Invalid expression");
    return nullptr;
}


/************************************************  TuiUserData methods *****************************************************/

TuiUserData::TuiUserData(void* value_)
: TuiRef()
{
    value = value_;
}
