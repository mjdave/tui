#include "TuiFunction.h"

#include "TuiTable.h"
#include "TuiRef.h"


bool TuiFunction::recursivelySerializeExpression(const char* str,
                                                 char** endptr,
                                                 TuiExpression* expression,
                                                 TuiTable* parent,
                                                 TuiTokenMap* tokenMap,
                                                 TuiDebugInfo* debugInfo,
                                                 int operatorLevel,
                                                 uint32_t subExpressionTokenStartPos)
{
    const char* s = str;
    
    if(*s == '\0' || *s == '\n' || *s == ',' || *s == '}')
    {
        return true; //complete
    }
    
    uint32_t tokenStartPos;
    
    if(subExpressionTokenStartPos != UINT32_MAX)
    {
        tokenStartPos = subExpressionTokenStartPos;
    }
    else
    {
        tokenStartPos = (int)expression->tokens.size();
        
        if(*s == '!')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            expression->tokens.push_back(Tui_token_not);
            recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_not);
            expression->tokens.push_back(Tui_token_end);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
        }
        else if(*s == '(')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            s++; //')'
            s = tuiSkipToNextChar(s, debugInfo, true);
        }
        else
        {
            TuiRef* leftValue = TuiRef::initUnknownTypeRefWithHumanReadableString(s, endptr, parent, debugInfo);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
            if(leftValue->type() == Tui_ref_type_STRING)
            {
                TuiString* varString = ((TuiString*)leftValue);
                if(varString->allowAsVariableName)
                {
                    uint32_t leftVarToken = 0;
                    
                    if(tokenMap->tokensByVarNames.count(varString->value) != 0)
                    {
                        leftVarToken = tokenMap->tokensByVarNames[varString->value];
                    }
                    else
                    {
                        leftVarToken = tokenMap->tokenIndex++;
                        tokenMap->tokensByVarNames[varString->value] = leftVarToken;
                        std::map<uint32_t, TuiRef*> locals;
                        TuiRef* newValueRef = parent->recursivelyFindVariable(varString, true, parent, tokenMap, &locals, debugInfo);
                        if(newValueRef)
                        {
                            tokenMap->refsByToken[leftVarToken] = newValueRef;
                        }
                    }
                    
                    expression->tokens.push_back(leftVarToken);
                    
                    if(varString->isValidFunctionString && *s =='(')
                    {
                        expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_functionCall);
                        s++;
                        
                        while(*s != ')' && *s != '\0')
                        {
                            if(*s == ',')
                            {
                                s++;
                            }
                            recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                            s = tuiSkipToNextChar(*endptr, debugInfo, true);
                        }
                        s++;
                        
                        expression->tokens.push_back(Tui_token_end);
                        s = tuiSkipToNextChar(s, debugInfo, true);
                    }
                    
                }
                else
                {
                    uint32_t leftVarToken = tokenMap->tokenIndex++;
                    tokenMap->refsByToken[leftVarToken] = leftValue;
                    expression->tokens.push_back(leftVarToken);
                }
            }
            else
            {
                if(leftValue->type() != Tui_ref_type_NIL)
                {
                    uint32_t leftVarToken = tokenMap->tokenIndex++;
                    tokenMap->refsByToken[leftVarToken] = leftValue;
                    expression->tokens.push_back(leftVarToken);
                }
                else
                {
                    expression->tokens.push_back(Tui_token_nil);
                }
            }
        }
    }
        
    char operatorChar = *s;
    char secondOperatorChar = *(s + 1);
    
    bool operatorOr = (operatorChar == 'o' && secondOperatorChar == 'r' && checkSymbolNameComplete(s + 2));
    bool operatorAnd = (operatorChar == 'a' && secondOperatorChar == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
    
    if(operatorOr || operatorAnd)
    {
        if(Tui_operator_level_and_or <= operatorLevel)
        {
            return false; //not complete
        }
    }
    else
    {
        if(TuiExpressionOperatorsSet.count(operatorChar) == 0 ||
           (operatorChar == '=' && secondOperatorChar != '='))
        {
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return true; //complete
        }
        
        if(TuiExpressionOperatorsToLevelMap[operatorChar] <= operatorLevel)
        {
            return false; //not complete
        }
    }
    
    s++;
    
    if(secondOperatorChar == '=' && (operatorChar == '=' || operatorChar == '>' || operatorChar == '<' || operatorChar == '!') )
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
    
    if((operatorChar == '+' && (secondOperatorChar == '+' || secondOperatorChar == '=')) ||
       (operatorChar == '-' && (secondOperatorChar == '-' || secondOperatorChar == '=')) ||
       (operatorChar == '*' && secondOperatorChar == '=') ||
       (operatorChar == '/' && secondOperatorChar == '='))
    {
        s++;
    }
    
    s = tuiSkipToNextChar(s, debugInfo, true);
    
    int newOperatorLevel = operatorLevel;
    
    if(operatorOr)
    {
        newOperatorLevel = Tui_operator_level_and_or;
        expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_or);
    }
    else if(operatorAnd)
    {
        newOperatorLevel = Tui_operator_level_and_or;
        expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_and);
    }
    else
    {
        newOperatorLevel = TuiExpressionOperatorsToLevelMap[operatorChar];
        switch(operatorChar)
        {
            case '+':
            {
                if(secondOperatorChar == '+')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_increment);
                }
                else if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_addInPlace);
                }
                else
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_add);
                }
            }
                break;
            case '-':
            {
                if(secondOperatorChar == '-')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_decrement);
                }
                else if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_subtractInPlace);
                }
                else
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_subtract);
                }
            }
                break;
            case '*':
            {
                if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_multiplyInPlace);
                }
                else
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_multiply);
                }
            }
                break;
            case '/':
            {
                if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_divideInPlace);
                }
                else
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_divide);
                }
            }
                break;
            case '>':
            {
                if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_greaterEqualTo);
                }
                else
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_greaterThan);
                }
            }
                break;
            case '<':
            {
                if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_lessEqualTo);
                }
                else
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_lessThan);
                }
            }
                break;
            case '=':
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_equalTo);
            }
                break;
            case '!':
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_notEqualTo);
            }
                break;
            default:
                break;
        }
    }
    
    s = tuiSkipToNextChar(s, debugInfo, true);
    
    
    
    *endptr = (char*)s;
    bool complete = recursivelySerializeExpression(s,
                                   endptr,
                                   expression,
                                   parent,
                                   tokenMap,
                                   debugInfo,
                                   newOperatorLevel);
    s = tuiSkipToNextChar(*endptr, debugInfo, true);
    
    if(!complete)
    {
        *endptr = (char*)s;
        complete = recursivelySerializeExpression(s,
                                                  endptr,
                                                  expression,
                                                  parent,
                                                  tokenMap,
                                                  debugInfo,
                                                  operatorLevel,
                                                  tokenStartPos);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
    }
    
    *endptr = (char*)s;
    
    return complete;
}

TuiForStatement* TuiFunction::serializeForStatement(const char* str, char** endptr, TuiTable* parent,  TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo) //entry point is after 'for'
{
    const char* s = str;
    if(*s == '(')
    {
        s++;
        s = tuiSkipToNextChar(s, debugInfo);
    }
    
    TuiForStatement* forStatement = new TuiForStatement();
    forStatement->lineNumber = debugInfo->lineNumber;
    forStatement->expression = new TuiExpression();
    
    TuiString* varNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo, Tui_variable_load_type_serializeExpressions, tokenMap);
    s = tuiSkipToNextChar(*endptr, debugInfo);
    
    if(varNameRef && varNameRef->allowAsVariableName)
    {
        forStatement->varName = varNameRef;
        
        if(tokenMap->tokensByVarNames.count(varNameRef->value) != 0)
        {
            forStatement->varToken = tokenMap->tokensByVarNames[varNameRef->value];
        }
        else
        {
            forStatement->varToken = tokenMap->tokenIndex++;
            tokenMap->tokensByVarNames[varNameRef->value] = forStatement->varToken;
        }
        
        if(*s == '=')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            
            recursivelySerializeExpression(s, endptr, forStatement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
            forStatement->continueExpression = new TuiExpression();
        }
        else
        {
            if(*s == ',')
            {
                forStatement->indexOrKeyName = forStatement->varName; //index is first so we need to switch: for(index,object in table)
                forStatement->indexOrKeyToken = forStatement->varToken;
                
                s++;
                s = tuiSkipToNextChar(s, debugInfo);
                
                TuiString* newValueNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo, Tui_variable_load_type_serializeExpressions, tokenMap);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                forStatement->varName = newValueNameRef;
                varNameRef = forStatement->varName;
                
                if(tokenMap->tokensByVarNames.count(newValueNameRef->value) != 0)
                {
                    forStatement->varToken = tokenMap->tokensByVarNames[newValueNameRef->value];
                }
                else
                {
                    forStatement->varToken = tokenMap->tokenIndex++;
                    tokenMap->tokensByVarNames[newValueNameRef->value] = forStatement->varToken;
                }
            }
            
            if(*s == ':' || (*s == 'i' && (*(s + 1) == 'n')))
            {
                if(*s == ':')
                {
                    s++;
                }
                else
                {
                    s+=2;
                }
                
                s = tuiSkipToNextChar(s, debugInfo);
                
                recursivelySerializeExpression(s, endptr, forStatement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                s = tuiSkipToNextChar(*endptr, debugInfo);
            }
            else
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' or ',' or ':' or 'in' after:%s", varNameRef->getDebugString().c_str());
                varNameRef->release();
                return nullptr;
            }
        }
    }
    else
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected variable assignment in for loop");
        return nullptr;
    }
    
    if(forStatement->continueExpression) //only allocated if this is of the format (i=0,i<n;i++)
    {
        if(*s == ',' || *s == '\n')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
        }
        else
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or '\n'");
            return nullptr;
        }
        
        recursivelySerializeExpression(s, endptr, forStatement->continueExpression, parent, tokenMap, debugInfo, Tui_operator_level_default);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        
        if(*s == ',' || *s == '\n')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
        }
        else
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or '\n'");
            return nullptr;
        }
        
        //todo this is c/p from below. Should be a factored out
        const char* varNameStartS = s; //rewind if we find a function
        int keyStartLineNumber = debugInfo->lineNumber;
        varNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo, Tui_variable_load_type_serializeExpressions, tokenMap);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        
        if(varNameRef)
        {
            if(varNameRef->isValidFunctionString)
            {
                TuiStatement* incrementStatement = new TuiStatement(Tui_statement_type_FUNCTION_CALL);
                forStatement->incrementStatement = incrementStatement;
                incrementStatement->lineNumber = debugInfo->lineNumber;
                incrementStatement->expression = new TuiExpression();
                
                debugInfo->lineNumber = keyStartLineNumber;
                recursivelySerializeExpression(varNameStartS, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                if(*s == ')')
                {
                    s++;
                    s = tuiSkipToNextChar(s, debugInfo, true);
                }
                
                varNameRef->release();
            }
            else if(varNameRef->allowAsVariableName)
            {
                if(TuiExpressionOperatorsSet.count(*s) != 0)
                {
                    
                    TuiStatement* incrementStatement = new TuiStatement(Tui_statement_type_VAR_ASSIGN);
                    forStatement->incrementStatement = incrementStatement;
                    incrementStatement->lineNumber = debugInfo->lineNumber;
                    incrementStatement->varName = varNameRef;
                    incrementStatement->expression = new TuiExpression();
                    
                    if(*s == '=')
                    {
                        s++;
                        s = tuiSkipToNextChar(s, debugInfo);
                        recursivelySerializeExpression(s, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    }
                    else
                    {
                        debugInfo->lineNumber = keyStartLineNumber;
                        recursivelySerializeExpression(varNameStartS, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    }
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    
                    
                    if(tokenMap->tokensByVarNames.count(varNameRef->value) != 0)
                    {
                        incrementStatement->varToken = tokenMap->tokensByVarNames[varNameRef->value];
                    }
                    else
                    {
                        incrementStatement->varToken = tokenMap->tokenIndex++;
                        tokenMap->tokensByVarNames[varNameRef->value] = incrementStatement->varToken;
                    }
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                    varNameRef->release();
                    return nullptr;
                }
            }
        }
    }
    
    if(*s == ')')
    {
        s++;
    }
    s = tuiSkipToNextChar(s, debugInfo);
    
    bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &forStatement->statements);
    if(!success)
    {
        return nullptr;
    }
    
    s = tuiSkipToNextChar(*endptr, debugInfo);
    
    return forStatement;
}

bool TuiFunction::serializeFunctionBody(const char* str, char** endptr, TuiTable* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, std::vector<TuiStatement*>* statements)
{
    const char* s = str;
    if(*s != '{')
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Function expected opening brace");
        return false;
    }
    s++;
    s = tuiSkipToNextChar(s, debugInfo);
    
    while(1)
    {
        s = tuiSkipToNextChar(s, debugInfo);
        if(*s == '}')
        {
            s++;
            break;
        }
        
        if(*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3))))
        {
            s+=3;
            s = tuiSkipToNextChar(s, debugInfo);
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, parent, tokenMap, debugInfo);
            if(!statement)
            {
                return false;
            }
            statements->push_back(statement);
            s = tuiSkipToNextChar(*endptr, debugInfo, false);
            continue;
        }
        
        if(*s == 'i' && *(s + 1) == 'f' && (*(s + 2) == '(' || isspace(*(s + 2))))
        {
            s+=2;
            s = tuiSkipToNextChar(s, debugInfo);
            
            TuiIfStatement* statement = new TuiIfStatement();
            statement->lineNumber = debugInfo->lineNumber;
            statement->expression = new TuiExpression();
            
            recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            if(*s == ')')
            {
                s++;
                s = tuiSkipToNextChar(s, debugInfo);
            }
            
            bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &statement->statements);
            if(!success)
            {
                return false;
            }
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            
            TuiIfStatement* currentStatement = statement;
            while(1)
            {
                if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
                {
                    s+=4;
                    s = tuiSkipToNextChar(s, debugInfo);
                    if(*s == '{')
                    {
                        currentStatement->elseIfStatement = new TuiIfStatement();
                        currentStatement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        
                        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &currentStatement->elseIfStatement->statements);
                        if(!success)
                        {
                            return false;
                        }
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                    }
                    else if(*s == 'i' && *(s + 1) == 'f')
                    {
                        s+=2;
                        s = tuiSkipToNextChar(s, debugInfo);
                        
                        currentStatement->elseIfStatement = new TuiIfStatement();
                        currentStatement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        currentStatement->elseIfStatement->expression = new TuiExpression();
                        
                        recursivelySerializeExpression(s, endptr, currentStatement->elseIfStatement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                        
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        
                        if(*s == ')')
                        {
                            s++;
                            s = tuiSkipToNextChar(s, debugInfo);
                        }
                        
                        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &currentStatement->elseIfStatement->statements);
                        if(!success)
                        {
                            return false;
                        }
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        currentStatement = currentStatement->elseIfStatement;
                    }
                    else
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else statement expected 'if' or '{'");
                        return false;
                    }
                }
                else
                {
                    break;
                }
            }
            
            
            statements->push_back(statement);
            continue;
        }
        else if(*s == 'r'
           && *(s + 1) == 'e'
           && *(s + 2) == 't'
           && *(s + 3) == 'u'
           && *(s + 4) == 'r'
           && *(s + 5) == 'n')
        {
            s+=6;
            s = tuiSkipToNextChar(s, debugInfo);
            if(*s == '}')
            {
                TuiStatement* statement = new TuiStatement(Tui_statement_type_RETURN);
                statements->push_back(statement);
                s++;
                s = tuiSkipToNextChar(s, debugInfo, true);
                break;
            }
            else
            {
                TuiStatement* statement = new TuiStatement(Tui_statement_type_RETURN_EXPRESSION);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = new TuiExpression();
                
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        
        if(*s == '}')
        {
            s++;
            break;
        }
        
        TuiString* varNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo, Tui_variable_load_type_serializeExpressions, tokenMap);
        
        if(varNameRef)
        {
            if(varNameRef->isValidFunctionString)
            {
                TuiStatement* statement = new TuiStatement(Tui_statement_type_FUNCTION_CALL);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = new TuiExpression();
                
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                if(*s == ')')
                {
                    s++;
                    s = tuiSkipToNextChar(s, debugInfo, true);
                }
                
                statements->push_back(statement);
                varNameRef->release();
            }
            else if(varNameRef->allowAsVariableName)
            {
                const char* varStartS = s;
                int keyStartLineNumber = debugInfo->lineNumber;
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(*s == '}')
                {
                    s++;
                    break;
                }
                
                if(TuiExpressionOperatorsSet.count(*s) != 0)
                {
                    TuiStatement* statement = new TuiStatement(Tui_statement_type_VAR_ASSIGN);
                    statement->lineNumber = debugInfo->lineNumber;
                    statement->varName = varNameRef;
                    
                    
                    statement->expression = new TuiExpression();
                    if(*s == '=')
                    {
                        s++;
                        s = tuiSkipToNextChar(s, debugInfo);
                        recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    }
                    else
                    {
                        debugInfo->lineNumber = keyStartLineNumber;
                        recursivelySerializeExpression(varStartS, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    }
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo);
                    
                    
                    if(tokenMap->tokensByVarNames.count(varNameRef->value) != 0)
                    {
                        statement->varToken = tokenMap->tokensByVarNames[varNameRef->value];
                    }
                    else
                    {
                        statement->varToken = tokenMap->tokenIndex++;
                        tokenMap->tokensByVarNames[varNameRef->value] = statement->varToken;
                    }
                    statements->push_back(statement);
                    
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                    varNameRef->release();
                    return false;
                }
            }
        }
    }
    
    s = tuiSkipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
    
    return true;
}

TuiFunction* TuiFunction::initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo) //assumes that '(' is currently in str
{
    const char* s = str;
    if(*s == 'f'
        && *(s + 1) == 'u'
        && *(s + 2) == 'n'
       && *(s + 3) == 'c'
       && *(s + 4) == 't'
       && *(s + 5) == 'i'
       && *(s + 6) == 'o'
       && *(s + 7) == 'n'
       && *(s + 8) == '('
       )
    {
        s+=9;
        *endptr = (char*)s;
        
        s = tuiSkipToNextChar(s, debugInfo);
        
        TuiFunction* mjFunction = new TuiFunction(parent);
        mjFunction->debugInfo.fileName = debugInfo->fileName;
        
        std::string currentVarName = "";
        
        for(;; s++)
        {
            s = tuiSkipToNextChar(s, debugInfo, true);
            if(*s == ')' || *s == '\0')
            {
                if(!currentVarName.empty())
                {
                    mjFunction->argNames.push_back(currentVarName);
                    currentVarName = "";
                }
                s++;
                break;
            }
            else if(*s == ',' || *s == '\n')
            {
                if(*s == '\n')
                {
                    debugInfo->lineNumber++;
                }
                if(!currentVarName.empty())
                {
                    mjFunction->argNames.push_back(currentVarName);
                    currentVarName = "";
                }
            }
            else if(!isspace(*s))
            {
                currentVarName += *s;
            }
        }
        
        s = tuiSkipToNextChar(s, debugInfo);
        
        if(*s == ')')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
        }
        
        bool success = serializeFunctionBody(s, endptr, parent, &mjFunction->tokenMap, debugInfo, &mjFunction->statements);
        if(!success)
        {
            delete mjFunction;
            return nullptr;
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        *endptr = (char*)s;
        return mjFunction;
        
    }
    
    return nullptr;
}


TuiRef* TuiFunction::runExpression(TuiExpression* expression, uint32_t* tokenPos, TuiRef* result, TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo)
{
    if(*tokenPos >= expression->tokens.size())
    {
        return nullptr;
    }
    uint32_t token = expression->tokens[*tokenPos];
    if(token == Tui_token_end)
    {
        return nullptr;
    }
    
    if(token < Tui_token_VAR_START_INDEX)
    {
        switch (token) {
            case Tui_token_functionCall:
            {
                (*tokenPos)++;
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                if(!functionVar)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function");
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiTable* args = nullptr;
                TuiRef* arg = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                (*tokenPos)++;
                while(arg)
                {
                    if(!args)
                    {
                        args = new TuiTable(functionState);
                    }
                    args->arrayObjects.push_back(arg);
                    arg->retain();
                    arg = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                    (*tokenPos)++;
                }
                
                TuiRef* functionResult = ((TuiFunction*)functionVar)->call(args, functionState, result, debugInfo);
                if(args)
                {
                    args->release();
                }
                
                if(result && result->type() == functionResult->type())
                {
                    result->assign(functionResult);
                }
                else
                {
                    return functionResult;
                }
                
            }
                break;
            case Tui_token_equalTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                
                if(result && result->type() == Tui_ref_type_BOOL)
                {
                    if(!leftResult)
                    {
                        ((TuiBool*)result)->value = (!rightResult || rightResult->type() == Tui_ref_type_NIL);
                    }
                    else
                    {
                        ((TuiBool*)result)->value = leftResult->isEqual(rightResult);
                    }
                }
                else
                {
                    if(!leftResult)
                    {
                        return new TuiBool(!rightResult || rightResult->type() == Tui_ref_type_NIL);
                    }
                    return new TuiBool(leftResult->isEqual(rightResult));
                }
            }
                break;
            case Tui_token_or:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                (*tokenPos)++;
                
                if(leftResult && leftResult->boolValue())
                {
                    if(result && result->type() == Tui_ref_type_BOOL)
                    {
                        ((TuiBool*)result)->value = true;
                    }
                    else
                    {
                        return new TuiBool(true);
                    }
                }
                else
                {
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                    
                    if(result && result->type() == Tui_ref_type_BOOL)
                    {
                        ((TuiBool*)result)->value = (rightResult && rightResult->boolValue());
                    }
                    else
                    {
                        return new TuiBool(rightResult && rightResult->boolValue());
                    }
                }
                
            }
                break;
            case Tui_token_and:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                (*tokenPos)++;
                
                if(!leftResult || !leftResult->boolValue())
                {
                    if(result && result->type() == Tui_ref_type_BOOL)
                    {
                        ((TuiBool*)result)->value = false;
                    }
                    else
                    {
                        return new TuiBool(false);
                    }
                }
                else
                {
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                    
                    if(result && result->type() == Tui_ref_type_BOOL)
                    {
                        ((TuiBool*)result)->value = (rightResult && rightResult->boolValue());
                    }
                    else
                    {
                        return new TuiBool(rightResult && rightResult->boolValue());
                    }
                }
                
            }
                break;
            case Tui_token_notEqualTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                
                if(result && result->type() == Tui_ref_type_BOOL)
                {
                    if(!leftResult)
                    {
                        ((TuiBool*)result)->value = (rightResult && rightResult->type() != Tui_ref_type_NIL);
                    }
                    else
                    {
                        ((TuiBool*)result)->value = !leftResult->isEqual(rightResult);
                    }
                }
                else
                {
                    if(!leftResult)
                    {
                        return new TuiBool(rightResult && rightResult->type() != Tui_ref_type_NIL);
                    }
                    return new TuiBool(!leftResult->isEqual(rightResult));
                }
            }
                break;
            case Tui_token_not:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                (*tokenPos)++;
                return TuiRef::logicalNot(leftResult);
            }
                break;
            case Tui_token_greaterThan:
            case Tui_token_lessThan:
            case Tui_token_greaterEqualTo:
            case Tui_token_lessEqualTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        double left = ((TuiNumber*)leftResult)->value;
                        (*tokenPos)++;
                        
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                        if(rightResult->type() != leftType)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                            return nullptr;
                        }
                        
                        if(result && result->type() == Tui_ref_type_BOOL)
                        {
                            switch (token) {
                                case Tui_token_greaterThan:
                                    ((TuiBool*)result)->value = left > ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_token_lessThan:
                                    ((TuiBool*)result)->value = left < ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_token_greaterEqualTo:
                                    ((TuiBool*)result)->value = left >= ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_token_lessEqualTo:
                                    ((TuiBool*)result)->value = left <= ((TuiNumber*)rightResult)->value;
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case Tui_token_greaterThan:
                                    return new TuiBool(left > ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_token_lessThan:
                                    return new TuiBool(left < ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_token_greaterEqualTo:
                                    return new TuiBool(left >= ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_token_lessEqualTo:
                                    return new TuiBool(left <= ((TuiNumber*)rightResult)->value);
                                    break;
                            };
                        }
                        
                    }
                        break;
                        
                    default:
                        break;
                }
            }
                break;
            case Tui_token_add:
            case Tui_token_addInPlace:
            case Tui_token_increment:
            case Tui_token_subtract:
            case Tui_token_multiply:
            case Tui_token_multiplyInPlace:
            case Tui_token_divide:
            case Tui_token_divideInPlace:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, debugInfo);
                if(!leftResult)
                {
                    leftResult = result;
                }
                
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        if(token == Tui_token_increment)
                        {
                            ((TuiNumber*)leftResult)->value++;
                        }
                        else
                        {
                            (*tokenPos)++;
                            TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                            if(!rightResult)
                            {
                                rightResult = result;
                            }
                            if(rightResult->type() != leftType)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                                return nullptr;
                            }
                            
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        ((TuiNumber*)result)->value += ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_subtract:
                                        ((TuiNumber*)result)->value -= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiply:
                                        ((TuiNumber*)result)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiNumber*)result)->value /= ((TuiNumber*)rightResult)->value;
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        ((TuiNumber*)result)->value += ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiNumber*)result)->value -= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiNumber*)result)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiNumber*)result)->value /= ((TuiNumber*)rightResult)->value;
                                        break;
                                };
                            }
                            else
                            {
                                double left = ((TuiNumber*)leftResult)->value;
                                switch (token) {
                                    case Tui_token_add:
                                        return new TuiNumber(left + ((TuiNumber*)rightResult)->value);
                                        break;
                                    case Tui_token_subtract:
                                        return new TuiNumber(left - ((TuiNumber*)rightResult)->value);
                                        break;
                                    case Tui_token_multiply:
                                        return new TuiNumber(left * ((TuiNumber*)rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        return new TuiNumber(left / ((TuiNumber*)rightResult)->value);
                                        break;
                                    default:
                                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid token");
                                        break;
                                };
                            }
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        dvec2 left = ((TuiVec2*)leftResult)->value;
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        uint32_t resultType = rightResult->type();
                        
                        if(result && resultType == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    ((TuiVec2*)result)->value += left;
                                    break;
                                case Tui_token_subtract:
                                    ((TuiVec2*)result)->value -= left;
                                    break;
                                case Tui_token_multiply:
                                    ((TuiVec2*)result)->value *= left;
                                    break;
                                case Tui_token_divide:
                                    ((TuiVec2*)result)->value /= left;
                                    break;
                            };
                        }
                        else if(resultType == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    return new TuiVec2(left + ((TuiVec2*)rightResult)->value);
                                    break;
                                case Tui_token_subtract:
                                    return new TuiVec2(left - ((TuiVec2*)rightResult)->value);
                                    break;
                                case Tui_token_multiply:
                                    return new TuiVec2(left * ((TuiVec2*)rightResult)->value);
                                    break;
                                case Tui_token_divide:
                                    return new TuiVec2(left / ((TuiVec2*)rightResult)->value);
                                    break;
                            };
                        }
                        else if(resultType == Tui_ref_type_NUMBER)
                        {
                            switch (token) {
                                case Tui_token_multiply:
                                    return new TuiVec2(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_token_divide:
                                    return new TuiVec2(left / ((TuiNumber*)rightResult)->value);
                                    break;
                                default:
                                {
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to add or subtract number from vec2");
                                    return nullptr;
                                }
                                    break;
                            };
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected vec2 or number");
                            return nullptr;
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        dvec3 left = ((TuiVec3*)leftResult)->value;
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        uint32_t resultType = rightResult->type();
                        
                        if(result && resultType == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    ((TuiVec3*)result)->value += left;
                                    break;
                                case Tui_token_subtract:
                                    ((TuiVec3*)result)->value -= left;
                                    break;
                                case Tui_token_multiply:
                                    ((TuiVec3*)result)->value *= left;
                                    break;
                                case Tui_token_divide:
                                    ((TuiVec3*)result)->value /= left;
                                    break;
                            };
                        }
                        else if(resultType == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    return new TuiVec3(left + ((TuiVec3*)rightResult)->value);
                                    break;
                                case Tui_token_subtract:
                                    return new TuiVec3(left - ((TuiVec3*)rightResult)->value);
                                    break;
                                case Tui_token_multiply:
                                    return new TuiVec3(left * ((TuiVec3*)rightResult)->value);
                                    break;
                                case Tui_token_divide:
                                    return new TuiVec3(left / ((TuiVec3*)rightResult)->value);
                                    break;
                            };
                        }
                        else if(resultType == Tui_ref_type_NUMBER)
                        {
                            switch (token) {
                                case Tui_token_multiply:
                                    return new TuiVec3(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_token_divide:
                                    return new TuiVec3(left / ((TuiNumber*)rightResult)->value);
                                    break;
                                default:
                                {
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to add or subtract number from vec3");
                                    return nullptr;
                                }
                                    break;
                            };
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected vec3 or number");
                            return nullptr;
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        dvec4 left = ((TuiVec4*)leftResult)->value;
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        uint32_t resultType = rightResult->type();
                        
                        if(result && resultType == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    ((TuiVec4*)result)->value += left;
                                    break;
                                case Tui_token_subtract:
                                    ((TuiVec4*)result)->value -= left;
                                    break;
                                case Tui_token_multiply:
                                    ((TuiVec4*)result)->value *= left;
                                    break;
                                case Tui_token_divide:
                                    ((TuiVec4*)result)->value /= left;
                                    break;
                            };
                        }
                        else if(resultType == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    return new TuiVec4(left + ((TuiVec4*)rightResult)->value);
                                    break;
                                case Tui_token_subtract:
                                    return new TuiVec4(left - ((TuiVec4*)rightResult)->value);
                                    break;
                                case Tui_token_multiply:
                                    return new TuiVec4(left * ((TuiVec4*)rightResult)->value);
                                    break;
                                case Tui_token_divide:
                                    return new TuiVec4(left / ((TuiVec4*)rightResult)->value);
                                    break;
                            };
                        }
                        else if(resultType == Tui_ref_type_NUMBER)
                        {
                            switch (token) {
                                case Tui_token_multiply:
                                    return new TuiVec4(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_token_divide:
                                    return new TuiVec4(left / ((TuiNumber*)rightResult)->value);
                                    break;
                                default:
                                {
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to add or subtract number from vec4");
                                    return nullptr;
                                }
                                    break;
                            };
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected vec2 or number");
                            return nullptr;
                        }
                        
                    }
                        break;
                        
                    default:
                        break;
                }
            }
                break;
                
            default:
            {
                TuiError("Unimplemented");
            }
                break;
        }
    }
    else
    {
        TuiRef* foundValue = nullptr;
        if(locals->count(token) != 0)
        {
            foundValue = locals->at(token);
        }
        else if(tokenMap->refsByToken.count(token) != 0)
        {
            foundValue = tokenMap->refsByToken[token];
        }
        
        if(foundValue && foundValue->type() != Tui_ref_type_NIL)
        {
            if(result && result->type() == foundValue->type())
            {
                result->assign(foundValue);
            }
            else
            {
                return foundValue;
            }
        }
        else
        {
            return nullptr;
        }
    }
    
    return nullptr;
}

TuiRef* TuiFunction::runStatement(TuiStatement* statement, TuiRef* result, TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo)
{
    debugInfo->lineNumber = statement->lineNumber;
    switch(statement->type)
    {
        case Tui_statement_type_RETURN:
        {
            return new TuiRef(parent);
        }
            break;
        case Tui_statement_type_RETURN_EXPRESSION:
        {
            TuiRef* existingValue = result;
            uint32_t tokenPos = 0;
            TuiRef* newResult = runExpression(statement->expression, &tokenPos, existingValue, functionState, parent, tokenMap, locals, debugInfo);
            if(newResult)
            {
                return newResult;
            }
            if(result)
            {
                return result;
            }
            return new TuiRef(parent);
        }
            break;
        case Tui_statement_type_VAR_ASSIGN:
        {
            TuiRef* existingValue = nullptr;
            if(locals->count(statement->varToken) != 0)
            {
                existingValue = locals->at(statement->varToken);
            }
            else if(functionState->objectsByStringKey.count(statement->varName->value) != 0)
            {
                existingValue = functionState->objectsByStringKey[statement->varName->value];
                (*locals)[statement->varToken] = existingValue;
            }
            else
            {
                existingValue = functionState->recursivelyFindVariable(statement->varName, false, functionState, tokenMap, locals, debugInfo);
                if(existingValue)
                {
                    (*locals)[statement->varToken] = existingValue;
                }
            }
            
            uint32_t tokenPos = 0;
            TuiRef* result = runExpression(statement->expression, &tokenPos, existingValue, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                tokenMap->refsByToken[statement->varToken] = result;
                functionState->objectsByStringKey[statement->varName->value] = result;
                result->retain();
                (*locals)[statement->varToken] = result;
                
            }//else it updated the existing value
            
        }
            break;
        case Tui_statement_type_FUNCTION_CALL:
        {
            uint32_t tokenPos = 0;
            debugInfo->lineNumber = statement->lineNumber; //?
            TuiRef* result = runExpression(statement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                result->release();
            }
        }
            break;
            
        case Tui_statement_type_FOR:
        {
            TuiForStatement* forStatement = (TuiForStatement*)statement;
            
            TuiTable* containerObject = nullptr; //only used in for(object in table)
            
            if(forStatement->continueExpression)
            {
                uint32_t tokenPos = 0;
                TuiRef* result = runExpression(forStatement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                if(result)
                {
                    tokenMap->refsByToken[forStatement->varToken] = result;
                    functionState->objectsByStringKey[forStatement->varName->value] = result;
                    result->retain();
                    (*locals)[forStatement->varToken] = result;
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Unable to initialize for loop");
                    return nullptr;
                }
            }
            else
            {
                uint32_t tokenPos = 0;
                TuiRef* result = runExpression(forStatement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                if(result)
                {
                    if(result->type() == Tui_ref_type_TABLE)
                    {
                        containerObject = (TuiTable*)result;
                    }
                    else
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Object to iterate is not a table:%s", result->getDebugString().c_str());
                        return nullptr;
                    }
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Table couldn't be found in for loop");
                    return nullptr;
                }
            }
            
            if(containerObject)
            {
                if(!containerObject->arrayObjects.empty())
                {
                    TuiNumber* indexNumber = nullptr;
                    if(forStatement->indexOrKeyToken != 0)
                    {
                        indexNumber = new TuiNumber(0);
                        (*locals)[forStatement->indexOrKeyToken] = indexNumber;
                    }
                    
                    int i = 0;
                    for(TuiRef* object : containerObject->arrayObjects)
                    {
                        if(indexNumber)
                        {
                            indexNumber->value = i++;
                        }
                        (*locals)[forStatement->varToken] = object;
                        
                        TuiRef* runResult = runStatementArray(forStatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                        
                        if(runResult)
                        {
                            return runResult;
                        }
                    }
                    
                    if(indexNumber)
                    {
                        delete indexNumber;
                    }
                }
                
                if(!containerObject->objectsByStringKey.empty())
                {
                    TuiString* keyString = nullptr;
                    if(forStatement->indexOrKeyToken != 0)
                    {
                        keyString = new TuiString("");
                        (*locals)[forStatement->indexOrKeyToken] = keyString;
                    }
                    
                    for(auto& kv : containerObject->objectsByStringKey)
                    {
                        if(keyString)
                        {
                            keyString->value = kv.first;
                        }
                        (*locals)[forStatement->varToken] = kv.second;
                        
                        TuiRef* runResult = runStatementArray(forStatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                        
                        if(runResult)
                        {
                            return runResult;
                        }
                    }
                    
                    if(keyString)
                    {
                        delete keyString;
                    }
                }
            }
            else
            {
                while(1)
                {
                    debugInfo->lineNumber = statement->lineNumber; //?
                    
                    bool expressionPass = true;
                    
                    
                    uint32_t tokenPos = 0;
                    TuiRef* expressionResult = runExpression(forStatement->continueExpression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo); //todo pass in an TuiBool result
                    
                    if(expressionResult)
                    {
                        expressionPass = expressionResult->boolValue();
                    }
                    
                    if(expressionPass)
                    {
                        TuiRef* runResult = runStatementArray(forStatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                        
                        if(runResult)
                        {
                            return runResult;
                        }
                    }
                    else
                    {
                        break;
                    }
                    
                    if(forStatement->incrementStatement)
                    {
                        TuiRef* runResult = runStatement(forStatement->incrementStatement, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(runResult)
                        {
                            return runResult;
                        }
                    }
                }
            }
        }
            break;
        case Tui_statement_type_IF:
        {
            TuiIfStatement* currentSatement = (TuiIfStatement*)statement;
            uint32_t tokenPos = 0;
            
            while(currentSatement)
            {
                TuiRef* expressionResult = runExpression(currentSatement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                
                bool expressionPass = true;
                if(expressionResult)
                {
                    expressionPass = expressionResult->boolValue();
                }
                
                if(expressionPass)
                {
                    TuiRef* runResult = runStatementArray(currentSatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                    
                    if(runResult)
                    {
                        return runResult;
                    }
                    break;
                }
                else
                {
                    if(currentSatement->elseIfStatement)
                    {
                        if(currentSatement->elseIfStatement->expression)
                        {
                            currentSatement = currentSatement->elseIfStatement;
                        }
                        else
                        {
                            TuiRef* runResult = runStatementArray(currentSatement->elseIfStatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                            if(runResult)
                            {
                                return runResult;
                            }
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
            break;
        default:
            break;
    }
    return nullptr;
}

    
TuiRef* TuiFunction::runStatementArray(std::vector<TuiStatement*>& statements_,  TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo) //static
{
    for(TuiStatement* statement : statements_)
    {
        TuiRef* newResult = TuiFunction::runStatement(statement, result, functionState, parent, tokenMap, locals, debugInfo);
        if(newResult)
        {
            return newResult;
        }
    }
    
    return nullptr;
}

    
TuiFunction::TuiFunction(TuiTable* parent_)
:TuiRef(parent_)
{
}


TuiFunction::TuiFunction(std::function<TuiRef*(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func_, TuiTable* parent_)
:TuiRef(parent_)
{
    func = func_;
}

TuiFunction::~TuiFunction()
{
    for(TuiStatement* statement : statements)
    {
        delete statement;
    }
}

TuiRef* TuiFunction::call(TuiTable* args, TuiTable* callLocationState, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)
{
    if(func)
    {
        TuiTable* currentCallState = new TuiTable(parent);
        TuiRef* result = func(args, currentCallState, existingResult, callingDebugInfo);
        currentCallState->release();
        return result;
    }
    else
    {
        TuiTable* currentCallState = new TuiTable(parent);
        std::map<uint32_t, TuiRef*> locals;
        
        int i = 0;
        int maxArgs = (int)argNames.size();
        for(TuiRef* arg : args->arrayObjects)
        {
            if(i >= maxArgs)
            {
                TuiParseWarn(debugInfo.fileName.c_str(), 0, "Too many arguments supplied to function. ignoring:%s", arg->getDebugString().c_str());
                continue;
            }
            const std::string& argName = argNames[i];
            currentCallState->objectsByStringKey[argName] = arg;
            arg->retain();
            if(tokenMap.tokensByVarNames.count(argName) != 0)
            {
                locals[tokenMap.tokensByVarNames[argName]] = arg;
            }
            
            i++;
        }
        
        
        TuiRef* result = runStatementArray(statements,  existingResult,  currentCallState, parent, &tokenMap, &locals, &debugInfo);
        //currentCallState->debugLog();
        currentCallState->release();
        
        return result;
    }
}
