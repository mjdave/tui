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
                    std::string* foundVarName = nullptr,
                    int* foundVarIndex = nullptr,
                    bool* foundVarWasCapture = nullptr)
{
    const char* s = str;
    
    std::string stringBuffer = "";
    
    bool singleQuote = false;
    bool doubleQuote = false;
    bool escaped = false;
    
    bool hasFoundInitialValue = false;
    bool varChainStarted = false;
    
    uint32_t tokenPos = tokenStartPos;
    
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
        else if(*s == '#' || *s == '\n' || *s == ',' || isspace(*s) || *s == ')' || *s == '}' || *s == ']' || TuiExpressionOperatorsSet.count(*s) != 0)
        {
            break;
        }
        else if(*s == '.' || *s == '(' || *s == '[')
        {
            if(*(s+1) == '.' && *s != '(' && *s != '[') // .. syntax eg. ..var
            {
                if(hasFoundInitialValue)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "'..' may only be used at the start of a variable chain or as a standalone var eg. ..var or .., not var..");
                    break;
                }
                hasFoundInitialValue = true;
                
                int depthCount = 0;
                while(*(s + 1) == '.')
                {
                    s++;
                    depthCount++;
                }
                
                uint32_t varToken = 0;
                if(tokenMap->capturedParentTokensByDepthCount.count(depthCount) != 0)
                {
                    varToken = tokenMap->capturedParentTokensByDepthCount[depthCount];
                }
                else
                {
                    varToken = tokenMap->tokenIndex++;
                    tokenMap->capturedParentTokensByDepthCount[depthCount] = varToken;
                }
                
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, varToken);
                
                s = tuiSkipToNextChar(s, debugInfo, true);
                *endptr = (char*)s;
                
            }
            else
            {
                if(!stringBuffer.empty())
                {
                    uint32_t varToken = 0;
                    if(singleQuote || doubleQuote)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unable to chain a quoted string");
                    }
                    else
                    {
                        if(hasFoundInitialValue)
                        {
                            if(!varChainStarted)
                            {
                                expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_varChain); //note tokenStartPos to place before all other tokens
                                tokenPos++;
                                varChainStarted = true;
                            }
                            
                            TuiString* stringConstant = new TuiString(stringBuffer, parent);
                            uint32_t stringConstantToken = tokenMap->tokenIndex++;
                            varToken = stringConstantToken;
                            tokenMap->refsByToken[stringConstantToken] = stringConstant;
                            expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_childByString);
                            expression->tokens.insert(expression->tokens.begin() + tokenPos++, stringConstantToken); //note if this is a function, the function token gets placed just before this, see below
                            if(foundVarName)
                            {
                                *foundVarName = stringBuffer;
                            }
                        }
                        else
                        {
                            hasFoundInitialValue = true;
                            if(tokenMap->localTokensByVarName.count(stringBuffer) != 0)
                            {
                                varToken = tokenMap->localTokensByVarName[stringBuffer];
                                expression->tokens.insert(expression->tokens.begin() + tokenPos++, varToken);
                                if(foundVarName)
                                {
                                    *foundVarName = stringBuffer;
                                }
                            }
                            else if(tokenMap->capturedTokensByVarName.count(stringBuffer) != 0)
                            {
                                varToken = tokenMap->capturedTokensByVarName[stringBuffer];
                                expression->tokens.insert(expression->tokens.begin() + tokenPos++, varToken);
                                if(foundVarName)
                                {
                                    *foundVarName = stringBuffer;
                                }
                                
                                if(foundVarWasCapture)
                                {
                                    *foundVarWasCapture = true;
                                }
                            }
                            else
                            {
                                varToken = tokenMap->tokenIndex++;
                                expression->tokens.insert(expression->tokens.begin() + tokenPos++, varToken);
                                tokenMap->capturedTokensByVarName[stringBuffer] = varToken;
                                if(foundVarName)
                                {
                                    *foundVarName = stringBuffer;
                                }
                                
                                if(foundVarWasCapture)
                                {
                                    *foundVarWasCapture = true;
                                }
                            }
                        }
                    }
                    
                    stringBuffer = "";
                }
                
                if(*s == '(')
                {
                    s++;
                    
                    expression->tokens.insert(expression->tokens.begin() + tokenPos - 1, Tui_token_functionCall);
                    tokenPos++;
                    
                    while(*s != ')' && *s != '\0')
                    {
                        if(*s == ',')
                        {
                            s++;
                            s = tuiSkipToNextChar(s, debugInfo);
                        }
                        TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                    }
                    
                    expression->tokens.push_back(Tui_token_end);
                    tokenPos = (int)expression->tokens.size();
                }
                else if(*s == '[')
                {
                    s++;
                    
                    if(!varChainStarted)
                    {
                        expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_varChain); //note tokenStartPos to place before all other tokens
                        tokenPos++;
                        varChainStarted = true;
                    }
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_childByArrayIndex);
                    TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    tokenPos = (int)expression->tokens.size();
                    s = tuiSkipToNextChar(*endptr, debugInfo);
                }
            }
        }
        else if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            double value = strtod(s, endptr);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            TuiNumber* number = new TuiNumber(value, parent);
            uint32_t constantNumberToken = tokenMap->tokenIndex++;
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, constantNumberToken);
            tokenMap->refsByToken[constantNumberToken] = number;
            break;
        }
        else if(*s == '{') // serialize table constructor
        {
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_tableConstruct);
            TuiFunction* constructorFunction = new TuiFunction(parent);
            constructorFunction->debugInfo.fileName = debugInfo->fileName;
            constructorFunction->debugInfo.lineNumber = debugInfo->lineNumber;
            //todo probably have to store initial lineNumber, then add difference to this parent function's debugInfo
            uint32_t constructorFunctionToken = tokenMap->tokenIndex++;
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, constructorFunctionToken);
            tokenMap->refsByToken[constructorFunctionToken] = constructorFunction;
            
            constructorFunction->tokenMap.tokenIndex = tokenMap->tokenIndex + 1;
            
            TuiTable* subParent = new TuiTable(parent);
            bool success = TuiFunction::serializeFunctionBody(s, endptr, subParent, &constructorFunction->tokenMap, debugInfo, true, &constructorFunction->statements);
            if(!success)
            {
                return;
            }
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
        }
        else
        {
            bool foundBuiltInType = false;
            
            if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 4) == '(')
            {
                int vecCount = 0;
                switch(*(s + 3))
                {
                    case '2':
                    {
                        vecCount = 2;
                        expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_vec2);
                    }
                        break;
                    case '3':
                    {
                        vecCount = 3;
                        expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_vec3);
                    }
                        break;
                    case '4':
                    {
                        vecCount = 4;
                        expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_vec4);
                    }
                        break;
                }
                
                if(vecCount > 0)
                {
                    foundBuiltInType = true;
                    
                    s+= 5;
                    s = tuiSkipToNextChar(s, debugInfo);
                    
                    
                    tokenPos++;
                    
                    int argCount = 0;
                    while(*s != ')' && *s != '\0' && argCount < vecCount)
                    {
                        if(*s == ',')
                        {
                            s++;
                            s = tuiSkipToNextChar(s, debugInfo);
                        }
                        TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        argCount++;
                    }
                }
            }
            
            TuiFunction* functionRef = TuiFunction::initWithHumanReadableString(s, endptr, parent, debugInfo, true);
            if(functionRef)
            {
                foundBuiltInType = true;
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_functionDeclaration);
                uint32_t functionToken = tokenMap->tokenIndex++;
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, functionToken);
                tokenMap->refsByToken[functionToken] = functionRef;
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
            }
            
            //todo maybe?
            /*
             
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
             */
            
            
            if(!foundBuiltInType)
            {
                stringBuffer += *s;
            }
        }
    }
    
    if(!stringBuffer.empty())
    {
        if(singleQuote || doubleQuote)
        {
            TuiString* stringConstant = new TuiString(stringBuffer, parent);
            uint32_t stringConstantToken = tokenMap->tokenIndex++;
            tokenMap->refsByToken[stringConstantToken] = stringConstant;
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, stringConstantToken);
        }
        else
        {
            if(hasFoundInitialValue)
            {
                if(!varChainStarted)
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_varChain); //note tokenStartPos to place before all other tokens
                    tokenPos++;
                    varChainStarted = true;
                }
                TuiString* stringConstant = new TuiString(stringBuffer, parent);
                uint32_t stringConstantToken = tokenMap->tokenIndex++;
                tokenMap->refsByToken[stringConstantToken] = stringConstant;
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_childByString);
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, stringConstantToken);
                if(foundVarName)
                {
                    *foundVarName = stringBuffer;
                }
            }
            else
            {
                //todo use a map
                if(stringBuffer == "true")
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_true);
                }
                else if(stringBuffer == "false")
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_false);
                }
                else if(tokenMap->localTokensByVarName.count(stringBuffer) != 0)
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, tokenMap->localTokensByVarName[stringBuffer]);
                    if(foundVarName)
                    {
                        *foundVarName = stringBuffer;
                    }
                }
                else if(tokenMap->capturedTokensByVarName.count(stringBuffer) != 0)
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, tokenMap->capturedTokensByVarName[stringBuffer]);
                    if(foundVarName)
                    {
                        *foundVarName = stringBuffer;
                    }
                    
                    if(foundVarWasCapture)
                    {
                        *foundVarWasCapture = true;
                    }
                }
                else
                {
                    uint32_t variableToken = tokenMap->tokenIndex++;
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, variableToken);
                    tokenMap->capturedTokensByVarName[stringBuffer] = variableToken;
                    if(foundVarName)
                    {
                        *foundVarName = stringBuffer;
                    }
                    
                    if(foundVarWasCapture)
                    {
                        *foundVarWasCapture = true;
                    }
                }
            }
        }
    }
    
    
    if(varChainStarted)
    {
        expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_end);
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
                                                 int* setIndex,
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
                                             bool sharesParentScope,
                                             TuiExpression* optionalPreloadedExpression = nullptr, //only sent through for the initial statement in a for loop
                                             std::string* preloadedSetKey = nullptr,
                                             int* preloadedSetIndex = nullptr)
{
    const char* s = str;
    
    TuiExpression* expression = optionalPreloadedExpression;
    std::string localSetKey;
    int localSetIndex;
    bool wasLocalCapture = false;
    if(!expression)
    {
        expression = new TuiExpression();
        serializeValue(s, endptr, expression, parent, tokenMap, 0, debugInfo, &localSetKey, &localSetIndex, &wasLocalCapture);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
    }
    TuiStatement* statement = nullptr;
    
    if(expression->tokens[0] == Tui_token_functionCall)
    {
        //TuiError("todo");
        statement = new TuiStatement(Tui_statement_type_functionCall);
        statement->lineNumber = debugInfo->lineNumber;
        statement->expression = expression;
    }
    else if(TuiExpressionOperatorsSet.count(*s) != 0)
    {
        if(*s == '=' && *(s + 1) != '=') // standard x = y assignment
        {
            uint32_t variableTokenToAddToLocals = 0;
            if(wasLocalCapture && (!sharesParentScope || parent->objectsByStringKey.count(localSetKey) == 0) && expression->tokens[0] != Tui_token_varChain)
            {
                expression->tokens.clear(); //remove parent capture. assumes a few things
                
                if(tokenMap->localTokensByVarName.count(localSetKey) != 0)
                {
                    expression->tokens.push_back(tokenMap->localTokensByVarName[localSetKey]);
                }
                else
                {
                    variableTokenToAddToLocals = tokenMap->tokenIndex++;
                    expression->tokens.push_back(variableTokenToAddToLocals);
                   // tokenMap->localVarNamesByToken[variableToken] = localSetKey;
                }
            }
            
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
            
            if(variableTokenToAddToLocals != 0)
            {
                tokenMap->localTokensByVarName[localSetKey] = variableTokenToAddToLocals; //must be added before the above call to recursivelySerializeExpression, so that captures are still available
            }
            
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
            TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default, nullptr, nullptr, 0);
            statement = new TuiStatement(Tui_statement_type_varModify);
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        statement->lineNumber = debugInfo->lineNumber;
        statement->expression = expression;
    }
    else
    {
        TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default, nullptr, nullptr, 0);
        statement = new TuiStatement(Tui_statement_type_value);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        statement->lineNumber = debugInfo->lineNumber;
        statement->expression = expression;
    }
    
    *endptr = (char*)s;
    return statement;
}

TuiStatement* TuiFunction::serializeForStatement(const char* str,
                                                 char** endptr,
                                                 TuiTable* parent,
                                                 TuiTokenMap* tokenMap,
                                                 TuiDebugInfo* debugInfo,
                                                 bool sharesParentScope) //entry point is after 'for'
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
            forStatement->initialStatement = serializeBasicStatement(s, endptr, parent, tokenMap, debugInfo, sharesParentScope, expression, &setKey);
            s = tuiSkipToNextChar(*endptr, debugInfo);
        }
        s++; // ','
        s = tuiSkipToNextChar(s, debugInfo);
        
        forStatement->continueExpression = new TuiExpression();
        recursivelySerializeExpression(s, endptr, forStatement->continueExpression, parent, tokenMap, debugInfo, Tui_operator_level_default);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        s++; // ','
        s = tuiSkipToNextChar(s, debugInfo);
        
        forStatement->incrementStatement = serializeBasicStatement(s, endptr, parent, tokenMap, debugInfo, sharesParentScope);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        s++; // ')'
        s = tuiSkipToNextChar(s, debugInfo);
        
        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, sharesParentScope, &forStatement->statements);
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
        
        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, sharesParentScope, &forStatement->statements);
        if(!success)
        {
            return nullptr;
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo);
    }
    
    return resultStatement;
}


bool TuiFunction::serializeFunctionBody(const char* str,
                                        char** endptr,
                                        TuiTable* parent,
                                        TuiTokenMap* tokenMap,
                                        TuiDebugInfo* debugInfo,
                                        bool sharesParentScope,
                                        std::vector<TuiStatement*>* statements)
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
        else if(*s == ',')
        {
            s++;
        }
        else if(*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3))))
        {
            s+=3;
            s = tuiSkipToNextChar(s, debugInfo);
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, parent, tokenMap, debugInfo, sharesParentScope);
            if(!statement)
            {
                return false;
            }
            statements->push_back(statement);
            s = tuiSkipToNextChar(*endptr, debugInfo, false);
        }
        else if(*s == 'i' && *(s + 1) == 'f' && (*(s + 2) == '(' || isspace(*(s + 2))))
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
            
            bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, true, &statement->statements);
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
                        
                        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, true, &currentStatement->elseIfStatement->statements);
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
                        
                        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, true, &currentStatement->elseIfStatement->statements);
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
        else
        {
            TuiStatement* statement = serializeBasicStatement(s, endptr, parent, tokenMap, debugInfo, sharesParentScope);
            statements->push_back(statement);
            s = *endptr;
        }
    }
    
    s = tuiSkipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
    
    return true;
}

TuiFunction* TuiFunction::initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, bool createStateSubTable) //assumes that '(' is currently in str.
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
        
        if(createStateSubTable)
        {
            parent = new TuiTable(parent);
        }
        
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
        
        bool success = serializeFunctionBody(s, endptr, parent, &mjFunction->tokenMap, debugInfo, false, &mjFunction->statements);
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
                                   TuiTable* parent,
                                   TuiTokenMap* tokenMap,
                                   TuiFunctionCallData* callData,
                                   TuiDebugInfo* debugInfo,
                                   std::string* setKey,
                                   int* setIndex,
                                   TuiRef** enclosingSetRef)
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
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                if(!functionVar)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiTable* args = nullptr;
                TuiRef* arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                
                if(arg)
                {
                    args = new TuiTable(parent);
                    arg->retain();
                    args->arrayObjects.push_back(arg);
                    
                    (*tokenPos)++;
                    
                    while(expression->tokens[*tokenPos] != Tui_token_end)
                    {
                        arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                        if(!arg)
                        {
                            arg = new TuiRef(parent);
                        }
                        else
                        {
                            arg->retain();
                        }
                        
                        args->arrayObjects.push_back(arg);
                        
                        (*tokenPos)++;
                    }
                    (*tokenPos)++;
                }
                
                
                TuiRef* functionResult = ((TuiFunction*)functionVar)->call(args, parent, result, debugInfo);
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
            case Tui_token_vec2:
            case Tui_token_vec3:
            case Tui_token_vec4:
            {
                (*tokenPos)++;
                
                TuiRef* x = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                (*tokenPos)++;
                
                if(!x || x->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "x component expected number, got:%s", (x ? x->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                TuiRef* y = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                (*tokenPos)++;
                
                if(!y || y->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "y component expected number, got:%s", (y ? y->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                switch (token) {
                    case Tui_token_vec2:
                        {
                            if(result && result->type() == Tui_ref_type_VEC2)
                            {
                                ((TuiVec2*)result)->value.x = ((TuiNumber*)x)->value;
                                ((TuiVec2*)result)->value.y = ((TuiNumber*)y)->value;
                            }
                            else
                            {
                                TuiVec2* newResultVec = new TuiVec2(dvec2(((TuiNumber*)x)->value, ((TuiNumber*)y)->value));
                                x->release();
                                y->release();
                                return newResultVec;
                            }
                        }
                        break;
                    case Tui_token_vec3:
                    case Tui_token_vec4:
                        {
                            TuiRef* z = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                            (*tokenPos)++;
                            
                            if(!z || z->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "z component expected number, got:%s", (z ? z->getDebugString().c_str() : "nil"));
                                return nullptr;
                            }
                            
                            if(token == Tui_token_vec3)
                            {
                                if(result && result->type() == Tui_ref_type_VEC3)
                                {
                                    ((TuiVec3*)result)->value.x = ((TuiNumber*)x)->value;
                                    ((TuiVec3*)result)->value.y = ((TuiNumber*)y)->value;
                                    ((TuiVec3*)result)->value.z = ((TuiNumber*)z)->value;
                                }
                                else
                                {
                                    TuiVec3* newResultVec = new TuiVec3(dvec3(((TuiNumber*)x)->value, ((TuiNumber*)y)->value, ((TuiNumber*)z)->value));
                                    x->release();
                                    y->release();
                                    z->release();
                                    return newResultVec;
                                }
                            }
                            else
                            {
                                TuiRef* w = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                                (*tokenPos)++;
                                
                                if(!w || w->type() != Tui_ref_type_NUMBER)
                                {
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "w component expected number, got:%s", (w ? w->getDebugString().c_str() : "nil"));
                                    return nullptr;
                                }
                                if(result && result->type() == Tui_ref_type_VEC4)
                                {
                                    ((TuiVec4*)result)->value.x = ((TuiNumber*)x)->value;
                                    ((TuiVec4*)result)->value.y = ((TuiNumber*)y)->value;
                                    ((TuiVec4*)result)->value.z = ((TuiNumber*)z)->value;
                                    ((TuiVec4*)result)->value.w = ((TuiNumber*)w)->value;
                                }
                                else
                                {
                                    TuiVec4* newResultVec = new TuiVec4(dvec4(((TuiNumber*)x)->value, ((TuiNumber*)y)->value, ((TuiNumber*)z)->value, ((TuiNumber*)w)->value));
                                    x->release();
                                    y->release();
                                    z->release();
                                    w->release();
                                    return newResultVec;
                                }
                            }
                        }
                        break;
                }
                
            }
                break;
            case Tui_token_tableConstruct:
            {
                (*tokenPos)++;
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiRef* functionResult = ((TuiFunction*)functionVar)->runTableConstruct(parent, result, debugInfo);
                
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
            case Tui_token_functionDeclaration:
            {
                (*tokenPos)++;
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                if(result && result->type() == functionVar->type())
                {
                    result->assign(functionVar);
                }
                else
                {
                    return functionVar;
                }
            }
                break;
            case Tui_token_equalTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                    
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                    
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                if(expression->tokens[*tokenPos+1] == Tui_token_end)
                {
                    (*tokenPos)++; //Tui_token_end
                }
                else
                {
                    TuiError("expected Tui_token_end");
                }
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
            case Tui_token_varChain:
            {
                (*tokenPos)++;
                TuiRef* chainParent = nullptr;
                TuiRef* chainResult = parent;
                TuiRef* keyConstant = nullptr;
                
                while(1)
                {
                    switch(chainResult->type())
                    {
                        case Tui_ref_type_TABLE:
                        {
                            chainParent = chainResult;
                            chainResult = runExpression(expression, tokenPos, result, (TuiTable*)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                        }
                            break;
                        case Tui_ref_type_VEC2:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                        }
                            break;
                        default:
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "no '.' syntax supported for type:%s", parent->getTypeName().c_str());
                            return nullptr;
                        }
                            break;
                    }
                    
                    (*tokenPos)++;
                    if(expression->tokens[*tokenPos] == Tui_token_end)
                    {
                        break;
                    }
                }
                
                if(keyConstant && chainResult->type() != Tui_ref_type_TABLE)
                {
                    if(enclosingSetRef)
                    {
                        *enclosingSetRef = chainResult;
                        *setKey = ((TuiString*)keyConstant)->value; //assumptions
                    }
                    
                    switch(chainResult->type())
                    {
                        case Tui_ref_type_VEC2:
                        {
                            switch(((TuiString*)keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.x;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec2*)chainResult)->value.x);
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.y;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec2*)chainResult)->value.y);
                                    }
                                    return nullptr;
                                }
                                    break;
                            }
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            switch(((TuiString*)keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.x;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec3*)chainResult)->value.x);
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.y;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec3*)chainResult)->value.y);
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'z':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.z;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec3*)chainResult)->value.z);
                                    }
                                    return nullptr;
                                }
                                    break;
                            }
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            switch(((TuiString*)keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.x;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec3*)chainResult)->value.x);
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.y;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec3*)chainResult)->value.y);
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'z':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.z;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec3*)chainResult)->value.z);
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'w':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec4*)chainResult)->value.w;
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        return new TuiNumber(((TuiVec4*)chainResult)->value.w);
                                    }
                                    return nullptr;
                                }
                                    break;
                            }
                        }
                            break;
                        default:
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "no '.' syntax supported for type:%s", parent->getTypeName().c_str());
                            return nullptr;
                        }
                            break;
                    }
                }
                
                
                if(enclosingSetRef)
                {
                    *enclosingSetRef = chainParent;
                }
                
                return chainResult;
            }
                break;
            case Tui_token_childByString:
            case Tui_token_childByArrayIndex:
            {
                bool isStringKey = (token == Tui_token_childByString);
                (*tokenPos)++;
                bool isFunctionCall = false;
                if(expression->tokens[*tokenPos] == Tui_token_functionCall)
                {
                    isFunctionCall = true;
                    (*tokenPos)++;
                }
                
                TuiRef* keyConstant = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                TuiRef* child = nullptr;
                
                if(isStringKey)
                {
                    if(keyConstant->type() != Tui_ref_type_STRING)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected string");
                        return nullptr;
                    }
                    
                    if(parent->objectsByStringKey.count(((TuiString*)keyConstant)->value) != 0)
                    {
                        child = parent->objectsByStringKey[((TuiString*)keyConstant)->value];
                    }
                    
                    /*
                     switch(parent->type())
                     {
                         case Tui_ref_type_TABLE:
                         {
                             if(parent->objectsByStringKey.count(((TuiString*)keyConstant)->value) != 0)
                             {
                                 child = parent->objectsByStringKey[((TuiString*)keyConstant)->value];
                             }
                         }
                             break;
                         default:
                         {
                             TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "no '.' syntax supported for type:%s", parent->getTypeName().c_str());
                             return nullptr;
                         }
                             break;
                     }*/
                }
                else
                {
                    if(keyConstant->type() != Tui_ref_type_NUMBER)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                        return nullptr;
                    }
                    
                    int arrayIndex = ((TuiNumber*)keyConstant)->value;
                    if(parent->arrayObjects.size() > arrayIndex)
                    {
                        child = parent->arrayObjects[arrayIndex];
                    }
                }
                
                if(!child)
                {
                    return nullptr;
                }
                
                if(isFunctionCall)
                {
                    if(child->type() != Tui_ref_type_FUNCTION)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (child ? child->getDebugString().c_str() : "nil"));
                        return nullptr;
                    }
                    
                    TuiTable* args = nullptr;
                    (*tokenPos)++;
                    TuiRef* arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                    
                    if(arg)
                    {
                        args = new TuiTable(parent);
                        arg->retain();
                        args->arrayObjects.push_back(arg);
                        
                        (*tokenPos)++;
                        
                        while(expression->tokens[*tokenPos] != Tui_token_end)
                        {
                            arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                            if(!arg)
                            {
                                arg = new TuiRef(parent);
                            }
                            else
                            {
                                arg->retain();
                            }
                            
                            args->arrayObjects.push_back(arg);
                            
                            (*tokenPos)++;
                        }
                    }
                    
                    
                    TuiRef* functionResult = ((TuiFunction*)child)->call(args, parent, result, debugInfo);
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
                else
                {
                    // todo maybe ++
                    if(enclosingSetRef && isStringKey)
                    {
                        *enclosingSetRef = parent;
                        *setKey = ((TuiString*)keyConstant)->value;
                    }
                    
                    if(result && result->type() == child->type())
                    {
                        result->assign(child);
                    }
                    else
                    {
                        return child;
                    }
                }
                return nullptr;
                
            }
                break;
            case Tui_token_greaterThan:
            case Tui_token_lessThan:
            case Tui_token_greaterEqualTo:
            case Tui_token_lessEqualTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        double left = ((TuiNumber*)leftResult)->value;
                        (*tokenPos)++;
                        
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                TuiRef* leftResult = runExpression(expression, tokenPos, result, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                            TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, result, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef);
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
        if(callData->locals.count(token) != 0)
        {
            foundValue = callData->locals[token];
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
            return new TuiRef(parent);
        }
    }
    
    return nullptr;
}

TuiRef* TuiFunction::runStatement(TuiStatement* statement,
                                  TuiRef* result,
                                  TuiTable* parent,
                                  TuiTokenMap* tokenMap,
                                  TuiFunctionCallData* callData,
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
            TuiRef* newResult = runExpression(statement->expression, &tokenPos, existingValue, parent, tokenMap, callData, debugInfo);
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
            runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
        }
            break;
        case Tui_statement_type_value:
        {
            uint32_t tokenPos = 0;
            TuiRef* newResult = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
            if(newResult)
            {
                TuiRef* copiedResult = newResult->copy();
                parent->arrayObjects.push_back(copiedResult);
                newResult->release();
            }
        }
            break;
        case Tui_statement_type_varAssign:
        {
            uint32_t tokenPos = 0;
           // debugInfo->lineNumber = statement->lineNumber; //?
            
            std::string setKey = ""; //this is set only if a Tui_token_childByString is found eg. color.x
            int setIndex = -1;
            TuiRef* enclosingSetRef = nullptr; //this is set only if a Tui_token_childByString is found eg. color.x
            
            TuiRef* existingValue = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo, &setKey, &setIndex, &enclosingSetRef);
            tokenPos++;
            
            TuiRef* newValue = runExpression(statement->expression, &tokenPos, existingValue, parent, tokenMap, callData, debugInfo);
            
            if(newValue)
            {
                if(enclosingSetRef)
                {
                    switch(enclosingSetRef->type())
                    {
                        case Tui_ref_type_TABLE:
                        {
                            ((TuiTable*)enclosingSetRef)->set(statement->varName, newValue);
                        }
                            break;
                        case Tui_ref_type_VEC2:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(setKey[0])
                            {
                                case 'x':
                                    ((TuiVec2*)enclosingSetRef)->value.x = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'y':
                                    ((TuiVec2*)enclosingSetRef)->value.y = ((TuiNumber*)newValue)->value;
                                    break;
                            }
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(setKey[0])
                            {
                                case 'x':
                                    ((TuiVec3*)enclosingSetRef)->value.x = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'y':
                                    ((TuiVec3*)enclosingSetRef)->value.y = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'z':
                                    ((TuiVec3*)enclosingSetRef)->value.z = ((TuiNumber*)newValue)->value;
                                    break;
                            }
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(setKey[0])
                            {
                                case 'x':
                                    ((TuiVec4*)enclosingSetRef)->value.x = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'y':
                                    ((TuiVec4*)enclosingSetRef)->value.y = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'z':
                                    ((TuiVec4*)enclosingSetRef)->value.z = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'w':
                                    ((TuiVec4*)enclosingSetRef)->value.w = ((TuiNumber*)newValue)->value;
                                    break;
                            }
                        }
                            break;
                        default:
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected table or vec object when accessing sub-value, got:%s", (enclosingSetRef ? enclosingSetRef->getDebugString().c_str() : "nil"));
                            return nullptr;
                        }
                            break;
                    }
                }
                else
                {
                    newValue = newValue->copy();
                    parent->set(statement->varName, newValue); //set a new local
                    newValue->release();
                }
                    
                uint32_t token = 0;
                if(tokenMap->localTokensByVarName.count(statement->varName) != 0)
                {
                    token = tokenMap->localTokensByVarName[statement->varName];
                }
                else if(tokenMap->capturedTokensByVarName.count(statement->varName) != 0)
                {
                    token = tokenMap->capturedTokensByVarName[statement->varName];
                }
                
                TuiRef* prevValue = nullptr;
                if(callData->locals.count(token) != 0)
                {
                    prevValue = callData->locals[token];
                }
                
                if(newValue)
                {
                    newValue->retain();
                    callData->locals[token] = newValue;
                }
                else
                {
                    callData->locals.erase(token);
                }
                
                if(prevValue)
                {
                    prevValue->release();
                }
            }
        }
            break;
        case Tui_statement_type_functionCall:
        {
            uint32_t tokenPos = 0;
            //debugInfo->lineNumber = statement->lineNumber; //?
            TuiRef* result = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
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
            
            TuiRef* collectionRef = runExpression(forStatement->expression, &containerTokenPos, nullptr, parent, tokenMap, callData, debugInfo);
            
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
                    callData->locals[indexToken] = indexNumber;
                }
                
                int i = 0;
                for(TuiRef* object : containerObject->arrayObjects)
                {
                    if(indexNumber)
                    {
                        indexNumber->value = i++;
                    }
                    
                    callData->locals[objectToken] = object;
                    
                    TuiRef* runResult = runStatementArray(forStatement->statements, result, parent, tokenMap, callData, debugInfo);
                    
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
                    callData->locals[indexToken] = keyString;
                }
                
                for(auto& kv : containerObject->objectsByStringKey)
                {
                    if(keyString)
                    {
                        keyString->value = kv.first;
                    }
                    
                    callData->locals[objectToken] = kv.second;
                    
                    TuiRef* runResult = runStatementArray(forStatement->statements, result, parent, tokenMap, callData, debugInfo);
                    
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
                TuiRef* runResult = runStatement(forStatement->initialStatement, nullptr, parent, tokenMap, callData, debugInfo);
                if(runResult)
                {
                    runResult->release();
                }
            }
            
            tokenPos = 0;
            TuiRef* continueResult = runExpression(forStatement->continueExpression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
            while(continueResult && continueResult->boolValue())
            {
                TuiRef* runResult = runStatementArray(forStatement->statements, nullptr, parent, tokenMap, callData, debugInfo);
                
                if(runResult)
                {
                    return runResult;
                }
                
                if(forStatement->incrementStatement)
                {
                    TuiRef* runResult = runStatement(forStatement->incrementStatement, nullptr, parent, tokenMap, callData, debugInfo);
                    if(runResult)
                    {
                        runResult->release();
                    }
                }
                
                tokenPos = 0;
                TuiRef* newResult = runExpression(forStatement->continueExpression, &tokenPos, continueResult, parent, tokenMap, callData, debugInfo);
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
                TuiRef* expressionResult = runExpression(currentSatement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                
                bool expressionPass = true;
                if(expressionResult)
                {
                    expressionPass = expressionResult->boolValue();
                }
                
                if(expressionPass)
                {
                    TuiRef* runResult = runStatementArray(currentSatement->statements, result, parent, tokenMap, callData, debugInfo);
                    
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
                            TuiRef* runResult = runStatementArray(currentSatement->elseIfStatement->statements, result, parent, tokenMap, callData, debugInfo);
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
                                       TuiTable* parent,
                                       TuiTokenMap* tokenMap,
                                       TuiFunctionCallData* callData,
                                       TuiDebugInfo* debugInfo) //static
{
    for(TuiStatement* statement : statements_)
    {
        TuiRef* newResult = TuiFunction::runStatement(statement, result, parent, tokenMap, callData, debugInfo);
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


TuiRef* TuiFunction::runTableConstruct(TuiTable* state,
             TuiRef* existingResult,
             TuiDebugInfo* callingDebugInfo)
{
    TuiFunctionCallData callData;
    
    // the code below up until calling the statement is very similar to TuiTable for() loop parsing
    // changes made here should probably be made there or it all could be factored out.
    
    
    for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
    {
        if(callData.locals.count(varNameAndToken.second) == 0)
        {
            TuiTable* parentRef = parent;
            while(parentRef)
            {
                if(parentRef->objectsByStringKey.count(varNameAndToken.first) != 0)
                {
                    TuiRef* var = parentRef->objectsByStringKey[varNameAndToken.first];
                    var->retain();//todo release this
                    callData.locals[varNameAndToken.second] = var;
                    break;
                }
                parentRef = parentRef->parent;
            }
        }
    }
    
    for(auto& parentDepthAndToken : tokenMap.capturedParentTokensByDepthCount)
    {
        if(callData.locals.count(parentDepthAndToken.first) == 0)
        {
            TuiTable* parentRef = parent;
            for(int i = 1; parentRef && i <= parentDepthAndToken.first; i++)
            {
                if(tokenMap.capturedParentTokensByDepthCount.count(i) != 0)
                {
                    uint32_t token = tokenMap.capturedParentTokensByDepthCount[i];
                    if(callData.locals.count(token) == 0)
                    {
                        parentRef->retain();//todo release this
                        callData.locals[token] = parentRef;
                    }
                }
                parentRef = parentRef->parent;
            }
        }
    }
    
    TuiTable* functionStateTable = new TuiTable(state);
    
    TuiRef* result = runStatementArray(statements,  existingResult, functionStateTable, &tokenMap, &callData, &debugInfo);
    if(result)
    {
        result->release();
    }
    
    return functionStateTable;
}

TuiRef* TuiFunction::call(TuiTable* args,
                          TuiTable* callLocationState,
                          TuiRef* existingResult,
                          TuiDebugInfo* callingDebugInfo)
{
    if(func)
    {
        TuiRef* result = func(args, parent, existingResult, callingDebugInfo);
        if(result)
        {
            result->retain();
        }
        return result;
    }
    else
    {
        
        TuiFunctionCallData callData;
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
                if(tokenMap.capturedTokensByVarName.count(argName) != 0)
                {
                    arg->retain();//todo release this
                    callData.locals[tokenMap.capturedTokensByVarName[argName]] = arg;
                }
                i++;
            }
        }
        
        // the code below up until calling the statement is very similar to TuiTable for() loop parsing
        // changes made here should probably be made there or it all could be factored out.
        
        
        for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
        {
            if(callData.locals.count(varNameAndToken.second) == 0)
            {
                TuiTable* parentRef = parent;
                while(parentRef)
                {
                    if(parentRef->objectsByStringKey.count(varNameAndToken.first) != 0)
                    {
                        TuiRef* var = parentRef->objectsByStringKey[varNameAndToken.first];
                        var->retain();//todo release this
                        callData.locals[varNameAndToken.second] = var;
                        break;
                    }
                    parentRef = parentRef->parent;
                }
            }
        }
        
        for(auto& parentDepthAndToken : tokenMap.capturedParentTokensByDepthCount)
        {
            if(callData.locals.count(parentDepthAndToken.first) == 0)
            {
                TuiTable* parentRef = parent;
                for(int i = 1; parentRef && i <= parentDepthAndToken.first; i++)
                {
                    if(tokenMap.capturedParentTokensByDepthCount.count(i) != 0)
                    {
                        uint32_t token = tokenMap.capturedParentTokensByDepthCount[i];
                        if(callData.locals.count(token) == 0)
                        {
                            parentRef->retain();//todo release this
                            callData.locals[token] = parentRef;
                        }
                    }
                    parentRef = parentRef->parent;
                }
            }
        }
        
        TuiTable* functionStateTable = new TuiTable(parent);
        
        TuiRef* result = runStatementArray(statements,  existingResult, functionStateTable, &tokenMap, &callData, &debugInfo);
        
        functionStateTable->release();
        
        return result;
    }
}
