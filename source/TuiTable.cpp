#include "TuiTable.h"

bool TuiTable::addHumanReadableKeyValuePair(const char* str, char** endptr, TuiDebugInfo* debugInfo, TuiRef** resultRef)
{
    const char* s = tuiSkipToNextChar(str, debugInfo);
    
    if(resultRef && *s == 'r'
       && *(s + 1) == 'e'
       && *(s + 2) == 't'
       && *(s + 3) == 'u'
       && *(s + 4) == 'r'
       && *(s + 5) == 'n')
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
            
            TuiRef* valueRef = TuiRef::loadExpression(s,
                                                      endptr,
                                                      nullptr,
                                                      nullptr,
                                                      this,
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
                valueRef->release();
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
        
        TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, this, &tokenMap, debugInfo, true, isWhileLoop);
        if(!statement)
        {
            return false;
        }
        s = tuiSkipToNextChar(*endptr, debugInfo, false);
        
        
        // the code below up until calling the statement is very similar to TuiFunction::call()
        // changes made here should probably be made there or it all could be factored out.
        TuiFunctionCallData callData;
        
        for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
        {
            if(tokenMap.refsByToken.count(varNameAndToken.second) != 0)
            {
                TuiRef* var = tokenMap.refsByToken[varNameAndToken.second];
                var->retain();
                callData.locals[varNameAndToken.second] = var;
            }
            else
            {
                TuiTable* parentTable = this;
                while(parentTable)
                {
                    if(parentTable->objectsByStringKey.count(varNameAndToken.first) != 0)
                    {
                        TuiRef* var = parentTable->objectsByStringKey[varNameAndToken.first];
                        var->retain();
                        callData.locals[varNameAndToken.second] = var;
                        break;
                    }
                    parentTable = parentTable->parentTable;
                }
            }
        }
        
        TuiRef* result = TuiFunction::runStatement(statement,  nullptr, parentTable, &tokenMap, &callData, debugInfo);
        delete statement;
        
        for(auto& tokenAndRef : callData.locals)
        {
            tokenAndRef.second->release();
        }
        
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
        
        TuiRef* expressionResult = TuiRef::loadExpression(s,
                                                          endptr,
                                                          nullptr, //existing
                                                          nullptr, //leftValue
                                                          this, //parent
                                                          debugInfo);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        if(expressionResult)
        {
            expressionPass = expressionResult->boolValue();
            expressionResult->release();
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
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "if statement expected '{'");
                return false;
            }
        }
        else
        {
            s = tuiSkipToNextMatchingChar(s, debugInfo, '}');
            s = tuiSkipToNextChar(s + 1, debugInfo);
        }
        
        
        while(1)
        {
            if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
            {
                if(ifStatementComplete)
                {
                    s = tuiSkipToNextMatchingChar(s, debugInfo, '}');
                    s = tuiSkipToNextChar(s + 1, debugInfo);
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
                        
                        TuiRef* expressionResult = TuiRef::loadExpression(s,
                                                                         endptr,
                                                                         nullptr, //existing
                                                                         nullptr, //leftValue
                                                                         this, //parent
                                                                         debugInfo);
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        if(expressionResult)
                        {
                            expressionPass = expressionResult->boolValue();
                            expressionResult->release();
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
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else if statement expected '{'");
                                return false;
                            }
                        }
                        else
                        {
                            s = tuiSkipToNextMatchingChar(s, debugInfo, '}');
                            s = tuiSkipToNextChar(s + 1, debugInfo);
                        }
                    }
                    else
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else statement expected 'if' or '{'");
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
    
    TuiRef* enclosingRef = nullptr;
    std::string finalKey = "";
    int finalIndex = -1; //todo this probably isn't actually used, wasn't needed?
    bool accessedParentVariable = false;
    
    //todo should this be released?
    TuiRef* existingObjectRef = loadValue(s,
                                          endptr,
                                          nullptr,
                                          this,
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
        
        TuiRef* valueRef = TuiRef::loadExpression(s,
                                                  endptr,
                                                  existingObjectRef,
                                                  nullptr,
                                                  this,
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
                    TuiRef* copyRef = valueRef->copy();
                    ((TuiTable*)enclosingRef)->set(finalKey, copyRef);
                    copyRef->release();
                }
                else if(finalIndex >= 0)
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    TuiRef* copyRef = valueRef->copy();
                    ((TuiTable*)enclosingRef)->replace(finalIndex, copyRef);
                    copyRef->release();
                }
                else if(existingObjectRef && existingObjectRef->type() == Tui_ref_type_TABLE && ((TuiTable*)existingObjectRef)->arrayObjects.size() == 1)
                {
                    TuiRef* arrayObject = ((TuiTable*)existingObjectRef)->arrayObjects[0];
                    if(arrayObject->type() == Tui_ref_type_NUMBER)
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        int indexToUse = ((TuiNumber*)arrayObject)->value;
                        TuiRef* copyRef = valueRef->copy();
                        ((TuiTable*)enclosingRef)->replace(indexToUse, copyRef);
                        copyRef->release();
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
                    ((TuiTable*)enclosingRef)->set(finalKey, valueRef);
                }
                else if(finalIndex >= 0)
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    ((TuiTable*)enclosingRef)->replace(finalIndex, valueRef);
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
                    ((TuiTable*)enclosingRef)->set(finalKey, nullptr);
                }
                else if(finalIndex >= 0)
                {
                    if(enclosingRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiError("Unimplemented");
                    }
                    ((TuiTable*)enclosingRef)->replace(finalIndex, valueRef);
                }
            }
        }
        else
        {
            TuiError("Something went wrong");
        }
        
        if(enclosingRef)
        {
            enclosingRef->release();
            enclosingRef = nullptr;
        }
        
        if(valueRef)
        {
            valueRef->release();
            valueRef = nullptr;
        }
        
        if(*s == ',' || *s == '\n')
        {
            if(*s == '\n')
            {
                debugInfo->lineNumber++;
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
           TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected ',' or newline after '=' or ':' assignment. unexpected character loading table:%c", *s);
            return false;
        }
        
        return true;
    }
    else
    {
        if(enclosingRef)
        {
            enclosingRef->release();
            enclosingRef = nullptr;
        }
        
        bool operatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
        bool operatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
        
        if(operatorOr || operatorAnd || *s == ',' || *s == '\n' || *s == '}' || *s == ']' || *s == ')' || TuiExpressionOperatorsSet.count(*s) != 0)
        {
            //todo memory problems in here
            TuiRef* leftValue = existingObjectRef;
            
            if(!leftValue && !finalKey.empty())
            {
                leftValue = new TuiString(finalKey);
            }
            
            TuiRef* valueRef = leftValue;
            if(valueRef)
            {
                valueRef = TuiRef::loadExpression(s,
                                                  endptr,
                                                  nullptr,
                                                  leftValue,
                                                  this,
                                                  debugInfo);
                
                if(!valueRef)
                {
                    valueRef = leftValue; //dubious
                }
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
            
            if(*s == '\n')
            {
                debugInfo->lineNumber++;
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
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected character loading table:%c", *s);
        }
    }
    
    return true;
}


TuiTable* TuiTable::initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef, TuiTable* inTable)
{
    TuiTable* table = inTable;
    
    if(!table)
    {
        table = new TuiTable(parent);
    }
    
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
    
    return table;
}

void TuiTable::printHumanReadableString(std::string& debugString, int indent)
{
    debugString += "{\n";
    indent = indent + 4;
    
    for(TuiRef* object : arrayObjects)
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        if(object) // it is valid to store a nullptr in arrayObjects
        {
            object->printHumanReadableString(debugString, indent);
        }
        else
        {
            debugString += "nil";
        }
        debugString += ",\n";
    }
    
    for(auto& kv : objectsByNumberKey)
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += Tui::string_format("%d = ", kv.first);
        kv.second->printHumanReadableString(debugString, indent);
        debugString += ",\n";
    }
    
    for(auto& kv : objectsByStringKey)
    {
        for(int i = 0; i < indent; i++)
        {
            debugString += " ";
        }
        debugString += kv.first + " = ";//"\"" + kv.first + "\" = "; //todo escape things correctly?
        kv.second->printHumanReadableString(debugString, indent);
        debugString += ",\n";
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
    
    
    for(TuiRef* object : arrayObjects)
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
    
        //todo
    //for(auto& kv : objectsByNumberKey)
    {
        
        //debugString += Tui::string_format("%d = ", kv.first);
        //kv.second->printHumanReadableString(debugString, indent);
    }
    //buffer[(*currentOffset)++] = Tui_binary_type_END_MARKER;
    
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
