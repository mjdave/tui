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
                            
                            TuiString* stringConstant = new TuiString(stringBuffer);
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
        else if(!stringBuffer.empty())
        {
            stringBuffer += *s;
        }
        else if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            double value = strtod(s, endptr);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            TuiNumber* number = new TuiNumber(value);
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
            bool success = TuiFunction::serializeFunctionBody(s, endptr, subParent, &constructorFunction->tokenMap, &constructorFunction->debugInfo, true, &constructorFunction->statements);
            
            if(!success)
            {
                return;
            }
            debugInfo->lineNumber = constructorFunction->debugInfo.lineNumber;
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            *endptr = (char*)s;
            break;
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
            
            TuiFunction* functionRef = TuiFunction::initWithHumanReadableString(s, endptr, parent, debugInfo);
            if(functionRef)
            {
                foundBuiltInType = true;
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_functionDeclaration);
                uint32_t functionToken = tokenMap->tokenIndex++;
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, functionToken);
                tokenMap->refsByToken[functionToken] = functionRef;
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                break;
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
             */
            
            
            if(!foundBuiltInType)
            {
                stringBuffer += *s;
            }
        }
    }
    
    
    if(singleQuote || doubleQuote)
    {
        TuiString* stringConstant = new TuiString(stringBuffer);
        uint32_t stringConstantToken = tokenMap->tokenIndex++;
        tokenMap->refsByToken[stringConstantToken] = stringConstant;
        expression->tokens.insert(expression->tokens.begin() + tokenPos++, stringConstantToken);
    }
    else if(!stringBuffer.empty())
    {
        if(hasFoundInitialValue)
        {
            if(!varChainStarted)
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_varChain); //note tokenStartPos to place before all other tokens
                tokenPos++;
                varChainStarted = true;
            }
            TuiString* stringConstant = new TuiString(stringBuffer);
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
        else if(*s == '-')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo, true);
            expression->tokens.push_back(Tui_token_negate);
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
            *endptr = (char*)s;
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
            case '%':
            {
                expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_modulo);
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
    
    if(expression->tokens.empty())
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpcted character:%c", *s);
        return nullptr;
    }
    
    if(expression->tokens[0] == Tui_token_functionCall)
    {
        //TuiError("todo");
        statement = new TuiStatement(Tui_statement_type_functionCall);
        statement->debugInfo = *debugInfo;
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
        
        statement->debugInfo = *debugInfo;
        statement->expression = expression;
    }
    else
    {
        TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default, nullptr, nullptr, 0);
        statement = new TuiStatement(Tui_statement_type_value);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        statement->debugInfo = *debugInfo;
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
        forStatement->debugInfo = *debugInfo;
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
        *endptr = (char*)s;
        
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
            forStatement->debugInfo = *debugInfo;
            forStatement->expression = expression;
            
            s++; // ','
            s = tuiSkipToNextChar(s, debugInfo);
            serializeValue(s, endptr, expression, parent, tokenMap, 1, debugInfo, nullptr); //store object var at index 1
            s = tuiSkipToNextChar(*endptr, debugInfo);
            *endptr = (char*)s;
        }
        else // for(object in table)
        {
            forStatement = new TuiForContainerLoopStatement(Tui_statement_type_forValues);
            resultStatement = forStatement;
            forStatement->debugInfo = *debugInfo;
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
        *endptr = (char*)s;
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
            statement->debugInfo = *debugInfo;
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
                        currentStatement->elseIfStatement->debugInfo = *debugInfo;
                        
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
                        currentStatement->elseIfStatement->debugInfo = *debugInfo;
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
                statement->debugInfo = *debugInfo;
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

TuiFunction* TuiFunction::initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo) //assumes that '(' is currently in str.
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
       && *(tuiSkipToNextChar(s + 8)) == '('
       )
    {
        s+=8;
        s = tuiSkipToNextChar(s, debugInfo);
        s++;
        //s = tuiSkipToNextChar(s, debugInfo);
        
        //*endptr = (char*)s;
        
        
        parent = new TuiTable(parent); //todo memeory leak?
        
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
                                   TuiRef** enclosingSetRef,
                                   std::string* subTypeAccessKey,
                                   TuiRef** subTypeRef)
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
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiTable* args = nullptr;
                TuiRef* arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(arg)
                {
                    args = new TuiTable(parent);
                    args->arrayObjects.push_back(arg);
                    
                    (*tokenPos)++;
                    
                    while(expression->tokens[*tokenPos] != Tui_token_end)
                    {
                        arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!arg)
                        {
                            arg = TUI_NIL;
                        }
                        
                        args->arrayObjects.push_back(arg);
                        
                        (*tokenPos)++;
                    }
                    (*tokenPos)++;
                }
                
                
                TuiRef* functionResult = ((TuiFunction*)functionVar)->call(args, result, debugInfo);
                if(args)
                {
                    args->release();
                }
                
                functionVar->release();
                
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
                TuiRef* x = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!x || x->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "x component expected number, got:%s", (x ? x->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                (*tokenPos)++;
                TuiRef* y = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
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
                                x->release();
                                y->release();
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
                            (*tokenPos)++;
                            TuiRef* z = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                            
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
                                    x->release();
                                    y->release();
                                    z->release();
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
                                (*tokenPos)++;
                                TuiRef* w = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                                
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
                                    x->release();
                                    y->release();
                                    z->release();
                                    w->release();
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
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                (*tokenPos)++;
                
                TuiRef* functionResult = ((TuiFunction*)functionVar)->runTableConstruct(parent, result, debugInfo);
                functionVar->release();
                
                return functionResult;
            }
                break;
            case Tui_token_functionDeclaration:
            {
                (*tokenPos)++;
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                parent->retain();
                if(functionVar->parentTable)
                {
                    functionVar->parentTable->release();
                }
                functionVar->parentTable = parent;// experimental, may have side effects
                
                if(result && result->type() == functionVar->type())
                {
                    result->assign(functionVar);
                    functionVar->release();
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!leftResult)
                {
                    bool boolResult = !rightResult || rightResult->type() == Tui_ref_type_NIL;
                    if(rightResult)
                    {
                        rightResult->release();
                    }
                    return TUI_BOOL(boolResult);
                }
                
                bool boolResult = leftResult->isEqual(rightResult);
                if(rightResult)
                {
                    rightResult->release();
                }
                leftResult->release();
                
                return TUI_BOOL(boolResult);
            }
                break;
            case Tui_token_or:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                
                if(leftResult && leftResult->boolValue())
                {
                    leftResult->release();
                    return TUI_TRUE;
                }
                else
                {
                    if(leftResult)
                    {
                        leftResult->release();
                    }
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    TuiBool* returnValue = TUI_BOOL(rightResult && rightResult->boolValue());
                    if(rightResult)
                    {
                        rightResult->release();
                    }
                    return returnValue;
                }
                
            }
                break;
            case Tui_token_and:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                
                if(!leftResult || !leftResult->boolValue())
                {
                    if(leftResult)
                    {
                        leftResult->release();
                    }
                    return TUI_FALSE;
                }
                else
                {
                    leftResult->release();
                    
                    TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    
                    TuiBool* returnValue = TUI_BOOL(rightResult && rightResult->boolValue());
                    if(rightResult)
                    {
                        rightResult->release();
                    }
                    return returnValue;
                    
                }
                
            }
                break;
            case Tui_token_notEqualTo:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!leftResult)
                {
                    TuiBool* returnValue = TUI_BOOL(rightResult && rightResult->type() != Tui_ref_type_NIL);
                    rightResult->release();
                    return returnValue;
                }
                TuiBool* returnValue = TUI_BOOL(!leftResult->isEqual(rightResult));
                leftResult->release();
                if(rightResult)
                {
                    rightResult->release();
                }
                return returnValue;
            }
                break;
            case Tui_token_not:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                if(expression->tokens[*tokenPos+1] == Tui_token_end)
                {
                    (*tokenPos)++; //Tui_token_end
                }
                else
                {
                    TuiError("expected Tui_token_end");
                }
                TuiBool* returnValue = TuiRef::logicalNot(leftResult);
                leftResult->release();
                return returnValue;
            }
                break;
            case Tui_token_negate:
            {
                (*tokenPos)++;
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                if(expression->tokens[*tokenPos+1] == Tui_token_end)
                {
                    (*tokenPos)++; //Tui_token_end
                }
                else
                {
                    TuiError("expected Tui_token_end");
                }
                
                if(!leftResult)
                {
                    TuiError("expected value");
                    return nullptr;
                }
                
                switch (leftResult->type()) {
                    case Tui_ref_type_NUMBER:
                    {
                        if(result && result->type() == Tui_ref_type_NUMBER)
                        {
                            ((TuiNumber*)result)->value = -((TuiNumber*)leftResult)->value;
                            leftResult->release();
                            return nullptr;
                        }
                        TuiNumber* returnResult = new TuiNumber(-((TuiNumber*)leftResult)->value);
                        leftResult->release();
                        return returnResult;
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        if(result && result->type() == Tui_ref_type_VEC2)
                        {
                            ((TuiVec2*)result)->value = -((TuiVec2*)leftResult)->value;
                            leftResult->release();
                            return nullptr;
                        }
                        TuiVec2* returnResult = new TuiVec2(-((TuiVec2*)leftResult)->value);
                        leftResult->release();
                        return returnResult;
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        if(result && result->type() == Tui_ref_type_VEC3)
                        {
                            ((TuiVec3*)result)->value = -((TuiVec3*)leftResult)->value;
                            leftResult->release();
                            return nullptr;
                        }
                        TuiVec3* returnResult = new TuiVec3(-((TuiVec3*)leftResult)->value);
                        leftResult->release();
                        return returnResult;
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        if(result && result->type() == Tui_ref_type_VEC4)
                        {
                            ((TuiVec4*)result)->value = -((TuiVec4*)leftResult)->value;
                            leftResult->release();
                            return nullptr;
                        }
                        TuiVec4* returnResult = new TuiVec4(-((TuiVec4*)leftResult)->value);
                        leftResult->release();
                        return returnResult;
                    }
                        break;
                        
                    default:
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number or vector, got:%s", (leftResult ? leftResult->getDebugString().c_str() : "nil"));
                        break;
                }
                return nullptr;
            }
                break;
            case Tui_token_true:
            {
                return TUI_TRUE;
            }
                break;
            case Tui_token_false:
            {
                return TUI_FALSE;
            }
                break;
            case Tui_token_varChain:
            {
                (*tokenPos)++;
                TuiRef* chainParent = nullptr;
                TuiRef* chainResult = parent;
                chainResult->retain();
                TuiRef* keyConstant = nullptr;
                
                while(1)
                {
                    switch(chainResult->type())
                    {
                        case Tui_ref_type_TABLE:
                        {
                            if(chainParent)
                            {
                                chainParent->release();
                            }
                            chainParent = chainResult;
                            chainResult = runExpression(expression, tokenPos, nullptr, (TuiTable*)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        case Tui_ref_type_VEC2:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, (TuiTable*)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, (TuiTable*)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, (TuiTable*)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        default:
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "no '.' syntax supported for type:%s (might also be attempting to call a nil functon)", chainResult->getTypeName().c_str());
                            return nullptr;
                        }
                            break;
                    }
                    
                    (*tokenPos)++;
                    
                    /*if(!chainResult) //this catches false positives. A function call foo.bar() maybe returns no result, but has done its job
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to use '.' syntax on nil value");
                        return nullptr;
                    }*/
                    
                    if(expression->tokens[*tokenPos] == Tui_token_end)
                    {
                        break;
                    }
                }
                
                if(keyConstant && chainResult->type() != Tui_ref_type_TABLE)
                {
                    if(subTypeRef)
                    {
                        TuiRef* prevRef = *subTypeRef;
                        *subTypeRef = chainResult;
                        chainResult->retain();
                        if(prevRef)
                        {
                            prevRef->release();
                        }
                        
                        if(subTypeAccessKey)
                        {
                            *subTypeAccessKey = ((TuiString*)keyConstant)->value;
                        }
                    }
                    
                    if(chainParent)
                    {
                        chainParent->release();
                        chainParent = nullptr;
                    }
                    
                    switch(chainResult->type())
                    {
                        case Tui_ref_type_VEC2:
                        {
                            switch(((TuiString*)keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.x;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec2*)chainResult)->value.x);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.y;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec2*)chainResult)->value.y);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
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
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.x;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec3*)chainResult)->value.x);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.y;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec3*)chainResult)->value.y);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
                                    return nullptr;
                                }
                                    break;
                                case 'z':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.z;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec3*)chainResult)->value.z);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
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
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.x;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec4*)chainResult)->value.x);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.y;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec4*)chainResult)->value.y);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
                                    return nullptr;
                                }
                                    break;
                                case 'z':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec3*)chainResult)->value.z;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec4*)chainResult)->value.z);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
                                    return nullptr;
                                }
                                    break;
                                case 'w':
                                {
                                    keyConstant->release();
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        ((TuiNumber*)result)->value = ((TuiVec4*)chainResult)->value.w;
                                        chainResult->release();
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiVec4*)chainResult)->value.w);
                                        chainResult->release();
                                        return returnResult;
                                    }
                                    chainResult->release();
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
                    TuiRef* prevRef = *enclosingSetRef;
                    *enclosingSetRef = chainParent;
                    if(prevRef)
                    {
                        prevRef->release();
                    }
                }
                else if(chainParent)
                {
                    chainParent->release();
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
                
                TuiRef* keyConstant = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
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
                        
                        if(setKey)
                        {
                            *setKey = ((TuiString*)keyConstant)->value;
                        }
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
                        
                        if(setIndex)
                        {
                            *setIndex = arrayIndex;
                        }
                    }
                }
                
                if(!child)
                {
                    if(isFunctionCall)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Attempting to call a nil function");
                        return nullptr;
                    }
                    keyConstant->release();
                    return TUI_NIL;
                }
                
                if(isFunctionCall)
                {
                    keyConstant->release();
                    
                    if(child->type() != Tui_ref_type_FUNCTION)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected function, got:%s", (child ? child->getDebugString().c_str() : "nil"));
                        return nullptr;
                    }
                    
                    TuiTable* args = nullptr;
                    (*tokenPos)++;
                    TuiRef* arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    
                    if(arg)
                    {
                        args = new TuiTable(parent);
                        args->arrayObjects.push_back(arg);
                        
                        (*tokenPos)++;
                        
                        while(expression->tokens[*tokenPos] != Tui_token_end)
                        {
                            arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                            if(!arg)
                            {
                                arg = TUI_NIL;
                            }
                            else
                            {
                                arg->retain();
                            }
                            
                            args->arrayObjects.push_back(arg);
                            
                            (*tokenPos)++;
                        }
                    }
                    
                    
                    TuiRef* functionResult = ((TuiFunction*)child)->call(args, result, debugInfo);
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
                        TuiRef* prevRef = *enclosingSetRef;
                        *enclosingSetRef = parent;
                        parent->retain();
                        if(prevRef)
                        {
                            prevRef->release();
                        }
                        
                        *setKey = ((TuiString*)keyConstant)->value;
                    }
                    
                    keyConstant->release();
                    
                    if(result && result->type() == child->type())
                    {
                        result->assign(child);
                    }
                    else
                    {
                        child->retain();
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
                TuiRef* leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        (*tokenPos)++;
                        
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(rightResult->type() != leftType)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                            return nullptr;
                        }
                        
                        switch (token) {
                            case Tui_token_greaterThan:
                            {
                                TuiBool* returnResult = TUI_BOOL(((TuiNumber*)leftResult)->value > ((TuiNumber*)rightResult)->value);
                                leftResult->release();
                                rightResult->release();
                                return returnResult;
                            }
                                break;
                            case Tui_token_lessThan:
                            {
                                TuiBool* returnResult = TUI_BOOL(((TuiNumber*)leftResult)->value < ((TuiNumber*)rightResult)->value);
                                leftResult->release();
                                rightResult->release();
                                return returnResult;
                            }
                                break;
                            case Tui_token_greaterEqualTo:
                            {
                                TuiBool* returnResult = TUI_BOOL(((TuiNumber*)leftResult)->value >= ((TuiNumber*)rightResult)->value);
                                leftResult->release();
                                rightResult->release();
                                return returnResult;
                            }
                                break;
                            case Tui_token_lessEqualTo:
                            {
                                TuiBool* returnResult = TUI_BOOL(((TuiNumber*)leftResult)->value <= ((TuiNumber*)rightResult)->value);
                                leftResult->release();
                                rightResult->release();
                                return returnResult;
                            }
                                break;
                        };
                        
                    }
                        break;
                        
                    default:
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                        return nullptr;
                    }
                        break;
                }
            }
                break;
            case Tui_token_add:
            case Tui_token_subtract:
            case Tui_token_multiply:
            case Tui_token_divide:
            case Tui_token_modulo:
            case Tui_token_increment:
            case Tui_token_decrement:
            case Tui_token_addInPlace:
            case Tui_token_subtractInPlace:
            case Tui_token_multiplyInPlace:
            case Tui_token_divideInPlace:
            {
                (*tokenPos)++;
                
                std::string inPlaceSetKey;
                std::string inPlaceSubTypeAccessKey;
                TuiRef* inPlaceSubRef = nullptr;
                TuiRef* inPlaceEnclosingSetRef = nullptr;
                
                TuiRef* leftResult;
                if(token <= Tui_token_modulo) //covers Tui_token_add, Tui_token_subtract, Tui_token_divide, Tui_token_multiply, Tui_token_modulo
                {
                    leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                }
                else
                {
                    leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, &inPlaceSetKey, nullptr, &inPlaceEnclosingSetRef, &inPlaceSubTypeAccessKey, &inPlaceSubRef);
                }
                
                if(!leftResult)
                {
                    if(inPlaceSubRef)
                    {
                        TuiRef* rightResult = nullptr;
                        if(token != Tui_token_increment && token != Tui_token_decrement)
                        {
                            (*tokenPos)++;
                            rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                            if(rightResult->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                                return nullptr;
                            }
                        }
                        
                        switch(inPlaceSubRef->type())
                        {
                            case Tui_ref_type_VEC2:
                            {
                                switch (token)
                                {
                                    case Tui_token_increment:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec2*)inPlaceSubRef)->value.x++;
                                                break;
                                            case 'y':
                                                ((TuiVec2*)inPlaceSubRef)->value.y++;
                                                break;
                                        }
                                        break;
                                    case Tui_token_decrement:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec2*)inPlaceSubRef)->value.x--;
                                                break;
                                            case 'y':
                                                ((TuiVec2*)inPlaceSubRef)->value.y--;
                                                break;
                                        }
                                        break;
                                    case Tui_token_addInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec2*)inPlaceSubRef)->value.x += ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec2*)inPlaceSubRef)->value.y += ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                        break;
                                    case Tui_token_subtractInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec2*)inPlaceSubRef)->value.x -= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec2*)inPlaceSubRef)->value.y -= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                    case Tui_token_multiplyInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec2*)inPlaceSubRef)->value.x *= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec2*)inPlaceSubRef)->value.y *= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                    case Tui_token_divideInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec2*)inPlaceSubRef)->value.x /= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec2*)inPlaceSubRef)->value.y /= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                };
                            }
                                break;
                            case Tui_ref_type_VEC3:
                            {
                                switch (token)
                                {
                                    case Tui_token_increment:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec3*)inPlaceSubRef)->value.x++;
                                                break;
                                            case 'y':
                                                ((TuiVec3*)inPlaceSubRef)->value.y++;
                                                break;
                                            case 'z':
                                                ((TuiVec3*)inPlaceSubRef)->value.z++;
                                                break;
                                        }
                                        break;
                                    case Tui_token_decrement:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec3*)inPlaceSubRef)->value.x--;
                                                break;
                                            case 'y':
                                                ((TuiVec3*)inPlaceSubRef)->value.y--;
                                                break;
                                            case 'z':
                                                ((TuiVec3*)inPlaceSubRef)->value.z--;
                                                break;
                                        }
                                        break;
                                    case Tui_token_addInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec3*)inPlaceSubRef)->value.x += ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec3*)inPlaceSubRef)->value.y += ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec3*)inPlaceSubRef)->value.z += ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                        break;
                                    case Tui_token_subtractInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec3*)inPlaceSubRef)->value.x -= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec3*)inPlaceSubRef)->value.y -= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec3*)inPlaceSubRef)->value.z -= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                    case Tui_token_multiplyInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec3*)inPlaceSubRef)->value.x *= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec3*)inPlaceSubRef)->value.y *= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec3*)inPlaceSubRef)->value.z *= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                    case Tui_token_divideInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec3*)inPlaceSubRef)->value.x /= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec3*)inPlaceSubRef)->value.y /= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec3*)inPlaceSubRef)->value.z /= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                };
                            }
                                break;
                            case Tui_ref_type_VEC4:
                            {
                                switch (token)
                                {
                                    case Tui_token_increment:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec4*)inPlaceSubRef)->value.x++;
                                                break;
                                            case 'y':
                                                ((TuiVec4*)inPlaceSubRef)->value.y++;
                                                break;
                                            case 'z':
                                                ((TuiVec4*)inPlaceSubRef)->value.z++;
                                                break;
                                            case 'w':
                                                ((TuiVec4*)inPlaceSubRef)->value.w++;
                                                break;
                                        }
                                        break;
                                    case Tui_token_decrement:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec4*)inPlaceSubRef)->value.x--;
                                                break;
                                            case 'y':
                                                ((TuiVec4*)inPlaceSubRef)->value.y--;
                                                break;
                                            case 'z':
                                                ((TuiVec4*)inPlaceSubRef)->value.z--;
                                                break;
                                            case 'w':
                                                ((TuiVec4*)inPlaceSubRef)->value.w--;
                                                break;
                                        }
                                        break;
                                    case Tui_token_addInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec4*)inPlaceSubRef)->value.x += ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec4*)inPlaceSubRef)->value.y += ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec4*)inPlaceSubRef)->value.z += ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'w':
                                                ((TuiVec4*)inPlaceSubRef)->value.w += ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                        break;
                                    case Tui_token_subtractInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec4*)inPlaceSubRef)->value.x -= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec4*)inPlaceSubRef)->value.y -= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec4*)inPlaceSubRef)->value.z -= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'w':
                                                ((TuiVec4*)inPlaceSubRef)->value.w -= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                    case Tui_token_multiplyInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec4*)inPlaceSubRef)->value.x *= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec4*)inPlaceSubRef)->value.y *= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec4*)inPlaceSubRef)->value.z *= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'w':
                                                ((TuiVec4*)inPlaceSubRef)->value.w *= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                    case Tui_token_divideInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                ((TuiVec4*)inPlaceSubRef)->value.x /= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'y':
                                                ((TuiVec4*)inPlaceSubRef)->value.y /= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'z':
                                                ((TuiVec4*)inPlaceSubRef)->value.z /= ((TuiNumber*)rightResult)->value;
                                                break;
                                            case 'w':
                                                ((TuiVec4*)inPlaceSubRef)->value.w /= ((TuiNumber*)rightResult)->value;
                                                break;
                                        }
                                    break;
                                };
                            }
                                break;
                        }
                        
                        if(rightResult)
                        {
                            rightResult->release();
                        }
                        
                        if(((TuiTable*)inPlaceEnclosingSetRef)->onSet)
                        {
                            ((TuiTable*)inPlaceEnclosingSetRef)->onSet(inPlaceEnclosingSetRef, inPlaceSetKey, inPlaceSubRef);
                        }
                        
                        inPlaceEnclosingSetRef->release();
                        
                        inPlaceSubRef->release();
                        
                        return nullptr;
                    }
                    else
                    {
                        //leftResult = result;
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "invalid value");
                        return nullptr;
                    }
                    
                }
                
                if(!leftResult)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "invalid value");
                    return nullptr;
                }
                
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        if(token == Tui_token_increment)
                        {
                            ((TuiNumber*)leftResult)->value++;
                            if(inPlaceEnclosingSetRef && ((TuiTable*)inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                            {
                                ((TuiTable*)inPlaceEnclosingSetRef)->onSet(((TuiTable*)inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                            }
                            if(inPlaceEnclosingSetRef)
                            {
                                inPlaceEnclosingSetRef->release();
                            }
                            leftResult->release();
                            return nullptr;
                        }
                        else
                        {
                            (*tokenPos)++;
                            TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                            if(!rightResult || rightResult->type() != leftType)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                                return nullptr;
                            }
                            
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        ((TuiNumber*)result)->value = ((TuiNumber*)leftResult)->value + ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_addInPlace:
                                        ((TuiNumber*)result)->value += ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_subtract:
                                        ((TuiNumber*)result)->value = ((TuiNumber*)leftResult)->value - ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiNumber*)result)->value -= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiply:
                                        ((TuiNumber*)result)->value = ((TuiNumber*)leftResult)->value * ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiNumber*)result)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiNumber*)result)->value = ((TuiNumber*)leftResult)->value / ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiNumber*)result)->value /= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_modulo:
                                        ((TuiNumber*)result)->value = ((int)((TuiNumber*)leftResult)->value) % ((int)((TuiNumber*)rightResult)->value);
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiNumber*)leftResult)->value + ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiNumber*)leftResult)->value - ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiNumber*)leftResult)->value * ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((TuiNumber*)leftResult)->value / ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_modulo:
                                    {
                                        TuiNumber* returnResult = new TuiNumber(((int)((TuiNumber*)leftResult)->value) % ((int)((TuiNumber*)rightResult)->value));
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
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
                            
                            
                            if(inPlaceEnclosingSetRef && ((TuiTable*)inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                            {
                                ((TuiTable*)inPlaceEnclosingSetRef)->onSet(((TuiTable*)inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                            }
                            
                            rightResult->release();
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected value");
                            return nullptr;
                        }
                        
                        
                        if(rightResult->type() == leftType)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        ((TuiVec2*)result)->value = ((TuiVec2*)leftResult)->value + ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_addInPlace:
                                        ((TuiVec2*)result)->value += ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_subtract:
                                        ((TuiVec2*)result)->value = ((TuiVec2*)leftResult)->value - ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiVec2*)result)->value -= ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_multiply:
                                        ((TuiVec2*)result)->value = ((TuiVec2*)leftResult)->value * ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec2*)result)->value *= ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiVec2*)result)->value = ((TuiVec2*)leftResult)->value / ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec2*)result)->value /= ((TuiVec2*)rightResult)->value;
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        TuiVec2* returnResult = new TuiVec2(((TuiVec2*)leftResult)->value + ((TuiVec2*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        TuiVec2* returnResult = new TuiVec2(((TuiVec2*)leftResult)->value - ((TuiVec2*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        TuiVec2* returnResult = new TuiVec2(((TuiVec2*)leftResult)->value * ((TuiVec2*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiVec2* returnResult = new TuiVec2(((TuiVec2*)leftResult)->value / ((TuiVec2*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        ((TuiVec2*)leftResult)->value += ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiVec2*)leftResult)->value -= ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec2*)leftResult)->value *= ((TuiVec2*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec2*)leftResult)->value /= ((TuiVec2*)rightResult)->value;
                                        break;
                                }
                            }
                        }
                        else if(rightResult->type() == Tui_ref_type_NUMBER)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                        
                                    case Tui_token_multiply:
                                        ((TuiVec2*)result)->value = ((TuiVec2*)leftResult)->value * ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec2*)result)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiVec2*)result)->value = ((TuiVec2*)leftResult)->value / ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec2*)result)->value /= ((TuiNumber*)rightResult)->value;
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
                                
                                switch (token) {
                                        
                                    case Tui_token_multiply:
                                    {
                                        TuiVec2* returnResult = new TuiVec2(((TuiVec2*)leftResult)->value * ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiVec2* returnResult = new TuiVec2(((TuiVec2*)leftResult)->value / ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec2*)leftResult)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec2*)leftResult)->value /= ((TuiNumber*)rightResult)->value;
                                        break;
                                    default:
                                    {
                                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to add or subtract number from vec2");
                                        return nullptr;
                                    }
                                        break;
                                };
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected vec2 or number");
                            return nullptr;
                        }
                        
                        
                        
                        if(inPlaceEnclosingSetRef && ((TuiTable*)inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            ((TuiTable*)inPlaceEnclosingSetRef)->onSet(((TuiTable*)inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                        rightResult->release();
                        
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected value");
                            return nullptr;
                        }
                        
                        
                        if(rightResult->type() == leftType)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        ((TuiVec3*)result)->value = ((TuiVec3*)leftResult)->value + ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_addInPlace:
                                        ((TuiVec3*)result)->value += ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_subtract:
                                        ((TuiVec3*)result)->value = ((TuiVec3*)leftResult)->value - ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiVec3*)result)->value -= ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_multiply:
                                        ((TuiVec3*)result)->value = ((TuiVec3*)leftResult)->value * ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec3*)result)->value *= ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiVec3*)result)->value = ((TuiVec3*)leftResult)->value / ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec3*)result)->value /= ((TuiVec3*)rightResult)->value;
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        TuiVec3* returnResult = new TuiVec3(((TuiVec3*)leftResult)->value + ((TuiVec3*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        TuiVec3* returnResult = new TuiVec3(((TuiVec3*)leftResult)->value - ((TuiVec3*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        TuiVec3* returnResult = new TuiVec3(((TuiVec3*)leftResult)->value * ((TuiVec3*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiVec3* returnResult = new TuiVec3(((TuiVec3*)leftResult)->value / ((TuiVec3*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        ((TuiVec3*)leftResult)->value += ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiVec3*)leftResult)->value -= ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec3*)leftResult)->value *= ((TuiVec3*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec3*)leftResult)->value /= ((TuiVec3*)rightResult)->value;
                                        break;
                                }
                            }
                        }
                        else if(rightResult->type() == Tui_ref_type_NUMBER)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                        
                                    case Tui_token_multiply:
                                        ((TuiVec3*)result)->value = ((TuiVec3*)leftResult)->value * ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec3*)result)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiVec3*)result)->value = ((TuiVec3*)leftResult)->value / ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec3*)result)->value /= ((TuiNumber*)rightResult)->value;
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
                                
                                switch (token) {
                                        
                                    case Tui_token_multiply:
                                    {
                                        TuiVec3* returnResult = new TuiVec3(((TuiVec3*)leftResult)->value * ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiVec3* returnResult = new TuiVec3(((TuiVec3*)leftResult)->value / ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec3*)leftResult)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec3*)leftResult)->value /= ((TuiNumber*)rightResult)->value;
                                        break;
                                    default:
                                    {
                                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to add or subtract number from vec3");
                                        return nullptr;
                                    }
                                        break;
                                };
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected vec2 or number");
                            return nullptr;
                        }
                        
                        
                        if(inPlaceEnclosingSetRef && ((TuiTable*)inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            ((TuiTable*)inPlaceEnclosingSetRef)->onSet(((TuiTable*)inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                        
                        rightResult->release();
                        
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected value");
                            return nullptr;
                        }
                        
                        
                        if(rightResult->type() == leftType)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        ((TuiVec4*)result)->value = ((TuiVec4*)leftResult)->value + ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_addInPlace:
                                        ((TuiVec4*)result)->value += ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_subtract:
                                        ((TuiVec4*)result)->value = ((TuiVec4*)leftResult)->value - ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiVec4*)result)->value -= ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_multiply:
                                        ((TuiVec4*)result)->value = ((TuiVec4*)leftResult)->value * ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec4*)result)->value *= ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiVec4*)result)->value = ((TuiVec4*)leftResult)->value / ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec4*)result)->value /= ((TuiVec4*)rightResult)->value;
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        TuiVec4* returnResult = new TuiVec4(((TuiVec4*)leftResult)->value + ((TuiVec4*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        TuiVec4* returnResult = new TuiVec4(((TuiVec4*)leftResult)->value - ((TuiVec4*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        TuiVec4* returnResult = new TuiVec4(((TuiVec4*)leftResult)->value * ((TuiVec4*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiVec4* returnResult = new TuiVec4(((TuiVec4*)leftResult)->value / ((TuiVec4*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        ((TuiVec4*)leftResult)->value += ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_subtractInPlace:
                                        ((TuiVec4*)leftResult)->value -= ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec4*)leftResult)->value *= ((TuiVec4*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec4*)leftResult)->value /= ((TuiVec4*)rightResult)->value;
                                        break;
                                }
                            }
                        }
                        else if(rightResult->type() == Tui_ref_type_NUMBER)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                        
                                    case Tui_token_multiply:
                                        ((TuiVec4*)result)->value = ((TuiVec4*)leftResult)->value * ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec4*)result)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divide:
                                        ((TuiVec4*)result)->value = ((TuiVec4*)leftResult)->value / ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec4*)result)->value /= ((TuiNumber*)rightResult)->value;
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
                                
                                switch (token) {
                                        
                                    case Tui_token_multiply:
                                    {
                                        TuiVec4* returnResult = new TuiVec4(((TuiVec4*)leftResult)->value * ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        TuiVec4* returnResult = new TuiVec4(((TuiVec4*)leftResult)->value / ((TuiNumber*)rightResult)->value);
                                        leftResult->release();
                                        rightResult->release();
                                        if(inPlaceEnclosingSetRef)
                                        {
                                            inPlaceEnclosingSetRef->release();
                                        }
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_multiplyInPlace:
                                        ((TuiVec4*)leftResult)->value *= ((TuiNumber*)rightResult)->value;
                                        break;
                                    case Tui_token_divideInPlace:
                                        ((TuiVec4*)leftResult)->value /= ((TuiNumber*)rightResult)->value;
                                        break;
                                    default:
                                    {
                                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to add or subtract number from vec4");
                                        return nullptr;
                                    }
                                        break;
                                };
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected vec4 or number");
                            return nullptr;
                        }
                        
                        if(inPlaceEnclosingSetRef && ((TuiTable*)inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            ((TuiTable*)inPlaceEnclosingSetRef)->onSet(((TuiTable*)inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                        rightResult->release();
                        
                    }
                        break;
                        
                    case Tui_ref_type_STRING: //left type string
                    {
                        (*tokenPos)++;
                        TuiRef* rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        if(result && result->type() == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    ((TuiString*)result)->value = ((TuiString*)leftResult)->value + rightResult->getStringValue();
                                    break;
                                case Tui_token_addInPlace:
                                    ((TuiString*)result)->value += rightResult->getStringValue();
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case Tui_token_add:
                                {
                                    TuiString* returnResult = new TuiString(((TuiString*)leftResult)->value + rightResult->getStringValue());
                                    leftResult->release();
                                    rightResult->release();
                                    if(inPlaceEnclosingSetRef)
                                    {
                                        inPlaceEnclosingSetRef->release();
                                    }
                                    return returnResult;
                                }
                                case Tui_token_addInPlace:
                                    ((TuiString*)leftResult)->value += rightResult->getStringValue();
                                    break;
                                    
                                default:
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Invalid token");
                                    break;
                            };
                        }
                        
                        
                        
                        if(inPlaceEnclosingSetRef && ((TuiTable*)inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            ((TuiTable*)inPlaceEnclosingSetRef)->onSet(((TuiTable*)inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                        rightResult->release();
                    }
                        break;
                        
                    default:
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "invalid value");
                    }
                        break;
                }
                
                
                leftResult->release();
                if(inPlaceEnclosingSetRef)
                {
                    inPlaceEnclosingSetRef->release();
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
            foundValue->retain();
        }
        else if(tokenMap->refsByToken.count(token) != 0)
        {
            foundValue = tokenMap->refsByToken[token];
            foundValue->retain();
        }
        
        if(foundValue)
        {
            if(result && result->type() == foundValue->type() && result->type() != Tui_ref_type_BOOL)// maybe? && (result->type() != Tui_ref_type_TABLE && result->type() != Tui_ref_type_FUNCTION))
            {
                result->assign(foundValue);
                foundValue->release();
            }
            else
            {
                return foundValue;
            }
        }
        else
        {
            return TUI_NIL;
        }
    }
    
    return nullptr;
}

TuiRef* TuiFunction::runStatement(TuiStatement* statement,
                                  TuiRef* result,
                                  TuiTable* parent,
                                  TuiTokenMap* tokenMap,
                                  TuiFunctionCallData* callData,
                                  TuiDebugInfo* callingDebugInfo)
{
    TuiDebugInfo* debugInfo = &statement->debugInfo;
    switch(statement->type)
    {
        case Tui_statement_type_return:
        {
            return TUI_NIL;
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
            return TUI_NIL;
        }
            break;
        case Tui_statement_type_varModify:
        {
            uint32_t tokenPos = 0;
            std::string setKey = ""; //this is set only if a Tui_token_childByString is found eg. color.x
            std::string subTypeSetKey = ""; //this is set only if a Tui_token_childByString is found eg. color.x
            int setIndex = -1;
            TuiRef* enclosingSetRef = nullptr;
            TuiRef* subTypeRef = nullptr; //this is set only if a Tui_token_childByString is found eg. color.x or foo.table
            
            runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo, &setKey, &setIndex, &enclosingSetRef, &subTypeSetKey, &subTypeRef);
            
            
            if(enclosingSetRef)
            {
                if(((TuiTable*)enclosingSetRef)->onSet && !setKey.empty())
                {
                    ((TuiTable*)enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                }
                enclosingSetRef->release();
                if(subTypeRef)
                {
                    subTypeRef->release();
                }
            }
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
            
            std::string setKey = "";
            std::string subTypeSetKey = ""; //this is set only if a Tui_token_childByString is found eg. color.x
            int setIndex = -1;
            TuiRef* enclosingSetRef = nullptr;
            TuiRef* subTypeRef = nullptr; //this is set only if a Tui_token_childByString is found eg. color.x or foo.table
            
            TuiRef* existingValue = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, debugInfo, &setKey, &setIndex, &enclosingSetRef, &subTypeSetKey, &subTypeRef);
            tokenPos++;
            
            TuiRef* newValue = runExpression(statement->expression, &tokenPos, existingValue, parent, tokenMap, callData, debugInfo);
            
            if(newValue)
            {
                if(subTypeRef)
                {
                    switch(subTypeRef->type())
                    {
                        case Tui_ref_type_TABLE:
                        {
                            TuiError("this shouldn't happen");
                        }
                            break;
                        case Tui_ref_type_VEC2:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(subTypeSetKey[0])
                            {
                                case 'x':
                                    ((TuiVec2*)subTypeRef)->value.x = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'y':
                                    ((TuiVec2*)subTypeRef)->value.y = ((TuiNumber*)newValue)->value;
                                    break;
                            }
                            
                            if(enclosingSetRef && ((TuiTable*)enclosingSetRef)->onSet && !setKey.empty())
                            {
                                ((TuiTable*)enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                            }
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(subTypeSetKey[0])
                            {
                                case 'x':
                                    ((TuiVec3*)subTypeRef)->value.x = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'y':
                                    ((TuiVec3*)subTypeRef)->value.y = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'z':
                                    ((TuiVec3*)subTypeRef)->value.z = ((TuiNumber*)newValue)->value;
                                    break;
                            }
                            
                            if(enclosingSetRef && ((TuiTable*)enclosingSetRef)->onSet && !setKey.empty())
                            {
                                ((TuiTable*)enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                            }
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(subTypeSetKey[0])
                            {
                                case 'x':
                                    ((TuiVec4*)subTypeRef)->value.x = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'y':
                                    ((TuiVec4*)subTypeRef)->value.y = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'z':
                                    ((TuiVec4*)subTypeRef)->value.z = ((TuiNumber*)newValue)->value;
                                    break;
                                case 'w':
                                    ((TuiVec4*)subTypeRef)->value.w = ((TuiNumber*)newValue)->value;
                                    break;
                            }
                            
                            if(enclosingSetRef && ((TuiTable*)enclosingSetRef)->onSet && !setKey.empty())
                            {
                                ((TuiTable*)enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                            }
                        }
                            break;
                        default:
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected table or vec object when accessing sub-value, got:%s", (subTypeRef ? subTypeRef->getDebugString().c_str() : "nil"));
                            return nullptr;
                        }
                            break;
                    }
                }
                else if(enclosingSetRef)
                {
                    ((TuiTable*)enclosingSetRef)->set(statement->varName, newValue);
                }
                else
                {
                    TuiRef* copiedValue = newValue->copy();
                    parent->set(statement->varName, copiedValue); //set a new local
                    newValue->release();
                    newValue = copiedValue;
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
                
                if(newValue != prevValue)
                {
                    if(newValue)
                    {
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
                else
                {
                    newValue->release();
                }
            }
            else if(enclosingSetRef && enclosingSetRef->type() == Tui_ref_type_TABLE)
            {
                if(((TuiTable*)enclosingSetRef)->onSet && !setKey.empty())
                {
                    ((TuiTable*)enclosingSetRef)->onSet(((TuiTable*)enclosingSetRef), setKey, existingValue);
                }
            }
            
            if(existingValue)
            {
                existingValue->release();
            }
            
            if(enclosingSetRef)
            {
                enclosingSetRef->release();
            }
            if(subTypeRef)
            {
                subTypeRef->release();
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
                    expressionResult->release();
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

    
TuiFunction::TuiFunction(TuiTable* parentTable_)
:TuiRef()
{
    parentTable = parentTable_;
    parentTable->retain();
}


TuiFunction::TuiFunction(std::function<TuiRef*(TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func_)
:TuiRef()
{
    func = func_;
}

TuiFunction::~TuiFunction()
{
    for(TuiStatement* statement : statements)
    {
        delete statement;
    }
    
    if(parentTable)
    {
        parentTable->release();
    }
}


TuiRef* TuiFunction::runTableConstruct(TuiTable* state,
             TuiRef* existingResult,
             TuiDebugInfo* callingDebugInfo)
{
    TuiTable* functionStateTable = new TuiTable(state);
    TuiFunctionCallData callData;
    callData.functionStateTable = functionStateTable;
    
    // the code below up until calling the statement is very similar to TuiTable for() loop parsing
    // changes made here should probably be made there or it all could be factored out.
    
    
    for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
    {
        if(callData.locals.count(varNameAndToken.second) == 0)
        {
            TuiTable* parentTable = state;
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
    
    for(auto& parentDepthAndToken : tokenMap.capturedParentTokensByDepthCount)
    {
        if(callData.locals.count(parentDepthAndToken.first) == 0)
        {
            TuiTable* parentTable = state;
            for(int i = 1; parentTable && i <= parentDepthAndToken.first; i++)
            {
                if(tokenMap.capturedParentTokensByDepthCount.count(i) != 0)
                {
                    uint32_t token = tokenMap.capturedParentTokensByDepthCount[i];
                    if(callData.locals.count(token) == 0)
                    {
                        parentTable->retain();
                        callData.locals[token] = parentTable;
                    }
                }
                parentTable = parentTable->parentTable;
            }
        }
    }
    
    
    TuiRef* result = runStatementArray(statements,  existingResult, functionStateTable, &tokenMap, &callData, &debugInfo);
    if(result)
    {
        result->release();
    }
    
    for(auto& tokenAndRef : callData.locals)
    {
        tokenAndRef.second->release();
    }
    
    return functionStateTable;
}

TuiRef* TuiFunction::call(TuiTable* args,
                          TuiRef* existingResult,
                          TuiDebugInfo* callingDebugInfo)
{
    if(func)
    {
        TuiRef* result = func(args, existingResult, callingDebugInfo);
        return result;
    }
    else
    {
        
        TuiTable* functionStateTable = new TuiTable(parentTable);
        TuiFunctionCallData callData;
        callData.functionStateTable = functionStateTable;
        
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
                    arg->retain();
                    callData.locals[tokenMap.capturedTokensByVarName[argName]] = arg;
                }
                functionStateTable->set(argName, arg);
                i++;
            }
        }
        
        // the code below up until calling the statement is very similar to TuiTable for() loop parsing
        // changes made here should probably be made there or it all could be factored out.
        
        
        for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
        {
            if(callData.locals.count(varNameAndToken.second) == 0)
            {
                TuiTable* parentRef = parentTable;
                while(parentRef)
                {
                    if(parentRef->objectsByStringKey.count(varNameAndToken.first) != 0)
                    {
                        TuiRef* var = parentRef->objectsByStringKey[varNameAndToken.first];
                        var->retain();
                        callData.locals[varNameAndToken.second] = var;
                        //functionStateTable->set(varNameAndToken.second, var);
                        break;
                    }
                    parentRef = parentRef->parentTable;
                }
            }
        }
        
        for(auto& parentDepthAndToken : tokenMap.capturedParentTokensByDepthCount)
        {
            if(callData.locals.count(parentDepthAndToken.first) == 0)
            {
                TuiTable* parentRef = parentTable->parentTable;
                for(int i = 1; parentRef && i <= parentDepthAndToken.first; i++)
                {
                    if(tokenMap.capturedParentTokensByDepthCount.count(i) != 0)
                    {
                        uint32_t token = tokenMap.capturedParentTokensByDepthCount[i];
                        if(callData.locals.count(token) == 0)
                        {
                            parentRef->retain();
                            callData.locals[token] = parentRef;
                        }
                    }
                    parentRef = parentRef->parentTable;
                }
            }
        }
        
        
        TuiRef* result = runStatementArray(statements,  existingResult, functionStateTable, &tokenMap, &callData, &debugInfo);
        
        functionStateTable->release();
        
        
        for(auto& tokenAndRef : callData.locals)
        {
            tokenAndRef.second->release();
        }
        
        return result;
    }
}


TuiRef* TuiFunction::call(const std::string& debugName, TuiRef* arg1, TuiRef* arg2, TuiRef* arg3, TuiRef* arg4) //NOTE!!!! Args will be released. You must retain any args that you wish to use after this call.
{
    TuiDebugInfo debugInfo;
    debugInfo.fileName = debugName;
    TuiTable* args = nullptr;
    if(arg1)
    {
        args = new TuiTable(nullptr);
        args->arrayObjects.push_back(arg1);
        if(arg2)
        {
            args->arrayObjects.push_back(arg2);
            if(arg3)
            {
                args->arrayObjects.push_back(arg3);
                if(arg4)
                {
                    args->arrayObjects.push_back(arg4);
                }
            }
        }
    }
    
    TuiRef* result = call(args, nullptr, &debugInfo);
    
    if(args)
    {
        args->release();
    }
    
    return result;
}
