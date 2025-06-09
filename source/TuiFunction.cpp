#include "TuiFunction.h"

#include "TuiTable.h"
#include "TuiRef.h"


// tokenize a value chain eg foo.bar().x[3].y
void serializeValue(const char* str,
                    char** endptr,
                    TuiExpression* expression,
                    TuiTable* parent,
                    TuiTokenMap* tokenMap,
                    uint32_t tokenStartPos,
                    TuiDebugInfo* debugInfo,
                    std::string* foundVarName = nullptr)
{
    const char* s = str;
    
    std::string stringBuffer = "";
    
    bool singleQuote = false;
    bool doubleQuote = false;
    bool escaped = false;
    
    bool hasFoundInitialValue = false;
    
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
                    s++;
                    break;
                }
                else
                {
                    singleQuote = true;
                }
            }
            else
            {
                stringBuffer += *s;
                escaped = false;
            }
        }
        else if(*s == '"')
        {
            if(!escaped && !singleQuote)
            {
                if(doubleQuote)
                {
                    s++;
                    break;
                }
                else
                {
                    doubleQuote = true;
                }
            }
            else
            {
                stringBuffer += *s;
                escaped = false;
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
                stringBuffer += *s;
                escaped = false;
            }
        }
        else if(escaped || singleQuote || doubleQuote)
        {
            stringBuffer += *s;
            escaped = false;
        }
        else if(*s == '#' || *s == '\n' || *s == ',' || isspace(*s) || *s == ')' || *s == ']' || TuiExpressionOperatorsSet.count(*s) != 0)
        {
            break;
        }
        else if(*s == '.')
        {
            TuiError("todo");
            /*if(*(s+1) == '.') // .. syntax eg. ..foo.x
            {
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
            }*/
        }
        else if(*s == '[') //to access array elements
        {
            TuiError("todo");
            break;
        }
        else if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            double value = strtod(s, endptr);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            TuiNumber* number = new TuiNumber(value, parent);
            uint32_t constantNumberToken = tokenMap->tokenIndex++;
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, constantNumberToken);
            tokenMap->refsByToken[constantNumberToken] = number;
            break;
        }
        else if(*s == '{') // serialize table constructor
        {
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, Tui_token_tableConstruct);
            TuiFunction* constructorFunction = new TuiFunction(parent);
            uint32_t constructorFunctionToken = tokenMap->tokenIndex++;
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, constructorFunctionToken);
            tokenMap->refsByToken[constructorFunctionToken] = constructorFunction;
            //tokenMap->tokensByVarNames[varString->value] = leftVarToken;
            
            constructorFunction->tokenMap.tokenIndex = tokenMap->tokenIndex + 1;
            
            TuiError("todo");
            //todo maybe
            /*constructorFunction->tokenMap.readOnlyTokensByVarNames = tokenMap->readOnlyTokensByVarNames;
             for(auto& varNameAndToken : tokenMap->readWriteTokensByVarNames)
             {
             constructorFunction->tokenMap.readOnlyTokensByVarNames[varNameAndToken.first] = varNameAndToken.second;
             }*/
            
            
            TuiFunction::serializeFunctionBody(s, endptr, parent, &constructorFunction->tokenMap, debugInfo, &constructorFunction->statements);
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            
        }
        else if(*s == '(')
        {
            s++;
            
            if(stringBuffer.empty())
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected '(' at start of value");
                return;
            }
            
            uint32_t varToken = 0;
            
            if(!hasFoundInitialValue)
            {
                hasFoundInitialValue = true;
                
                if(tokenMap->capturedTokensByVarName.count(stringBuffer) != 0)
                {
                    varToken = tokenMap->capturedTokensByVarName[stringBuffer];
                }
                else
                {
                    varToken = tokenMap->tokenIndex++;
                    tokenMap->capturedTokensByVarName[stringBuffer] = varToken;
                }
            }
            else
            {
                TuiError("todo");
            }
            
            /*if(tokenMap->readWriteTokensByVarNames.count(varString->value) != 0)
            {
                leftVarToken = tokenMap->readWriteTokensByVarNames[varString->value];
            }
            else if(tokenMap->readOnlyTokensByVarNames.count(varString->value) != 0)
            {
                leftVarToken = tokenMap->readOnlyTokensByVarNames[varString->value];
            }
            else
            {*/
               // uint32_t varToken = tokenMap->tokenIndex++;
                //tokenMap->readOnlyTokensByVarNames[varString->value] = leftVarToken;
                //std::map<uint32_t, TuiRef*> locals;
            //    TuiRef* newValueRef = nullptr;//todo  = parent->recursivelyFindVariable(varString, true, parent, tokenMap, &locals, debugInfo);
            
                
            
               /* if(newValueRef)
                {
                    tokenMap->refsByToken[varToken] = newValueRef;
                }*/
           // }
            
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, Tui_token_functionCall);
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, varToken);
            
            while(*s != ')' && *s != '\0')
            {
                if(*s == ',')
                {
                    s++;
                }
                TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
            
            stringBuffer = "";
            
            expression->tokens.push_back(Tui_token_end);
        }
        else
        {
            stringBuffer += *s;
        }
    }
    
    if(!stringBuffer.empty())
    {
        if(singleQuote || doubleQuote)
        {
            TuiString* stringConstant = new TuiString(stringBuffer, parent);
            uint32_t stringConstantToken = tokenMap->tokenIndex++;
            tokenMap->refsByToken[stringConstantToken] = stringConstant;
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, stringConstantToken);
        }
        else //todo most likely we need to check if we are setting or not?
        {
            //todo use a map
            if(stringBuffer == "true")
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, Tui_token_true);
            }
            else if(stringBuffer == "false")
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, Tui_token_false);
            }
            else if(tokenMap->capturedTokensByVarName.count(stringBuffer) != 0)
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, tokenMap->capturedTokensByVarName[stringBuffer]);
                if(foundVarName)
                {
                    *foundVarName = stringBuffer;
                }
            }
            else
            {
                uint32_t variableToken = tokenMap->tokenIndex++;
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos++, variableToken);
                tokenMap->capturedTokensByVarName[stringBuffer] = variableToken;
                if(foundVarName)
                {
                    *foundVarName = stringBuffer;
                }
            }
        }
    }
    
    //maybe don't need to do this
    s = tuiSkipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
}

bool TuiFunction::recursivelySerializeExpression(const char* str,
                                                 char** endptr,
                                                 TuiExpression* expression,
                                                 TuiTable* parent,
                                                 TuiTokenMap* tokenMap,
                                                 TuiDebugInfo* debugInfo,
                                                 int operatorLevel,
                                                 std::string* setKey,
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
            serializeValue(s, endptr, expression, parent, tokenMap, tokenStartPos, debugInfo, setKey);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
        }
        /*else if(*s == '{') // serialize table constructor
        {
            expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_tableConstruct);
            TuiFunction* constructorFunction = new TuiFunction(parent);
            uint32_t constructorFunctionToken = tokenMap->tokenIndex++;
            expression->tokens.push_back(constructorFunctionToken);
            tokenMap->refsByToken[constructorFunctionToken] = constructorFunction;
            //tokenMap->tokensByVarNames[varString->value] = leftVarToken;
            
            constructorFunction->tokenMap.tokenIndex = tokenMap->tokenIndex + 1;
            
            TuiError("Unimplemented");
            //todo maybe
            **onstructorFunction->tokenMap.readOnlyTokensByVarNames = tokenMap->readOnlyTokensByVarNames;
            for(auto& varNameAndToken : tokenMap->readWriteTokensByVarNames)
            {
                constructorFunction->tokenMap.readOnlyTokensByVarNames[varNameAndToken.first] = varNameAndToken.second;
            }**
            
            
            TuiFunction::serializeFunctionBody(s, endptr, parent, &constructorFunction->tokenMap, debugInfo, &constructorFunction->statements);
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            
        }
        else if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && (*(s + 3) == '2' || *(s + 3) == '3' || *(s + 3) == '4'))
        {
            TuiError("Unimplemented");
            //todo
        }*/
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
                                                  nullptr,
                                                  tokenStartPos);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
    }
    
    *endptr = (char*)s;
    
    return complete;
}


static TuiStatement* serializeBasicStatement(const char* str,
                                             char** endptr,
                                             TuiTable* parent,
                                             TuiTokenMap* tokenMap,
                                             TuiDebugInfo* debugInfo,
                                             TuiExpression* optionalPreloadedExpression = nullptr,
                                             std::string* preloadedSetKey = nullptr)
{
    const char* s = str;
    
    TuiExpression* expression = optionalPreloadedExpression;
    std::string localSetKey;
    if(!expression)
    {
        expression = new TuiExpression();
        serializeValue(s, endptr, expression, parent, tokenMap, 0, debugInfo, &localSetKey);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
    }
    TuiStatement* statement = nullptr;
    
    if(expression->tokens[0] == Tui_token_functionCall)
    {
        statement = new TuiStatement(Tui_statement_type_functionCall);
        statement->lineNumber = debugInfo->lineNumber;
        statement->expression = expression;
    }
    else if(TuiExpressionOperatorsSet.count(*s) != 0)
    {
        if(*s == '=' && *(s + 1) != '=') // standard x = y assignment
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
            statement = new TuiStatement(Tui_statement_type_varAssign);
            if(preloadedSetKey)
            {
                statement->varName = *preloadedSetKey;
            }
            else
            {
                statement->varName = localSetKey;
            }
        }
        else //todo assumes += 4, ++ etc, check for other stuff and report error
        {
            TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default, nullptr, 0);
            statement = new TuiStatement(Tui_statement_type_varModify);
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        statement->lineNumber = debugInfo->lineNumber;
        statement->expression = expression;
    }
    else
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' variable assignment or function call");
    }
    
    if(!expression)
    {
    }
    
    *endptr = (char*)s;
    return statement;
}

TuiStatement* TuiFunction::serializeForStatement(const char* str, char** endptr, TuiTable* parent,  TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo) //entry point is after 'for'
{
    const char* s = str;
    if(*s == '(')
    {
        s++;
        s = tuiSkipToNextChar(s, debugInfo);
    }
    
    TuiExpression* expression = new TuiExpression();
    std::string setKey;
    
    bool skipInitializationStatement = false;
    if(*s == ',')
    {
        skipInitializationStatement = true;
    }
    else
    {
        recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default, &setKey);
        s = tuiSkipToNextChar(*endptr, debugInfo);
    }
    
    TuiStatement* resultStatement = nullptr;
    
    //todo test function call for initial statement, probably a bug, expression probably already contains the whole call
    if(skipInitializationStatement || *s == '=' || *s == '(') // for(i = 0, i < 5, i++)
    {
        //expression only contains the variable so far
        TuiForExpressionsStatement* forStatement = new TuiForExpressionsStatement();
        resultStatement = forStatement;
        forStatement->lineNumber = debugInfo->lineNumber;
        if(!skipInitializationStatement)
        {
            forStatement->initialStatement = serializeBasicStatement(s, endptr, parent, tokenMap, debugInfo, expression, &setKey);
            s = tuiSkipToNextChar(*endptr, debugInfo);
        }
        s++; // ','
        s = tuiSkipToNextChar(s, debugInfo);
        
        forStatement->continueExpression = new TuiExpression();
        recursivelySerializeExpression(s, endptr, forStatement->continueExpression, parent, tokenMap, debugInfo, Tui_operator_level_default);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        s++; // ','
        s = tuiSkipToNextChar(s, debugInfo);
        
        forStatement->incrementStatement = serializeBasicStatement(s, endptr, parent, tokenMap, debugInfo);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        s++; // ')'
        s = tuiSkipToNextChar(s, debugInfo);
        
        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &forStatement->statements);
        if(!success)
        {
            return nullptr;
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo);
        
    }
    else // for(object in table) || for(indexOrKey, object in table) || for(object : table) || for(indexOrKey, object : table)
    {
        int containerObjectTokenIndex = 1;
        TuiForContainerLoopStatement* forStatement = nullptr;
        if(*s == ',') // for(indexOrKey, object in table)
        {
            containerObjectTokenIndex = 2;
            
            forStatement = new TuiForContainerLoopStatement(Tui_statement_type_forKeyedValues);
            resultStatement = forStatement;
            forStatement->lineNumber = debugInfo->lineNumber;
            forStatement->expression = expression;
            
            s++; // ','
            s = tuiSkipToNextChar(s, debugInfo);
            serializeValue(s, endptr, expression, parent, tokenMap, 1, debugInfo, nullptr); //store object var at index 1
            s = tuiSkipToNextChar(*endptr, debugInfo);
        }
        else // for(object in table)
        {
            forStatement = new TuiForContainerLoopStatement(Tui_statement_type_forValues);
            resultStatement = forStatement;
            forStatement->lineNumber = debugInfo->lineNumber;
            forStatement->expression = expression;
        }
        
        if(*s == 'i')
        {
            s+= 2;
        }
        else if(*s == ':')
        {
            s++;
        }
        else
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or 'in' or ':' in for loop");
        }
        s = tuiSkipToNextChar(s, debugInfo);
        
        serializeValue(s, endptr, expression, parent, tokenMap, containerObjectTokenIndex, debugInfo, nullptr);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        s++; // ')'
        s = tuiSkipToNextChar(s, debugInfo);
        
        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &forStatement->statements);
        if(!success)
        {
            return nullptr;
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo);
    }
    
    return resultStatement;
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
                TuiStatement* statement = new TuiStatement(Tui_statement_type_return);
                statements->push_back(statement);
                s++;
                s = tuiSkipToNextChar(s, debugInfo, true);
                break;
            }
            else
            {
                TuiStatement* statement = new TuiStatement(Tui_statement_type_returnExpression);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = new TuiExpression();
                
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        
        s = tuiSkipToNextChar(s, debugInfo);
        
        if(*s == '}')
        {
            s++;
            break;
        }
        
        TuiStatement* statement = serializeBasicStatement(s, endptr, parent, tokenMap, debugInfo);
        statements->push_back(statement);
        
        s = *endptr;
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


TuiRef* TuiFunction::runExpression(TuiExpression* expression,
                                   uint32_t* tokenPos,
                                   TuiRef* result,
                                   TuiTable* functionState,
                                   TuiTable* parent,
                                   TuiTokenMap* tokenMap,
                                   std::map<uint32_t, TuiRef*>* locals,
                                   std::map<uint32_t, TuiRef*>* captures,
                                   TuiDebugInfo* debugInfo,
                                   TuiRef* assignNewLocal)
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
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                if(!functionVar)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function");
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiTable* args = nullptr;
                TuiRef* arg = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                
                while(expression->tokens[*tokenPos] != Tui_token_end)
                {
                    if(!args)
                    {
                        args = new TuiTable(functionState);
                    }
                    
                    if(!arg)
                    {
                        arg = new TuiRef(functionState);
                    }
                    else
                    {
                        arg->retain();
                    }
                    
                    args->arrayObjects.push_back(arg);
                    
                    (*tokenPos)++;
                    arg = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                }
                (*tokenPos)++;
                
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
            case Tui_token_tableConstruct:
            {
                (*tokenPos)++;
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                if(!functionVar)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function");
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiError("todo");
                /*
                std::map<uint32_t, TuiRef*> functionLocals;
                for(auto& varNameAndToken : functionVar->tokenMap.readOnlyTokensByVarNames)
                {
                    uint32_t writeToken = varNameAndToken.second;
                    
                    if(functionVar->tokenMap.refsByToken.count(writeToken) == 0)
                    {
                       // tokenMap->readWriteTokensByVarNames
                        
                        uint32_t readToken = 0;
                        
                        const std::string& varName = varNameAndToken.first;
                        if(tokenMap->readWriteTokensByVarNames.count(varName) != 0)
                        {
                            readToken = tokenMap->readWriteTokensByVarNames[varName];
                        }
                        else if(tokenMap->readOnlyTokensByVarNames.count(varName) != 0)
                        {
                            readToken = tokenMap->readOnlyTokensByVarNames[varName];
                        }
                        
                        if(readToken)
                        {
                            functionLocals[writeToken] = tokenMap->refsByToken[readToken];
                        }
                        else
                        {
                            TuiError("expected token");
                        }
                        
                        //functionLocals[writeToken] = functionVar->tokenMap.refsByToken[token];
                    }
                }
                
                TuiRef* tableResultRef = nullptr;
                ((TuiFunction*)functionVar)->call(nullptr, functionState, result, &functionLocals, debugInfo, &tableResultRef);
                
                if(result && result->type() == tableResultRef->type())
                {
                    result->assign(tableResultRef);
                }
                else
                {
                    return tableResultRef;
                }*/
                
            }
                break;
            case Tui_token_equalTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                    
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                    
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                (*tokenPos)++;
                return TuiRef::logicalNot(leftResult);
            }
                break;
            case Tui_token_true:
            {
                if(result && result->type() == Tui_ref_type_BOOL)
                {
                    ((TuiBool*)result)->value = true;
                }
                return new TuiBool(true);
            }
                break;
            case Tui_token_false:
            {
                if(result && result->type() == Tui_ref_type_BOOL)
                {
                    ((TuiBool*)result)->value = false;
                }
                return new TuiBool(false);
            }
                break;
            case Tui_token_greaterThan:
            case Tui_token_lessThan:
            case Tui_token_greaterEqualTo:
            case Tui_token_lessEqualTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        double left = ((TuiNumber*)leftResult)->value;
                        (*tokenPos)++;
                        
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                TuiRef* leftResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                            TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                                switch (token) {
                                    case Tui_token_add:
                                        return new TuiNumber(((TuiNumber*)leftResult)->value + ((TuiNumber*)rightResult)->value);
                                        break;
                                    case Tui_token_subtract:
                                        return new TuiNumber(((TuiNumber*)leftResult)->value - ((TuiNumber*)rightResult)->value);
                                        break;
                                    case Tui_token_multiply:
                                        return new TuiNumber(((TuiNumber*)leftResult)->value * ((TuiNumber*)rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        return new TuiNumber(((TuiNumber*)leftResult)->value / ((TuiNumber*)rightResult)->value);
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        ((TuiNumber*)leftResult)->value += ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiNumber*)leftResult)->value -= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiNumber*)leftResult)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiNumber*)leftResult)->value /= ((TuiNumber*)rightResult)->value;
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, functionState, parent, tokenMap, locals, captures, debugInfo);
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
                        
                    case Tui_ref_type_STRING: //left type string
                    {
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        if(result && result->type() == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    ((TuiString*)result)->value += ((TuiString*)rightResult)->value;
                                    break;
                                case Tui_token_addInPlace:
                                    ((TuiString*)result)->value += ((TuiString*)rightResult)->value;
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case Tui_token_add:
                                    return new TuiString(((TuiString*)leftResult)->value + rightResult->getStringValue());
                                    break;
                                case Tui_token_addInPlace:
                                    ((TuiString*)leftResult)->value += rightResult->getStringValue();
                                    break;
                                    
                                default:
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid token");
                                    break;
                            };
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
        else if(captures->count(token) != 0)
        {
            foundValue = captures->at(token);
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

TuiRef* TuiFunction::runStatement(TuiStatement* statement,
                                  TuiRef* result,
                                  TuiTable* functionState,
                                  TuiTable* parent,
                                  TuiTokenMap* tokenMap,
                                  std::map<uint32_t, TuiRef*>* locals,
                                  std::map<uint32_t, TuiRef*>* captures,
                                  TuiDebugInfo* debugInfo)
{
    debugInfo->lineNumber = statement->lineNumber;
    switch(statement->type)
    {
        case Tui_statement_type_return:
        {
            return new TuiRef(parent);
        }
            break;
        case Tui_statement_type_returnExpression:
        {
            TuiRef* existingValue = result;
            uint32_t tokenPos = 0;
            TuiRef* newResult = runExpression(statement->expression, &tokenPos, existingValue, functionState, parent, tokenMap, locals, captures, debugInfo);
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
        case Tui_statement_type_varModify:
        {
            uint32_t tokenPos = 0;
            runExpression(statement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
        }
            break;
        case Tui_statement_type_varAssign:
        {
            uint32_t tokenPos = 0;
           // debugInfo->lineNumber = statement->lineNumber; //?
            
            TuiRef* existingValue = runExpression(statement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
            tokenPos++;
            
            if(existingValue && existingValue->parent != functionState) //todo this should be optimized out during serialization with a const token to avoid the above expression run
            {
                existingValue = nullptr;
            }
            
            TuiRef* newValue = runExpression(statement->expression, &tokenPos, existingValue, functionState, parent, tokenMap, locals, captures, debugInfo);
            if(newValue)
            {
                //todo if this is a chain, we need to grab the first capture, then derive token by token
                
                if(existingValue)
                {
                    existingValue->parent->set(statement->varName, newValue);
                }
                
                (*locals)[statement->expression->tokens[0]] = newValue;
                
            }
        }
            break;
        case Tui_statement_type_functionCall:
        {
            uint32_t tokenPos = 0;
            //debugInfo->lineNumber = statement->lineNumber; //?
            TuiRef* result = runExpression(statement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
            if(result)
            {
                result->release();
            }
        }
            break;
            
        case Tui_statement_type_forValues:  // for(object in table)
        case Tui_statement_type_forKeyedValues:  // for(indexOrKey, object in table)
        {
            TuiForContainerLoopStatement* forStatement = (TuiForContainerLoopStatement*)statement;
            
            uint32_t containerTokenPos = 1;
            
            uint32_t indexToken = 0;
            uint32_t objectToken = forStatement->expression->tokens[0];
            
            if(statement->type == Tui_statement_type_forKeyedValues)
            {
                indexToken = objectToken;
                objectToken = forStatement->expression->tokens[1];
                containerTokenPos = 2;
            }
            
            TuiRef* collectionRef = runExpression(forStatement->expression, &containerTokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
            
            if(!collectionRef || collectionRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected table object, got:%s", (collectionRef ? collectionRef->getDebugString().c_str() : "nil"));
                return nullptr;
            }
            
            TuiTable* containerObject = (TuiTable*)collectionRef;
            
            if(!containerObject->arrayObjects.empty())
            {
                TuiNumber* indexNumber = nullptr;
                if(indexToken)
                {
                    indexNumber = new TuiNumber(0);
                    (*locals)[indexToken] = indexNumber;
                }
                
                int i = 0;
                for(TuiRef* object : containerObject->arrayObjects)
                {
                    if(indexNumber)
                    {
                        indexNumber->value = i++;
                    }
                    
                    (*locals)[objectToken] = object;
                    
                    TuiRef* runResult = runStatementArray(forStatement->statements, result, functionState, parent, tokenMap, locals, captures, debugInfo);
                    
                    if(runResult)
                    {
                        return runResult;
                    }
                }
            }
            
            if(!containerObject->objectsByStringKey.empty())
            {
                TuiString* keyString = nullptr;
                if(indexToken)
                {
                    keyString = new TuiString("");
                    (*locals)[indexToken] = keyString;
                }
                
                for(auto& kv : containerObject->objectsByStringKey)
                {
                    if(keyString)
                    {
                        keyString->value = kv.first;
                    }
                    
                    (*locals)[objectToken] = kv.second;
                    
                    TuiRef* runResult = runStatementArray(forStatement->statements, result, functionState, parent, tokenMap, locals, captures, debugInfo);
                    
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
            break;
            
        case Tui_statement_type_forExpressions: // for(i = 0, i < 5, i++)
        {
            TuiForExpressionsStatement* forStatement = (TuiForExpressionsStatement*)statement;
            
            uint32_t tokenPos = 0;
            //debugInfo->lineNumber = statement->lineNumber; //?
            
            if(forStatement->initialStatement)
            {
                TuiRef* runResult = runStatement(forStatement->initialStatement, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                if(runResult)
                {
                    runResult->release();
                }
            }
            
            tokenPos = 0;
            TuiRef* continueResult = runExpression(forStatement->continueExpression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
            while(continueResult && continueResult->boolValue())
            {
                TuiRef* runResult = runStatementArray(forStatement->statements, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                
                if(runResult)
                {
                    return runResult;
                }
                
                if(forStatement->incrementStatement)
                {
                    TuiRef* runResult = runStatement(forStatement->incrementStatement, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                    if(runResult)
                    {
                        runResult->release();
                    }
                }
                
                tokenPos = 0;
                TuiRef* newResult = runExpression(forStatement->continueExpression, &tokenPos, continueResult, functionState, parent, tokenMap, locals, captures, debugInfo);
                if(newResult)
                {
                    continueResult->release();
                    continueResult = newResult;
                }
            }
            if(continueResult)
            {
                continueResult->release();
            }
        }
            break;
        case Tui_statement_type_if:
        {
            TuiIfStatement* currentSatement = (TuiIfStatement*)statement;
            
            while(currentSatement)
            {
                uint32_t tokenPos = 0;
                TuiRef* expressionResult = runExpression(currentSatement->expression, &tokenPos, nullptr, functionState, parent, tokenMap, locals, captures, debugInfo);
                
                bool expressionPass = true;
                if(expressionResult)
                {
                    expressionPass = expressionResult->boolValue();
                }
                
                if(expressionPass)
                {
                    TuiRef* runResult = runStatementArray(currentSatement->statements, result, functionState, parent, tokenMap, locals, captures, debugInfo);
                    
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
                            TuiRef* runResult = runStatementArray(currentSatement->elseIfStatement->statements, result, functionState, parent, tokenMap, locals, captures, debugInfo);
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

    
TuiRef* TuiFunction::runStatementArray(std::vector<TuiStatement*>& statements_,
                                       TuiRef* result,
                                       TuiTable* functionState,
                                       TuiTable* parent,
                                       TuiTokenMap* tokenMap,
                                       std::map<uint32_t,TuiRef*>* locals,
                                       std::map<uint32_t, TuiRef*>* captures,
                                       TuiDebugInfo* debugInfo) //static
{
    for(TuiStatement* statement : statements_)
    {
        TuiRef* newResult = TuiFunction::runStatement(statement, result, functionState, parent, tokenMap, locals, captures, debugInfo);
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

TuiRef* TuiFunction::call(TuiTable* args,
                          TuiTable* callLocationState,
                          TuiRef* existingResult,
                          TuiDebugInfo* callingDebugInfo,
                          TuiRef** createdStateTable)
{
    if(func)
    {
        TuiTable* currentCallState = new TuiTable(callLocationState);
        TuiRef* result = func(args, parent, existingResult, callingDebugInfo);
        if(result)
        {
            result->retain();
        }
        if(createdStateTable)
        {
            *createdStateTable = currentCallState;
        }
        else
        {
            currentCallState->release();
        }
        return result;
    }
    else
    {
        TuiTable* currentCallState = new TuiTable(callLocationState);
        
        std::map<uint32_t, TuiRef*> captures; //collected at the start, retained, need to release
        std::map<uint32_t, TuiRef*> locals; //added to as variables are created, pointing to variables that are within this table, including the args. need to release
        
        if(args)
        {
            int i = 0;
            int maxArgs = (int)argNames.size();
            for(TuiRef* arg : args->arrayObjects)
            {
                if(i >= maxArgs)
                {
                    TuiParseWarn(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "Too many arguments supplied to function. ignoring:%s", arg->getDebugString().c_str());
                    continue;
                }
                const std::string& argName = argNames[i];
                currentCallState->objectsByStringKey[argName] = arg;
                arg->retain();
               // TuiError("todo");
                /*
                if(tokenMap.readWriteTokensByVarNames.count(argName) != 0)
                {
                    (*locals)[tokenMap.readWriteTokensByVarNames[argName]] = arg;
                }
                else
                {
                    uint32_t argToken = tokenMap.tokenIndex++;
                    tokenMap.readWriteTokensByVarNames[argName] = argToken;
                    tokenMap.refsByToken[argToken] = arg;
                }*/
                
                i++;
            }
        }
        
        for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
        {
            if(currentCallState->objectsByStringKey.count(varNameAndToken.first) != 0) //it's an arg
            {
                TuiRef* var = currentCallState->objectsByStringKey[varNameAndToken.first];
                var->retain();//todo release this
                locals[varNameAndToken.second] = var;
            }
            else
            {
                TuiTable* parentRef = parent;
                while(parentRef)
                {
                    if(parentRef->objectsByStringKey.count(varNameAndToken.first) != 0)
                    {
                        TuiRef* var = parentRef->objectsByStringKey[varNameAndToken.first];
                        var->retain();//todo release this
                        captures[varNameAndToken.second] = var;
                        break;
                    }
                    parentRef = parentRef->parent;
                }
                
            }
        }
        
        
        TuiRef* result = runStatementArray(statements,  existingResult,  currentCallState, parent, &tokenMap, &locals, &captures, &debugInfo);
        //currentCallState->debugLog();
        if(createdStateTable)
        {
            *createdStateTable = currentCallState;
        }
        else
        {
            currentCallState->release();
        }
        
        return result;
    }
}
