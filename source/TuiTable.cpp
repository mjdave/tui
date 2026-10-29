#include "TuiTable.h"

void TuiTable::initWithConstructorString(const std::string& tableString)
{
    TuiDebugInfo debugInfo;
    TuiDebugInfoPush(&debugInfo, "table string construct", 1);
    const char* cString = tableString.c_str();
    char* endPtr;
    
    const char* s = tuiSkipToNextChar(cString, &debugInfo);
    
    bool foundOpeningBracket = false;
    
    if(*s == '{' || *s == '[')
    {
        foundOpeningBracket = true;
        s++;
    }
        
    while(1)
    {
        s = tuiSkipToNextChar(s, &debugInfo);
        
        if(*s == '\0')
        {
            break;
        }
        
        if(foundOpeningBracket && (*s == '}' || *s == ']'))
        {
            s++;
            endPtr = (char*)s;
            break;
        }
        
        if(!addHumanReadableKeyValuePair(s, &endPtr, &debugInfo, nullptr))
        {
            break;
        }
        s = endPtr;
    }
}

bool TuiTable::addHumanReadableKeyValuePair(const char* str, char** endptr, TuiDebugInfo* debugInfo, tui::ref* resultRef)
{
    const char* s = tuiSkipToNextChar(str, debugInfo);
    
    if(resultRef && *s == 'r'
       && *(s + 1) == 'e'
       && *(s + 2) == 't'
       && *(s + 3) == 'u'
       && *(s + 4) == 'r'
       && *(s + 5) == 'n' && checkSymbolNameComplete(s + 6))
    {
        s+=6;
        s = tuiSkipToNextChar(s, debugInfo, true);
        if(*s == '}' || *s == '\0' || *s == '\n')
        {
            if(resultRef)
            {
                *resultRef = TUI_NIL;
            }
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            return false;
        }
        else
        {
            
            tui::ref valueRef = TuiRef::loadExpression(s,
                                                      endptr,
                                                      nullptr,
                                                      nullptr,
                                                      createFromThis<TuiTable>(),
                                                      debugInfo);
            
            if(resultRef)
            {
                if(valueRef)
                {
                    *resultRef = valueRef;
                }
                else
                {
                    *resultRef = TUI_NIL;
                }
            }
            else
            {
            }
            
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            *endptr = (char*)s;
            return false;
        }
    }
    
    if((*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3)))) ||
       (*s == 'w' && *(s + 1) == 'h' && *(s + 2) == 'i' && *(s + 3) == 'l' && *(s + 4) == 'e' && (*(s + 5) == '(' || isspace(*(s + 5)))))
    {
        bool isWhileLoop = (*s == 'w');
        if(isWhileLoop)
        {
            s+=5;
        }
        else
        {
            s+=3;
        }
        s = tuiSkipToNextChar(s, debugInfo);
        
        TuiTokenMap tokenMap;
        
        TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, createFromThis<TuiTable>(), debugInfo, true, isWhileLoop);
        if(!statement)
        {
            return false;
        }
        s = tuiSkipToNextChar(*endptr, debugInfo, false);
        
        
        // the code below up until calling the statement is very similar to TuiFunction::call()
        // changes made here should probably be made there or it all could be factored out.
        TuiFunctionCallData callData;
        callData.thisTable = createFromThis<TuiTable>();
        
        for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
        {
            if(tokenMap.refsByToken.count(varNameAndToken.second) != 0)
            {
                tui::ref var = tokenMap.refsByToken[varNameAndToken.second];
                callData.locals[varNameAndToken.second] = var;
                callData.localTokensByStringKey[varNameAndToken.first] = varNameAndToken.second;
            }
            else
            {
                tui::table parentTable = createFromThis<TuiTable>();
                std::set<tui::table> found;
                while(parentTable)
                {
                    if(parentTable->objectsByStringKey.count(varNameAndToken.first) != 0)
                    {
                        tui::ref var = parentTable->objectsByStringKey[varNameAndToken.first];
                        callData.locals[varNameAndToken.second] = var;
                        callData.localTokensByStringKey[varNameAndToken.first] = varNameAndToken.second;
                        break;
                    }
                    parentTable = parentTable->parentTable;
                    if(found.count(parentTable) != 0)
                    {
                        break;
                    }
                    found.insert(parentTable);
                }
            }
        }
        
        tui::ref result = TuiFunction::runStatement(statement,  nullptr, createFromThis<TuiTable>(), &tokenMap, &callData, debugInfo);
        
        if(result)
        {
            *resultRef = result;
            *endptr = (char*)s;
            return false;
        }
        *endptr = (char*)s;
        return true;
    }
    
    if(*s == 'i' && *(s + 1) == 'f' && (*(s + 2) == '(' || isspace(*(s + 2))))
    {
        s+=2;
        s = tuiSkipToNextChar(s, debugInfo);
        
        bool expressionPass = true;
        
        tui::ref expressionResult = TuiRef::loadExpression(s,
                                                          endptr,
                                                          nullptr, //existing
                                                          nullptr, //leftValue
                                                          createFromThis<TuiTable>(), //parent
                                                          debugInfo);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        if(expressionResult)
        {
            expressionPass = expressionResult->boolValue();
        }
        else
        {
            expressionPass = false;
        }
        
        bool ifStatementComplete = false;
        
        if(expressionPass)
        {
            s = tuiSkipToNextChar(s, debugInfo);
            if(*s == '{')
            {
                s++;
                ifStatementComplete = true;
                
                while(1)
                {
                    s = tuiSkipToNextChar(s, debugInfo);
                    
                    if(*s == '}' || *s == ']' || *s == ')')
                    {
                        s = tuiSkipToNextChar(s + 1, debugInfo);
                        *endptr = (char*)s;
                        break;
                    }
                    
                    if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                    {
                        //s = *endptr;
                       // break;
                        return false;
                    }
                    s = *endptr;
                }
            }
            else
            {
                TuiParseError(debugInfo, "if statement expected '{'");
                return false;
            }
        }
        else
        {
            s = tuiSkipToAfterNextClosingBrace(s, debugInfo, false);
        }
        
        
        while(1)
        {
            if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
            {
                if(ifStatementComplete)
                {
                    s = tuiSkipToAfterNextClosingBrace(s, debugInfo, false);
                }
                else
                {
                    s+=4;
                    s = tuiSkipToNextChar(s, debugInfo);
                    if(*s == '{')
                    {
                        s++;
                        ifStatementComplete = true;
                        
                        while(1)
                        {
                            s = tuiSkipToNextChar(s, debugInfo);
                            
                            if(*s == '}' || *s == ']' || *s == ')')
                            {
                                s = tuiSkipToNextChar(s + 1, debugInfo);
                                *endptr = (char*)s;
                                break;
                            }
                            
                            if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                            {
                                return false;
                            }
                            s = *endptr;
                        }
                        
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        break;
                    }
                    else if(*s == 'i' && *(s + 1) == 'f')
                    {
                        s+=2;
                        s = tuiSkipToNextChar(s, debugInfo);
                        
                        bool expressionPass = true;
                        
                        tui::ref expressionResult = TuiRef::loadExpression(s,
                                                                         endptr,
                                                                         nullptr, //existing
                                                                         nullptr, //leftValue
                                                                         createFromThis<TuiTable>(), //parent
                                                                         debugInfo);
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        if(expressionResult)
                        {
                            expressionPass = expressionResult->boolValue();
                        }
                        else
                        {
                            expressionPass = false;
                        }
                        
                        
                        if(expressionPass)
                        {
                            s = tuiSkipToNextChar(s, debugInfo);
                            if(*s == '{')
                            {
                                s++;
                                ifStatementComplete = true;
                                
                                while(1)
                                {
                                    s = tuiSkipToNextChar(s, debugInfo);
                                    
                                    if(*s == '}' || *s == ']' || *s == ')')
                                    {
                                        s = tuiSkipToNextChar(s + 1, debugInfo);
                                        *endptr = (char*)s;
                                        break;
                                    }
                                    
                                    if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                                    {
                                        return false;
                                    }
                                    s = *endptr;
                                }
                                
                                s = tuiSkipToNextChar(*endptr, debugInfo);
                            }
                            else
                            {
                                TuiParseError(debugInfo, "else if statement expected '{'");
                                return false;
                            }
                        }
                        else
                        {
                            s = tuiSkipToAfterNextClosingBrace(s, debugInfo, false);
                        }
                    }
                    else
                    {
                        TuiParseError(debugInfo, "else statement expected 'if' or '{'");
                        return false;
                    }
                }
            }
            else
            {
                break;
            }
        }
        *endptr = (char*)s;
        return true; //return now, we are done
    }
    
    tui::ref enclosingRef = nullptr;
    std::string finalKey = "";
    int finalIndex = -1; //todo this probably isn't actually used, wasn't needed?
    bool accessedParentVariable = false;
    
    //todo should this be released?
    tui::ref existingObjectRef = loadValue(s,
                                          endptr,
                                          nullptr,
                                          createFromThis<TuiTable>(),
                                          debugInfo,
                                          &enclosingRef,
                                          &finalKey,
                                          &finalIndex,
                                          &accessedParentVariable);
    
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    if((*s == '=' && *(s + 1) != '=') || *s == ':')
    {
        s++;
        s = tuiSkipToNextChar(s, debugInfo);
        
        if(accessedParentVariable)
        {
            existingObjectRef = nullptr;
        }
        
        tui::ref valueRef = TuiRef::loadExpression(s,
                                                  endptr,
                                                  existingObjectRef,
                                                  nullptr,
                                                  createFromThis<TuiTable>(),
                                                  debugInfo);
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        // set value if valueRef isn't nil, setting enclosing->finalKey to nil if needed
        if(enclosingRef) //watch out, all this stuff might not be called on a successful copy into the existing keyRef
        {
            if(valueRef)
            {
                if(!finalKey.empty())
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    tui::ref copyRef = valueRef->copy();
                    tui::cast<TuiTable>(enclosingRef)->set(finalKey, copyRef);
                }
                else if(finalIndex >= 0)
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    tui::ref copyRef = valueRef->copy();
                    tui::cast<TuiTable>(enclosingRef)->replace(finalIndex, copyRef);
                }
                else if(existingObjectRef && existingObjectRef->type() == Tui_ref_type_TABLE && tui::cast<TuiTable>(existingObjectRef)->arrayObjects.size() == 1)
                {
                    tui::ref arrayObject = (tui::cast<TuiTable>(existingObjectRef)->arrayObjects[0]);
                    if(arrayObject->type() == Tui_ref_type_NUMBER)
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        int indexToUse = (tui::cast<TuiNumber>(arrayObject)->value);
                        tui::ref copyRef = valueRef->copy();
                        tui::cast<TuiTable>(enclosingRef)->replace(indexToUse, copyRef);
                    }
                }
            }
        }
        else if(existingObjectRef)
        {
            if(valueRef && valueRef->type() != Tui_ref_type_NIL)
            {
                if(!finalKey.empty())
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    tui::cast<TuiTable>(enclosingRef)->set(finalKey, valueRef);
                }
                else if(finalIndex >= 0)
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    tui::cast<TuiTable>(enclosingRef)->replace(finalIndex, valueRef);
                }
            }
            else
            {
                if(!finalKey.empty())
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    tui::cast<TuiTable>(enclosingRef)->set(finalKey, nullptr);
                }
                else if(finalIndex >= 0)
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    tui::cast<TuiTable>(enclosingRef)->replace(finalIndex, valueRef);
                }
            }
        }
        else
        {
            TuiError("Something went wrong");
        }
        
        if(enclosingRef)
        {
            enclosingRef = nullptr;
        }
        
        if(valueRef)
        {
            valueRef = nullptr;
        }
        
        if(*s == ',' || *s == '\n')
        {
            if(*s == '\n')
            {
                debugInfo->currentLine->lineNumber++;
            }
            s++;
            *endptr = (char*)s;
            return true;
        }
        else if(*s == '}' || *s == ']')
        {
            s++;
            *endptr = (char*)s;
            return false;
        }
        else if(*s != '\0')
        {
           TuiParseError(debugInfo, "Expected ',' or newline after '=' or ':' assignment. unexpected character loading table:%c", *s);
            return false;
        }
        
        return true;
    }
    else
    {
        if(enclosingRef)
        {
            enclosingRef = nullptr;
        }
        
        bool operatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
        bool operatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
        
        if(*s == '\0' || operatorOr || operatorAnd || *s == ',' || *s == '\n' || *s == '}' || *s == ']' || *s == ')' || TuiExpressionOperatorsSet.count(*s) != 0)
        {
            //todo memory problems in here
            tui::ref leftValue = existingObjectRef;
            
            if(!leftValue && !finalKey.empty())
            {
                leftValue = tui::create<TuiString>(finalKey);
            }
            
            tui::ref valueRef = leftValue;
            if(valueRef)
            {
                valueRef = TuiRef::loadExpression(s,
                                                  endptr,
                                                  nullptr,
                                                  leftValue,
                                                  createFromThis<TuiTable>(),
                                                  debugInfo);
                
                if(!valueRef)
                {
                    valueRef = leftValue; //dubious
                }
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
            
            if(*s == '\n')
            {
                debugInfo->currentLine->lineNumber++;
            }
            
            if(valueRef)
            {
                arrayObjects.push_back(valueRef);
            }
            
            if(*s == '\0')
            {
                *endptr = (char*)s;
                return false;
            }
            
            if(*s == '}' || *s == ']' || *s == ')') // ')' is added here to terminate function arg lists, but '(' is not valid generally.
            {
                s++;
                *endptr = (char*)s;
                return false;
            }
            
            s++;
            *endptr = (char*)s;
        }
        else if(*s != '\0')
        {
            TuiParseError(debugInfo, "unexpected character loading table:%c", *s);
        }
    }
    
    return true;
}


void TuiTable::printSingleSubObject(std::string& debugString, int indent, const tui::ref& object)
{
    if(indent > 128)
    {
        debugString += "[found likely circular loop, supressing log output]";
    }
    else
    {
        object->printHumanReadableString(debugString, indent);
    }
}

void TuiTable::printHumanReadableString(std::string& debugString, int indent)
{
    debugString += "{\n";
    indent = indent + 4;
    
    if(!set8.empty())
    {
        //note this syntax is probably final. set8-64 are not yet loaded (unimplimented) when you read back, could be done easily enough. Fully supported in binary.
        /*
         example:{
             :8 = {1,4}
         }
         */
        
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += ":8 = {";
        bool first = true;
        for(uint8_t v : set8)
        {
            if(!first)
            {
                debugString+=",";
            }
            first = false;
            debugString += tui::string_format("%u", v);
        }
        debugString += "}\n";
    }
    
    if(!set16.empty())
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += ":16 = {";
        bool first = true;
        for(uint16_t v : set16)
        {
            if(!first)
            {
                debugString+=",";
            }
            first = false;
            debugString += tui::string_format("%u", v);
        }
        debugString += "}\n";
    }
    
    if(!set32.empty())
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += ":32 = {";
        bool first = true;
        for(uint32_t v : set32)
        {
            if(!first)
            {
                debugString+=",";
            }
            first = false;
            debugString += tui::string_format("%u", v);
        }
        debugString += "}\n";
    }
    
    if(!set64.empty())
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += ":64 = {";
        bool first = true;
        for(uint64_t v : set64)
        {
            if(!first)
            {
                debugString+=",";
            }
            first = false;
            debugString += tui::string_format("%llu", v);
        }
        debugString += "}\n";
    }
    
    
    for(tui::ref object : arrayObjects)
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        if(object) // it is valid to store a nullptr in arrayObjects
        {
            printSingleSubObject(debugString, indent, object);
        }
        else
        {
            debugString += "nil";
        }
        debugString += "\n";
    }
    
    for(auto& kv : objectsByNumberKey)
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += tui::string_format("%d = ", kv.first);
        printSingleSubObject(debugString, indent, kv.second);
        debugString += "\n";
    }
    
    for(auto& kv : objectsByStringKey)
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += kv.first + " = ";//"\"" + kv.first + "\" = "; //todo escape things correctly?
        printSingleSubObject(debugString, indent, kv.second);
        debugString += "\n";
    }
    
    for(int i = 0; i < indent-4; i++)
    {
        debugString += " ";
    }
    
    debugString += "}";
}

void TuiTable::serializeBinaryToBuffer(std::string& buffer, int* currentOffset)
{
    resizeBufferIfNeeded(buffer, currentOffset, 1);
    buffer[(*currentOffset)++] = Tui_binary_type_TABLE;
    
    if(!set8.empty())
    {
        resizeBufferIfNeeded(buffer, currentOffset, 2 + sizeof(uint8_t) * (int)set8.size());
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_8_SET;
        for(const uint8_t& v : set8)
        {
            memcpy(&buffer[(*currentOffset)], &v, sizeof(uint8_t));
            *currentOffset += sizeof(uint8_t);
        }
        buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
    }
    
    if(!set16.empty())
    {
        resizeBufferIfNeeded(buffer, currentOffset, 2 + sizeof(uint16_t) * (int)set16.size());
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_16_SET;
        for(const uint16_t& v : set16)
        {
            memcpy(&buffer[(*currentOffset)], &v, sizeof(uint16_t));
            *currentOffset += sizeof(uint16_t);
        }
        buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
    }
    
    if(!set32.empty())
    {
        resizeBufferIfNeeded(buffer, currentOffset, 2 + sizeof(uint32_t) * (int)set32.size());
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_32_SET;
        for(const uint32_t& v : set32)
        {
            memcpy(&buffer[(*currentOffset)], &v, sizeof(uint32_t));
            *currentOffset += sizeof(uint32_t);
        }
        buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
    }
    
    if(!set64.empty())
    {
        resizeBufferIfNeeded(buffer, currentOffset, 2 + sizeof(uint64_t) * (int)set64.size());
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_64_SET;
        for(const uint64_t& v : set64)
        {
            memcpy(&buffer[(*currentOffset)], &v, sizeof(uint64_t));
            *currentOffset += sizeof(uint64_t);
        }
        buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
    }
    
    for(tui::ref object : arrayObjects)
    {
        if(object)
        {
            object->serializeBinaryToBuffer(buffer, currentOffset);
        }
        else
        {
            resizeBufferIfNeeded(buffer, currentOffset, 1);
            buffer[(*currentOffset)++] = Tui_binary_type_NIL;
        }
    }
    resizeBufferIfNeeded(buffer, currentOffset, 1);
    buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
    
    for(auto& kv : objectsByNumberKey)
    {
        const uint32_t& keyInt = kv.first;
        
        resizeBufferIfNeeded(buffer, currentOffset, 1 + 4);
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_32;
        memcpy(&buffer[(*currentOffset)], &keyInt, 4);
        *currentOffset += 4;
        
        kv.second->serializeBinaryToBuffer(buffer, currentOffset);
    }
    //buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER; //we can just check the type byte for int or string key
    
    for(auto& kv : objectsByStringKey)
    {
        const std::string& keyString = kv.first;
        
        resizeBufferIfNeeded(buffer, currentOffset, 5 + (int)keyString.size());
        buffer[(*currentOffset)++] = Tui_binary_type_STRING;
        uint32_t stringLength = (uint32_t)keyString.size();
        memcpy(&buffer[(*currentOffset)], &stringLength, 4);
        (*currentOffset)+=4;
        memcpy(&buffer[(*currentOffset)], keyString.c_str(), keyString.size());
        *currentOffset += keyString.size();
        
        kv.second->serializeBinaryToBuffer(buffer, currentOffset);
    }
    resizeBufferIfNeeded(buffer, currentOffset, 1);
    buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
}
