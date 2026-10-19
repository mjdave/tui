#include "TuiFunction.h"

#include "TuiTable.h"
#include "TuiRef.h"


#define CAPTURE_TYPE_NONE 0
#define CAPTURE_TYPE_EXISTING 1
#define CAPTURE_TYPE_NEW 2

// tokenize a value chain eg foo.bar().x[3].y
void serializeValue(const char* str,
                    char** endptr,
                    TuiExpression* expression,
                    Tui::table parent,
                    TuiTokenMap* tokenMap,
                    uint32_t tokenStartPos,
                    TuiDebugInfo* debugInfo,
                    std::string* foundVarName = nullptr,
                    int* foundVarIndex = nullptr,
                    int* foundVarCaptureType = nullptr)
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
        }
        else if(*s == '#' || *s == '\n' || *s == ',' || isspace(*s) || *s == ')' || *s == '}' || *s == ']' || TuiExpressionOperatorsSet.count(*s) != 0)
        {
            break;
        }
        else if(*s == '.' || *s == '(' || *s == '[')
        {
            if(*(s+1) == '.' && *s != '(' && *s != '[') // .. syntax eg. ..var
            {
                TuiParseError(debugInfo, "\"..\" is not valid"); // .foo = x sets x on the closest parent which has a key 'foo', or a global on the root table if none exists
            }
            else
            {
                if(!stringBuffer.empty())
                {
                    uint32_t varToken = 0;
                    if(singleQuote || doubleQuote)
                    {
                        TuiParseError(debugInfo, "unable to chain a quoted string");
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
                            
                            Tui::string stringConstant = Tui::create<TuiString>(stringBuffer);
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
                                
                                if(foundVarCaptureType)
                                {
                                    *foundVarCaptureType = CAPTURE_TYPE_EXISTING;
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
                                
                                if(foundVarCaptureType)
                                {
                                    *foundVarCaptureType = CAPTURE_TYPE_NEW;
                                }
                            }
                        }
                    }
                    
                    stringBuffer = "";
                }
                else // .foo = 23
                {
                    if(!varChainStarted)
                    {
                        expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_parentCaptureVarChain); //note tokenStartPos to place before all other tokens
                        tokenPos++;
                        varChainStarted = true;
                    }
                    hasFoundInitialValue = true;
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
                    expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_childByExpression);
    
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
            Tui::number number = Tui::create<TuiNumber>(value);
            uint32_t constantNumberToken = tokenMap->tokenIndex++;
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, constantNumberToken);
            tokenMap->refsByToken[constantNumberToken] = number;
            break;
        }
        else if(*s == '{') // serialize table constructor
        {
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_tableConstruct);
            Tui::function constructorFunction = Tui::create<TuiFunction>(parent);
            constructorFunction->debugInfoLine = *debugInfo->currentLine;
            uint32_t constructorFunctionToken = tokenMap->tokenIndex++;
            expression->tokens.insert(expression->tokens.begin() + tokenPos++, constructorFunctionToken);
            tokenMap->refsByToken[constructorFunctionToken] = constructorFunction;
            
            constructorFunction->tokenMap.tokenIndex = tokenMap->tokenIndex + 1;
            
            bool success = TuiFunction::serializeFunctionBody(s, endptr, parent, &constructorFunction->tokenMap, debugInfo, false, &constructorFunction->statements);
            
            if(!success)
            {
                return;
            }
            
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
            
            if(*s == 'm' && *(s + 1) == 'a' && *(s + 2) == 't' && *(s + 4) == '(')
            {
                int vecCount = 0;
                switch(*(s + 3))
                {
                    case '3':
                    {
                        vecCount = 9;
                        expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_mat3);
                    }
                        break;
                    /*case '4':
                    {
                        vecCount = 16;
                        expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_vec3);
                    }
                        break;*/
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
            
            Tui::function functionRef = TuiFunction::initWithHumanReadableString(s, endptr, parent, debugInfo);
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
        Tui::string stringConstant = Tui::create<TuiString>(stringBuffer);
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
            Tui::string stringConstant = Tui::create<TuiString>(stringBuffer);
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
            if(stringBuffer == "true")
            {
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_true);
            }
            else if(stringBuffer == "false")
            {
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_false);
            }
            else if(stringBuffer == "this")
            {
                expression->tokens.insert(expression->tokens.begin() + tokenPos++, Tui_token_this);
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
                
                if(foundVarCaptureType)
                {
                    *foundVarCaptureType = CAPTURE_TYPE_EXISTING;
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
                
                if(foundVarCaptureType)
                {
                    *foundVarCaptureType = CAPTURE_TYPE_NEW;
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
                                                 const Tui::table& parent,
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
                    newOperatorLevel = Tui_operator_level_default;
                }
                else if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_addInPlace);
                    newOperatorLevel = Tui_operator_level_default;
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
                    newOperatorLevel = Tui_operator_level_default;
                }
                else if(secondOperatorChar == '=')
                {
                    expression->tokens.insert(expression->tokens.begin() + tokenStartPos, Tui_token_subtractInPlace);
                    newOperatorLevel = Tui_operator_level_default;
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
                    newOperatorLevel = Tui_operator_level_default;
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
                    newOperatorLevel = Tui_operator_level_default;
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
                                             Tui::table parent,
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
    int foundVarCaptureType = CAPTURE_TYPE_NONE;
    if(!expression)
    {
        expression = new TuiExpression();
        serializeValue(s, endptr, expression, parent, tokenMap, 0, debugInfo, &localSetKey, &localSetIndex, &foundVarCaptureType);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
    }
    TuiStatement* statement = nullptr;
    
    if(expression->tokens.empty())
    {
        TuiParseError(debugInfo, "unexpcted character:%c", *s);
        return nullptr;
    }
    
    if(expression->tokens[0] == Tui_token_functionCall)
    {
        statement = new TuiStatement(Tui_statement_type_functionCall);
        statement->debugInfoLine = *debugInfo->currentLine;
        statement->expression = expression;
    }
    else if(TuiExpressionOperatorsSet.count(*s) != 0)
    {
        if(*s == '=' && *(s + 1) != '=') // standard x = y assignment
        {
            uint32_t variableTokenToAddToLocals = 0;
            
            if(foundVarCaptureType != CAPTURE_TYPE_NONE && (!sharesParentScope || parent->objectsByStringKey.count(localSetKey) == 0) && expression->tokens[0] != Tui_token_varChain)
            {
                expression->tokens.clear(); //remove parent capture. assumes a few things
                
                if(tokenMap->localTokensByVarName.count(localSetKey) != 0)
                {
                    expression->tokens.push_back(tokenMap->localTokensByVarName[localSetKey]);
                }
                else if(foundVarCaptureType != CAPTURE_TYPE_EXISTING && tokenMap->capturedTokensByVarName.count(localSetKey) != 0)
                {
                    variableTokenToAddToLocals = tokenMap->capturedTokensByVarName[localSetKey];
                    tokenMap->capturedTokensByVarName.erase(localSetKey);//erase so that it is not visible when assigning. assigned local later.
                    expression->tokens.push_back(variableTokenToAddToLocals);
                }
                else
                {
                    variableTokenToAddToLocals = tokenMap->tokenIndex++;
                    expression->tokens.push_back(variableTokenToAddToLocals);
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
        
        statement->debugInfoLine = *debugInfo->currentLine;
        statement->expression = expression;
    }
    else
    {
        TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default, nullptr, nullptr, 0);
        statement = new TuiStatement(Tui_statement_type_value);
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        statement->debugInfoLine = *debugInfo->currentLine;
        statement->expression = expression;
    }
    
    *endptr = (char*)s;
    return statement;
}

TuiStatement* TuiFunction::serializeForStatement(const char* str,
                                                 char** endptr,
                                                 const Tui::table& parent,
                                                 TuiDebugInfo* debugInfo,
                                                 bool sharesParentScope,
                                                 bool isWhileLoop) //entry point is after 'for'
{
    const char* s = str;
    if(*s == '(')
    {
        s++;
        s = tuiSkipToNextChar(s, debugInfo);
    }
    
    TuiStatement* resultStatement = nullptr;
    
    //todo test function call for initial statement, probably a bug, expression probably already contains the whole call
    if(isWhileLoop)
    {
        TuiForExpressionsStatement* forStatement = new TuiForExpressionsStatement();
        resultStatement = forStatement;
        forStatement->debugInfoLine = *debugInfo->currentLine;
        
        forStatement->continueExpression = new TuiExpression();
        recursivelySerializeExpression(s, endptr, forStatement->continueExpression, parent, &forStatement->outerTokenMap, debugInfo, Tui_operator_level_default);
        s = tuiSkipToNextChar(*endptr, debugInfo);
        s++; // ')'
        s = tuiSkipToNextChar(s, debugInfo);
        
        bool success = serializeFunctionBody(s, endptr, parent, &forStatement->innerTokenMap, debugInfo, sharesParentScope, &forStatement->statements);
        if(!success)
        {
            return nullptr;
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo);
        *endptr = (char*)s;
    }
    else
    {
        int commmaCount = 0;
        int parenthisisCount = 0;
        const char* sLookAhead = s;
        for(;; sLookAhead++)
        {
            if(*sLookAhead == ',')
            {
                commmaCount++;
            }
            else if(*sLookAhead == '(')
            {
                parenthisisCount++;
            }
            else if(*sLookAhead == ')')
            {
                if(parenthisisCount == 0)
                {
                    break;
                }
                parenthisisCount--;
            }
        }
        bool isIndexLoopType = (commmaCount == 2);
        
        if(isIndexLoopType) // for(i = 0, i < 5, i++)
        {
            TuiForExpressionsStatement* forStatement = new TuiForExpressionsStatement();
            resultStatement = forStatement;
            forStatement->debugInfoLine = *debugInfo->currentLine;
            
            forStatement->initialStatement = serializeBasicStatement(s, endptr, parent, &forStatement->outerTokenMap, debugInfo, sharesParentScope);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            
            s++; // ','
            s = tuiSkipToNextChar(s, debugInfo);
            
            forStatement->continueExpression = new TuiExpression();
            recursivelySerializeExpression(s, endptr, forStatement->continueExpression, parent, &forStatement->outerTokenMap, debugInfo, Tui_operator_level_default);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            s++; // ','
            s = tuiSkipToNextChar(s, debugInfo);
            
            forStatement->incrementStatement = serializeBasicStatement(s, endptr, parent, &forStatement->outerTokenMap, debugInfo, sharesParentScope);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            s++; // ')'
            s = tuiSkipToNextChar(s, debugInfo);
            
            bool success = serializeFunctionBody(s, endptr, parent, &forStatement->innerTokenMap, debugInfo, sharesParentScope, &forStatement->statements);
            if(!success)
            {
                return nullptr;
            }
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            *endptr = (char*)s;
            
        }
        else // for(object in table) || for(indexOrKey, object in table) || for(object : table) || for(indexOrKey, object : table)
        {
            TuiExpression* expression = new TuiExpression();
            std::string setKey;
            
            TuiTokenMap outerTokenMap;
            
            recursivelySerializeExpression(s, endptr, expression, parent, &outerTokenMap, debugInfo, Tui_operator_level_default, &setKey);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            
            int containerObjectTokenIndex = 1;
            TuiForContainerLoopStatement* forStatement = nullptr;
            if(*s == ',') // for(indexOrKey, object in table)
            {
                containerObjectTokenIndex = 2;
                
                forStatement = new TuiForContainerLoopStatement(Tui_statement_type_forKeyedValues);
                forStatement->keyOrIndexName = setKey;
                resultStatement = forStatement;
                forStatement->debugInfoLine = *debugInfo->currentLine;
                forStatement->expression = expression;
                
                s++; // ','
                s = tuiSkipToNextChar(s, debugInfo);
                std::string objectSetKey;
                serializeValue(s, endptr, expression, parent, &outerTokenMap, 1, debugInfo, &objectSetKey); //store object var at index 1
                forStatement->objectName = objectSetKey;
                s = tuiSkipToNextChar(*endptr, debugInfo);
                *endptr = (char*)s;
            }
            else // for(object in table)
            {
                forStatement = new TuiForContainerLoopStatement(Tui_statement_type_forValues);
                forStatement->objectName = setKey;
                resultStatement = forStatement;
                forStatement->debugInfoLine = *debugInfo->currentLine;
                forStatement->expression = expression;
            }
            
            forStatement->outerTokenMap = outerTokenMap;
            
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
                TuiParseError(debugInfo, "expected ',' or 'in' or ':' in for loop");
            }
            s = tuiSkipToNextChar(s, debugInfo);
            
            serializeValue(s, endptr, expression, parent, &forStatement->outerTokenMap, containerObjectTokenIndex, debugInfo, nullptr);
            s = tuiSkipToNextChar(*endptr, debugInfo);
            s++; // ')'
            s = tuiSkipToNextChar(s, debugInfo);
            
            bool success = serializeFunctionBody(s, endptr, parent, &forStatement->innerTokenMap, debugInfo, sharesParentScope, &forStatement->statements);
            if(!success)
            {
                return nullptr;
            }
            
            if(forStatement->innerTokenMap.capturedTokensByVarName.count(forStatement->objectName) == 0)
            {
                uint32_t variableToken = forStatement->innerTokenMap.tokenIndex++;
                forStatement->innerTokenMap.capturedTokensByVarName[forStatement->objectName] = variableToken;
            }
            
            forStatement->objectInnerToken = forStatement->innerTokenMap.capturedTokensByVarName[forStatement->objectName];
            if(!forStatement->keyOrIndexName.empty())
            {
                if(forStatement->innerTokenMap.capturedTokensByVarName.count(forStatement->keyOrIndexName) == 0)
                {
                    uint32_t variableToken = forStatement->innerTokenMap.tokenIndex++;
                    forStatement->innerTokenMap.capturedTokensByVarName[forStatement->keyOrIndexName] = variableToken;
                }
                
                forStatement->indexInnerToken = forStatement->innerTokenMap.capturedTokensByVarName[forStatement->keyOrIndexName];
            }
            
            s = tuiSkipToNextChar(*endptr, debugInfo);
            *endptr = (char*)s;
        }
    }
    
    return resultStatement;
}


bool TuiFunction::serializeFunctionBody(const char* str,
                                        char** endptr,
                                        const Tui::table& parent,
                                        TuiTokenMap* tokenMap,
                                        TuiDebugInfo* debugInfo,
                                        bool sharesParentScope,
                                        std::vector<TuiStatement*>* statements)
{
    const char* s = str;
    if(*s != '{')
    {
        TuiParseError(debugInfo, "Function expected opening brace");
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
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, parent, debugInfo, sharesParentScope, false);
            if(!statement)
            {
                return false;
            }
            statements->push_back(statement);
            s = tuiSkipToNextChar(*endptr, debugInfo, false);
        }
        else if(*s == 'w' && *(s + 1) == 'h' && *(s + 2) == 'i' && *(s + 3) == 'l' && *(s + 4) == 'e' && (*(s + 5) == '(' || isspace(*(s + 5))))
        {
            s+=5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, parent, debugInfo, sharesParentScope, true);
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
            statement->debugInfoLine = *debugInfo->currentLine;
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
                        currentStatement->elseIfStatement->debugInfoLine = *debugInfo->currentLine;
                        
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
                        currentStatement->elseIfStatement->debugInfoLine = *debugInfo->currentLine;
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
                        TuiParseError(debugInfo, "else statement expected 'if' or '{'");
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
           && *(s + 5) == 'n' && checkSymbolNameComplete(s + 6))
        {
            s+=6;
            s = tuiSkipToNextChar(s, debugInfo);
            if(*s == '}') //this seems off
            {
                TuiStatement* statement = new TuiStatement(Tui_statement_type_return);
                statements->push_back(statement);
                s++; //this in particular
                s = tuiSkipToNextChar(s, debugInfo, true);
                break;
            }
            else
            {
                TuiStatement* statement = new TuiStatement(Tui_statement_type_returnExpression);
                statement->debugInfoLine = *debugInfo->currentLine;
                statement->expression = new TuiExpression();
                
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        else if(*s == 'b'
           && *(s + 1) == 'r'
           && *(s + 2) == 'e'
           && *(s + 3) == 'a'
           && *(s + 4) == 'k' && checkSymbolNameComplete(s + 5))
        {
            s+=5;
            s = tuiSkipToNextChar(s, debugInfo, true);
            TuiStatement* statement = new TuiStatement(Tui_statement_type_break);
            statements->push_back(statement);
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

Tui::function TuiFunction::initWithHumanReadableString(const char* str, char** endptr, const Tui::table& parent, TuiDebugInfo* debugInfo) //assumes that '(' is currently in str.
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
        
        Tui::function mjFunction = Tui::create<TuiFunction>(parent);
        mjFunction->debugInfoLine = *debugInfo->currentLine;
        
        //TuiDebugInfoCopy(debugInfo, &mjFunction->debugInfo);
        //TuiDebugInfoPush(&mjFunction->debugInfo, debugInfo->currentLine->fileName, 1);
        
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
                    debugInfo->currentLine->lineNumber++;
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
            return nullptr;
        }
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        *endptr = (char*)s;
        return mjFunction;
        
    }
    
    return nullptr;
}


Tui::ref TuiFunction::runExpression(TuiExpression* expression,
                                   uint32_t* tokenPos,
                                   const Tui::ref& result,
                                   const Tui::table& parent,
                                   TuiTokenMap* tokenMap,
                                   TuiFunctionCallData* callData,
                                   TuiDebugInfo* debugInfo,
                                   std::string* setKey,
                                   int* setIndex,
                                   Tui::ref* enclosingSetRef,
                                   std::string* subTypeAccessKey,
                                   Tui::ref* subTypeRef)
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
                Tui::function functionVar = Tui::cast<TuiFunction>(runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef));
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    
                    if(functionVar && functionVar->type() != Tui_ref_type_NIL)
                    {
                        TuiParseError(debugInfo, "Expected function, found %s.", (functionVar ? functionVar->getTypeName().c_str() : "nil"));
                        return nullptr;
                    }
                    
                    std::string funcNameText = "";
                    
                    if(expression->tokens.size() > *tokenPos)
                    {
                        for(auto& varNameAndToken : tokenMap->capturedTokensByVarName)
                        {
                            if(varNameAndToken.second == expression->tokens[*tokenPos])
                            {
                                funcNameText = " '" + varNameAndToken.first + "'";
                            }
                        }
                    }
                    
                    TuiParseError(debugInfo, "Attempt to call missing function%s", funcNameText.c_str());
                    return nullptr;
                }
                (*tokenPos)++;
                
                Tui::table args = nullptr;
                Tui::ref arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(arg)
                {
                    args = Tui::create<TuiTable>(parent);
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
                }
                
                
                Tui::ref functionResult = Tui::cast<TuiFunction>(functionVar)->call(args, result, callData, debugInfo);
                if(result && functionResult && result->type() == functionResult->type() && result->type() != Tui_ref_type_BOOL && result->type() != Tui_ref_type_TABLE && result->type() != Tui_ref_type_FUNCTION)
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
                Tui::ref x = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!x || x->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo, "x component expected number, got:%s", (x ? x->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                (*tokenPos)++;
                Tui::ref y = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!y || y->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo, "y component expected number, got:%s", (y ? y->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                switch (token) {
                    case Tui_token_vec2:
                        {
                            if(result && result->type() == Tui_ref_type_VEC2)
                            {
                                Tui::cast<TuiVec2>(result)->value.x = (Tui::cast<TuiNumber>(x)->value);
                                Tui::cast<TuiVec2>(result)->value.y = (Tui::cast<TuiNumber>(y)->value);
                            }
                            else
                            {
                                Tui::vec2 newResultVec = Tui::create<TuiVec2>(dvec2(Tui::cast<TuiNumber>(x)->value, (Tui::cast<TuiNumber>(y)->value)));
                                return newResultVec;
                            }
                        }
                        break;
                    case Tui_token_vec3:
                    case Tui_token_vec4:
                        {
                            (*tokenPos)++;
                            Tui::ref z = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                            
                            if(!z || z->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo, "z component expected number, got:%s", (z ? z->getDebugString().c_str() : "nil"));
                                return nullptr;
                            }
                            
                            if(token == Tui_token_vec3)
                            {
                                if(result && result->type() == Tui_ref_type_VEC3)
                                {
                                    Tui::cast<TuiVec3>(result)->value.x = (Tui::cast<TuiNumber>(x)->value);
                                    Tui::cast<TuiVec3>(result)->value.y = (Tui::cast<TuiNumber>(y)->value);
                                    Tui::cast<TuiVec3>(result)->value.z = (Tui::cast<TuiNumber>(z)->value);
                                }
                                else
                                {
                                    Tui::vec3 newResultVec = Tui::create<TuiVec3>(dvec3(Tui::cast<TuiNumber>(x)->value, Tui::cast<TuiNumber>(y)->value, (Tui::cast<TuiNumber>(z)->value)));
                                    return newResultVec;
                                }
                            }
                            else
                            {
                                (*tokenPos)++;
                                Tui::ref w = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                                
                                if(!w || w->type() != Tui_ref_type_NUMBER)
                                {
                                    TuiParseError(debugInfo, "w component expected number, got:%s", (w ? w->getDebugString().c_str() : "nil"));
                                    return nullptr;
                                }
                                if(result && result->type() == Tui_ref_type_VEC4)
                                {
                                    Tui::cast<TuiVec4>(result)->value.x = (Tui::cast<TuiNumber>(x)->value);
                                    Tui::cast<TuiVec4>(result)->value.y = (Tui::cast<TuiNumber>(y)->value);
                                    Tui::cast<TuiVec4>(result)->value.z = (Tui::cast<TuiNumber>(z)->value);
                                    Tui::cast<TuiVec4>(result)->value.w = (Tui::cast<TuiNumber>(w)->value);
                                }
                                else
                                {
                                    Tui::vec4 newResultVec = Tui::create<TuiVec4>(dvec4(Tui::cast<TuiNumber>(x)->value, Tui::cast<TuiNumber>(y)->value, Tui::cast<TuiNumber>(z)->value, (Tui::cast<TuiNumber>(w)->value)));
                                    return newResultVec;
                                }
                            }
                        }
                        break;
                }
                
            }
                break;
            case Tui_token_mat3:
            {
                double values[9];
                for(int i= 0; i < 9; i++)
                {
                    (*tokenPos)++;
                    Tui::ref x = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    
                    if(!x || x->type() != Tui_ref_type_NUMBER)
                    {
                        TuiParseError(debugInfo, "matrix component at index:%d expected number, got:%s", i, (x ? x->getDebugString().c_str() : "nil"));
                        return nullptr;
                    }
                    
                    values[i] = (Tui::cast<TuiNumber>(x)->value);
                }
                
                if(result && result->type() == Tui_ref_type_MAT3)
                {
                    Tui::cast<TuiMat3>(result)->value[0].x = values[0];
                    Tui::cast<TuiMat3>(result)->value[0].y = values[1];
                    Tui::cast<TuiMat3>(result)->value[0].z = values[2];
                    Tui::cast<TuiMat3>(result)->value[1].x = values[3];
                    Tui::cast<TuiMat3>(result)->value[1].y = values[4];
                    Tui::cast<TuiMat3>(result)->value[1].z = values[5];
                    Tui::cast<TuiMat3>(result)->value[2].x = values[6];
                    Tui::cast<TuiMat3>(result)->value[2].y = values[7];
                    Tui::cast<TuiMat3>(result)->value[2].z = values[8];
                }
                else
                {
                    Tui::mat3 newResult = Tui::create<TuiMat3>(dmat3(values[0],
                                                  values[1],
                                                  values[2],
                                                  values[3],
                                                  values[4],
                                                  values[5],
                                                  values[6],
                                                  values[7],
                                                  values[8]));
                    return newResult;
                }
                
            }
                break;
            case Tui_token_tableConstruct:
            {
                (*tokenPos)++;
                Tui::function functionVar = Tui::cast<TuiFunction>(runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef));
                if(!functionVar || functionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo, "expected function, got:%s", (functionVar ? functionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                Tui::ref functionResult = Tui::cast<TuiFunction>(functionVar)->runTableConstruct(parent, result, debugInfo);
                
                return functionResult;
            }
                break;
            case Tui_token_functionDeclaration:
            {
                (*tokenPos)++;
                Tui::function baseFunctionVar = Tui::cast<TuiFunction>(runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef));
                if(!baseFunctionVar || baseFunctionVar->type() != Tui_ref_type_FUNCTION)
                {
                    TuiParseError(debugInfo, "expected function, got:%s", (baseFunctionVar ? baseFunctionVar->getDebugString().c_str() : "nil"));
                    return nullptr;
                }
                
                Tui::function functionCopy = baseFunctionVar->trueCopy();
                
                functionCopy->parentTable = parent;
                functionCopy->retainedTransientLoopTables = callData->transientLoopTables;
                
                callData->capturedFunctions.push_back(functionCopy);
                
                for(auto& varNameAndToken : functionCopy->tokenMap.capturedTokensByVarName)
                {
                    if(callData->localTokensByStringKey.count(varNameAndToken.first) != 0)
                    {
                        Tui::ref var = callData->locals[callData->localTokensByStringKey[varNameAndToken.first]];
                        functionCopy->tokenMap.refsByToken[varNameAndToken.second] = var;
                    }
                    else
                    {
                        Tui::table parentTable = parent;
                        std::set<Tui::table> found;
                        while(parentTable)
                        {
                            if(parentTable->objectsByStringKey.count(varNameAndToken.first) != 0)
                            {
                                Tui::ref var = parentTable->objectsByStringKey[varNameAndToken.first];
                                
                                functionCopy->tokenMap.refsByToken[varNameAndToken.second] = var;
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
                
                return functionCopy;
            }
                break;
            case Tui_token_equalTo:
            {
                (*tokenPos)++;
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!leftResult)
                {
                    bool boolResult = !rightResult || rightResult->type() == Tui_ref_type_NIL;
                    return TUI_BOOL(boolResult);
                }
                
                bool boolResult = leftResult->isEqual(rightResult);
                
                return TUI_BOOL(boolResult);
            }
                break;
            case Tui_token_or:
            {
                (*tokenPos)++;
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                
                if(leftResult && leftResult->boolValue())
                {
                    return TUI_TRUE;
                }
                else
                {
                    Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    Tui::boolean returnValue = TUI_BOOL(rightResult && rightResult->boolValue());
                    return returnValue;
                }
                
            }
                break;
            case Tui_token_and:
            {
                (*tokenPos)++;
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                
                if(!leftResult || !leftResult->boolValue())
                {
                    return TUI_FALSE;
                }
                else
                {
                    
                    Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    
                    Tui::boolean returnValue = TUI_BOOL(rightResult && rightResult->boolValue());
                    return returnValue;
                    
                }
                
            }
                break;
            case Tui_token_notEqualTo:
            {
                (*tokenPos)++;
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                (*tokenPos)++;
                Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                
                if(!leftResult)
                {
                    Tui::boolean returnValue = TUI_BOOL(rightResult && rightResult->type() != Tui_ref_type_NIL);
                    return returnValue;
                }
                Tui::boolean returnValue = TUI_BOOL(!leftResult->isEqual(rightResult));
                return returnValue;
            }
                break;
            case Tui_token_not:
            {
                (*tokenPos)++;
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                if(expression->tokens[*tokenPos+1] == Tui_token_end)
                {
                    (*tokenPos)++; //Tui_token_end
                }
                else
                {
                    TuiError("expected Tui_token_end");
                }
                Tui::boolean returnValue = TuiRef::logicalNot(leftResult);
                return returnValue;
            }
                break;
            case Tui_token_negate:
            {
                (*tokenPos)++;
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
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
                            Tui::cast<TuiNumber>(result)->value = -(Tui::cast<TuiNumber>(leftResult)->value);
                            return nullptr;
                        }
                        Tui::number returnResult = Tui::create<TuiNumber>(-(Tui::cast<TuiNumber>(leftResult)->value));
                        return returnResult;
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        if(result && result->type() == Tui_ref_type_VEC2)
                        {
                            Tui::cast<TuiVec2>(result)->value = -(Tui::cast<TuiVec2>(leftResult)->value);
                            return nullptr;
                        }
                        Tui::vec2 returnResult = Tui::create<TuiVec2>(-(Tui::cast<TuiVec2>(leftResult)->value));
                        return returnResult;
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        if(result && result->type() == Tui_ref_type_VEC3)
                        {
                            Tui::cast<TuiVec3>(result)->value = -(Tui::cast<TuiVec3>(leftResult)->value);
                            return nullptr;
                        }
                        Tui::vec3 returnResult = Tui::create<TuiVec3>(-(Tui::cast<TuiVec3>(leftResult)->value));
                        return returnResult;
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        if(result && result->type() == Tui_ref_type_VEC4)
                        {
                            Tui::cast<TuiVec4>(result)->value = -(Tui::cast<TuiVec4>(leftResult)->value);
                            return nullptr;
                        }
                        Tui::vec4 returnResult = Tui::create<TuiVec4>(-(Tui::cast<TuiVec4>(leftResult)->value));
                        return returnResult;
                    }
                        break;
                        
                    default:
                        TuiParseError(debugInfo, "expected number or vector, got:%s", (leftResult ? leftResult->getDebugString().c_str() : "nil"));
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
            case Tui_token_this:
            {
                Tui::table parentToReturn = callData->thisTable;
                if(parentToReturn)
                {
                    return parentToReturn;
                }
                return TUI_NIL;
            }
                break;
            case Tui_token_false:
            {
                return TUI_FALSE;
            }
                break;
            case Tui_token_varChain:
            case Tui_token_parentCaptureVarChain:
            {
                (*tokenPos)++;
                Tui::table chainParent = nullptr;
                Tui::table chainResult = parent;
                
                if(token == Tui_token_parentCaptureVarChain)
                {
                    uint32_t nextToken = expression->tokens[*tokenPos];
                    if(nextToken == Tui_token_childByString)
                    {
                        uint32_t subTokenPos = *tokenPos;
                        subTokenPos++;
                        if(expression->tokens[subTokenPos] == Tui_token_functionCall)
                        {
                            TuiParseError(debugInfo, "Function call not supported here");
                        }
                        
                        Tui::ref keyConstant = runExpression(expression, &subTokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                        
                        if(keyConstant->type() == Tui_ref_type_STRING)
                        {
                            while(chainResult->parentTable)
                            {
                                if(chainResult->objectsByStringKey.count(Tui::cast<TuiString>(keyConstant)->value) != 0)
                                {
                                    break;
                                }
                                
                                chainResult = chainResult->parentTable;
                            }
                        }
                    }
                }
                Tui::ref keyConstant = nullptr;
                
                while(1)
                {
                    switch(chainResult->type())
                    {
                        case Tui_ref_type_TABLE:
                        {
                            chainParent = chainResult;
                            chainResult = Tui::cast<TuiTable>(runExpression(expression, tokenPos, nullptr, chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef)); //todo should pass result here
                        }
                            break;
                        case Tui_ref_type_VEC2:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, Tui::cast<TuiTable>(chainParent), tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, (Tui::table)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            (*tokenPos)++;
                            keyConstant = runExpression(expression, tokenPos, nullptr, (Tui::table)chainParent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        }
                            break;
                        default:
                        { //#Attempt to use '.' to access from nil object 'bar'
                            
                            std::string parentNameText = "";
                            
                            if(expression->tokens.size() - 1 > *tokenPos)
                            {
                                for(auto& varNameAndToken : tokenMap->capturedTokensByVarName)
                                {
                                    if(varNameAndToken.second == expression->tokens[*tokenPos - 1])
                                    {
                                        parentNameText = " '" + varNameAndToken.first + "'";
                                    }
                                }
                            }
                            
                            if(chainResult->type() == Tui_ref_type_NIL)
                            {
                                TuiParseError(debugInfo, "Attempt to use '.' to access from nil object%s", parentNameText.c_str());
                            }
                            else
                            {
                                TuiParseError(debugInfo, "Attempt to use '.' to access from object%s of unsupported type:%s", parentNameText.c_str(), chainResult->getTypeName().c_str());
                            }
                            return nullptr;
                        }
                            break;
                    }
                    
                    (*tokenPos)++;
                    
                    /*if(!chainResult) //this catches false positives. A function call foo.bar() maybe returns no result, but has done its job
                    {
                        TuiParseError(debugInfo, "attempt to use '.' syntax on nil value");
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
                        Tui::ref prevRef = *subTypeRef;
                        *subTypeRef = chainResult;
                        
                        if(subTypeAccessKey)
                        {
                            *subTypeAccessKey = (Tui::cast<TuiString>(keyConstant)->value);
                        }
                    }
                    
                    if(chainParent)
                    {
                        chainParent = nullptr;
                    }
                    
                    switch(chainResult->type())
                    {
                        case Tui_ref_type_VEC2:
                        {
                            switch(Tui::cast<TuiString>(keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.x);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec2>(Tui::cast<TuiRef>(chainResult))->value.x));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.y);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec2>(Tui::cast<TuiRef>(chainResult))->value.y));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                            }
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            switch(Tui::cast<TuiString>(keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.x);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.x));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.y);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.y));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'z':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.z);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec3>(Tui::cast<TuiRef>(chainResult))->value.z));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                            }
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            switch(Tui::cast<TuiString>(keyConstant)->value[0])
                            {
                                case 'x':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.x);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.x));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'y':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.y);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.y));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'z':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.z);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.z));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                                case 'w':
                                {
                                    if(result && result->type() == Tui_ref_type_NUMBER)
                                    {
                                        Tui::cast<TuiNumber>(result)->value = (Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.w);
                                        return nullptr;
                                    }
                                    else if(!enclosingSetRef)
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>((Tui::cast<TuiVec4>(Tui::cast<TuiRef>(chainResult))->value.w));
                                        return returnResult;
                                    }
                                    return nullptr;
                                }
                                    break;
                            }
                        }
                            break;
                        default:
                        {
                            TuiParseError(debugInfo, "no '.' syntax supported for type:%s", parent->getTypeName().c_str());
                            return nullptr;
                        }
                            break;
                    }
                }
                
                
                if(enclosingSetRef)
                {
                    Tui::ref prevRef = *enclosingSetRef;
                    *enclosingSetRef = chainParent;
                }
                
                return chainResult;
            }
                break;
            case Tui_token_childByString:
            case Tui_token_childByExpression:
            {
                (*tokenPos)++;
                bool isFunctionCall = false;
                if(expression->tokens[*tokenPos] == Tui_token_functionCall)
                {
                    isFunctionCall = true;
                    (*tokenPos)++;
                }
                
                Tui::ref keyConstant = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                Tui::ref child = nullptr;
                bool isStringKey = false;
                bool isNumberKey = false;
                
                if(keyConstant->type() == Tui_ref_type_STRING)
                {
                    isStringKey = true;
                    
                    if(parent->objectsByStringKey.count(Tui::cast<TuiString>(keyConstant)->value) != 0)
                    {
                        child = parent->objectsByStringKey[Tui::cast<TuiString>(keyConstant)->value];
                    }
                    
                    if(setKey)
                    {
                        *setKey = (Tui::cast<TuiString>(keyConstant)->value);
                    }
                    
                    if(enclosingSetRef)
                    {
                        Tui::ref prevRef = *enclosingSetRef;
                        *enclosingSetRef = parent;
                    }
                }
                else
                {
                    if(!keyConstant->isNumber())
                    {
                        TuiParseError(debugInfo, "expected number");
                        return nullptr;
                    }
                    
                    isNumberKey = true;
                    
                    int64_t arrayIndex = Tui::cast<TuiNumber>(keyConstant)->getNumberValue();
                    if(arrayIndex >= 0 && arrayIndex < parent->arrayObjects.size())
                    {
                        child = parent->arrayObjects[arrayIndex];
                    }
                    else if(parent->objectsByNumberKey.count((uint32_t)arrayIndex) != 0)
                    {
                        child = parent->objectsByNumberKey[(uint32_t)arrayIndex];
                    }
                    else if(parent->set8.count((uint8_t)arrayIndex) != 0)
                    {
                        child = TUI_TRUE;
                    }
                    else if(parent->set16.count((uint16_t)arrayIndex) != 0)
                    {
                        child = TUI_TRUE;
                    }
                    else if(parent->set32.count((uint32_t)arrayIndex) != 0)
                    {
                        child = TUI_TRUE;
                    }
                    else if(parent->set64.count(arrayIndex) != 0)
                    {
                        child = TUI_TRUE;
                    }
                    
                    if(setIndex)
                    {
                        *setIndex = (uint32_t)arrayIndex;
                    }
                    
                    if(enclosingSetRef)
                    {
                        Tui::ref prevRef = *enclosingSetRef;
                        *enclosingSetRef = parent;
                    }
                }
                
                if(!child)
                {
                    if(isFunctionCall)
                    {
                        TuiParseError(debugInfo, "Attempt to call missing function '%s'.", (keyConstant ? Tui::cast<TuiString>(keyConstant)->value.c_str() : "nil"));
                        return nullptr;
                    }
                    return TUI_NIL;
                }
                
                if(isFunctionCall)
                {
                    
                    if(child->type() != Tui_ref_type_FUNCTION)
                    {
                        TuiParseError(debugInfo, "Expected function '%s', found %s. 'table.%s' is not a function", Tui::cast<TuiString>(keyConstant)->value.c_str(), (child ? child->getTypeName().c_str() : "nil"), (Tui::cast<TuiString>(keyConstant)->value.c_str()));
                        return nullptr;
                    }
                    
                    Tui::table args = nullptr;
                    (*tokenPos)++;
                    Tui::ref arg = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                    
                    if(arg)
                    {
                        args = Tui::create<TuiTable>(parent);
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
                    }
                    
                    
                    Tui::ref functionResult = Tui::cast<TuiFunction>(child)->call(args, result, callData, debugInfo);
                    if(args)
                    {
                    }
                    
                    if(result && result->type() == functionResult->type() && result->type() != Tui_ref_type_BOOL && result->type() != Tui_ref_type_TABLE && result->type() != Tui_ref_type_FUNCTION)
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
                    if(enclosingSetRef && (isStringKey || isNumberKey))
                    {
                        Tui::ref prevRef = *enclosingSetRef;
                        *enclosingSetRef = parent;
                        
                        if(isStringKey)
                        {
                            *setKey = (Tui::cast<TuiString>(keyConstant)->value);
                        }
                        else if(isNumberKey && setIndex)
                        {
                            *setIndex = Tui::cast<TuiNumber>(keyConstant)->getNumberValue();
                        }
                    }
                    
                    if(result && result->type() == child->type() && result->type() != Tui_ref_type_BOOL && result->type() != Tui_ref_type_TABLE && result->type() != Tui_ref_type_FUNCTION)
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
                Tui::ref leftResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        (*tokenPos)++;
                        
                        Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(rightResult->type() != leftType)
                        {
                            TuiParseError(debugInfo, "expected number");
                            return nullptr;
                        }
                        
                        switch (token) {
                            case Tui_token_greaterThan:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiNumber>(leftResult)->value > (Tui::cast<TuiNumber>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                            case Tui_token_lessThan:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiNumber>(leftResult)->value < (Tui::cast<TuiNumber>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                            case Tui_token_greaterEqualTo:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiNumber>(leftResult)->value >= (Tui::cast<TuiNumber>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                            case Tui_token_lessEqualTo:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiNumber>(leftResult)->value <= (Tui::cast<TuiNumber>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                        };
                        
                    }
                        break;
                    case Tui_ref_type_STRING:
                    {
                        (*tokenPos)++;
                        
                        Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(rightResult->type() != leftType)
                        {
                            TuiParseError(debugInfo, "expected string");
                            return nullptr;
                        }
                        
                        switch (token) {
                            case Tui_token_greaterThan:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiString>(leftResult)->value > (Tui::cast<TuiString>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                            case Tui_token_lessThan:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiString>(leftResult)->value < (Tui::cast<TuiString>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                            case Tui_token_greaterEqualTo:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiString>(leftResult)->value >= (Tui::cast<TuiString>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                            case Tui_token_lessEqualTo:
                            {
                                Tui::boolean returnResult = TUI_BOOL(Tui::cast<TuiString>(leftResult)->value <= (Tui::cast<TuiString>(rightResult)->value));
                                return returnResult;
                            }
                                break;
                        };
                        
                    }
                        break;
                        
                    default:
                    {
                        TuiParseError(debugInfo, "expected number");
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
                Tui::ref inPlaceSubRef = nullptr;
                Tui::ref inPlaceEnclosingSetRef = nullptr;
                
                Tui::ref leftResult;
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
                        Tui::ref rightResult = nullptr;
                        if(token != Tui_token_increment && token != Tui_token_decrement)
                        {
                            (*tokenPos)++;
                            rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                            if(rightResult->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(debugInfo, "expected number");
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
                                                (Tui::cast<TuiVec2>(inPlaceSubRef)->value.x++);
                                                break;
                                            case 'y':
                                                (Tui::cast<TuiVec2>(inPlaceSubRef)->value.y++);
                                                break;
                                        }
                                        break;
                                    case Tui_token_decrement:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                (Tui::cast<TuiVec2>(inPlaceSubRef)->value.x--);
                                                break;
                                            case 'y':
                                                (Tui::cast<TuiVec2>(inPlaceSubRef)->value.y--);
                                                break;
                                        }
                                        break;
                                    case Tui_token_addInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.x += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.y += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                        break;
                                    case Tui_token_subtractInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.x -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.y -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                    case Tui_token_multiplyInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.x *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.y *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                    case Tui_token_divideInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.x /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec2>(inPlaceSubRef)->value.y /= (Tui::cast<TuiNumber>(rightResult)->value);
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
                                                (Tui::cast<TuiVec3>(inPlaceSubRef)->value.x++);
                                                break;
                                            case 'y':
                                                (Tui::cast<TuiVec3>(inPlaceSubRef)->value.y++);
                                                break;
                                            case 'z':
                                                (Tui::cast<TuiVec3>(inPlaceSubRef)->value.z++);
                                                break;
                                        }
                                        break;
                                    case Tui_token_decrement:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                (Tui::cast<TuiVec3>(inPlaceSubRef)->value.x--);
                                                break;
                                            case 'y':
                                                (Tui::cast<TuiVec3>(inPlaceSubRef)->value.y--);
                                                break;
                                            case 'z':
                                                (Tui::cast<TuiVec3>(inPlaceSubRef)->value.z--);
                                                break;
                                        }
                                        break;
                                    case Tui_token_addInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.x += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.y += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.z += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                        break;
                                    case Tui_token_subtractInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.x -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.y -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.z -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                    case Tui_token_multiplyInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.x *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.y *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.z *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                    case Tui_token_divideInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.x /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.y /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec3>(inPlaceSubRef)->value.z /= (Tui::cast<TuiNumber>(rightResult)->value);
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
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.x++);
                                                break;
                                            case 'y':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.y++);
                                                break;
                                            case 'z':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.z++);
                                                break;
                                            case 'w':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.w++);
                                                break;
                                        }
                                        break;
                                    case Tui_token_decrement:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.x--);
                                                break;
                                            case 'y':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.y--);
                                                break;
                                            case 'z':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.z--);
                                                break;
                                            case 'w':
                                                (Tui::cast<TuiVec4>(inPlaceSubRef)->value.w--);
                                                break;
                                        }
                                        break;
                                    case Tui_token_addInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.x += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.y += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.z += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'w':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.w += (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                        break;
                                    case Tui_token_subtractInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.x -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.y -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.z -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'w':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.w -= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                    case Tui_token_multiplyInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.x *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.y *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.z *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'w':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.w *= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                    case Tui_token_divideInPlace:
                                        switch(inPlaceSubTypeAccessKey[0])
                                        {
                                            case 'x':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.x /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'y':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.y /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'z':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.z /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                            case 'w':
                                                Tui::cast<TuiVec4>(inPlaceSubRef)->value.w /= (Tui::cast<TuiNumber>(rightResult)->value);
                                                break;
                                        }
                                    break;
                                };
                            }
                                break;
                        }
                        
                        if(inPlaceEnclosingSetRef)
                        {
                            if(Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet)
                            {
                                Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(inPlaceEnclosingSetRef, inPlaceSetKey, inPlaceSubRef);
                            }
                        }
                        
                        return nullptr;
                    }
                    else
                    {
                        //leftResult = result;
                        TuiParseError(debugInfo, "invalid value");
                        return nullptr;
                    }
                    
                }
                
                if(!leftResult)
                {
                    TuiParseError(debugInfo, "invalid value");
                    return nullptr;
                }
                
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        if(token == Tui_token_increment || token == Tui_token_decrement)
                        {
                            if(token == Tui_token_increment)
                            {
                                (Tui::cast<TuiNumber>(leftResult)->value++);
                            }
                            else //Tui_token_decrement
                            {
                                (Tui::cast<TuiNumber>(leftResult)->value--);
                            }
                            
                            if(inPlaceEnclosingSetRef && Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                            {
                                Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(Tui::cast<TuiTable>(inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                            }
                            return nullptr;
                        }
                        else
                        {
                            (*tokenPos)++;
                            Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo);
                            if(!rightResult || (rightResult->type() != leftType && rightResult->type() != Tui_ref_type_STRING))
                            {
                                TuiParseError(debugInfo, "expected number");
                                return nullptr;
                            }
                            
                            if(rightResult->type() == Tui_ref_type_STRING)
                            {
                                Tui::string returnResult = Tui::create<TuiString>(Tui::cast<TuiNumber>(leftResult)->getStringValue() + (Tui::cast<TuiString>(rightResult)->value));
                                return returnResult;
                            }
                            
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        Tui::cast<TuiNumber>(result)->value = Tui::cast<TuiNumber>(leftResult)->value + (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiNumber>(result)->value += (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_subtract:
                                        Tui::cast<TuiNumber>(result)->value = Tui::cast<TuiNumber>(leftResult)->value - (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiNumber>(result)->value -= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_multiply:
                                        Tui::cast<TuiNumber>(result)->value = Tui::cast<TuiNumber>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiNumber>(result)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiNumber>(result)->value = Tui::cast<TuiNumber>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiNumber>(result)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_modulo:
                                        Tui::cast<TuiNumber>(result)->value = ((int)Tui::cast<TuiNumber>(leftResult)->value) % ((int)(Tui::cast<TuiNumber>(rightResult)->value));
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftResult)->value + (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftResult)->value - (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>(Tui::cast<TuiNumber>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_modulo:
                                    {
                                        Tui::number returnResult = Tui::create<TuiNumber>(((int)Tui::cast<TuiNumber>(leftResult)->value) % ((int)(Tui::cast<TuiNumber>(rightResult)->value)));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiNumber>(leftResult)->value += (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiNumber>(leftResult)->value -= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiNumber>(leftResult)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiNumber>(leftResult)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                        
                                    default:
                                        TuiParseError(debugInfo, "Invalid token");
                                        break;
                                };
                            }
                            
                            
                            if(inPlaceEnclosingSetRef && Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                            {
                                Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(Tui::cast<TuiTable>(inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                            }
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        (*tokenPos)++;
                        Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            TuiParseError(debugInfo, "expected value");
                            return nullptr;
                        }
                        
                        
                        if(rightResult->type() == leftType)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        Tui::cast<TuiVec2>(result)->value = Tui::cast<TuiVec2>(leftResult)->value + (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiVec2>(result)->value += (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_subtract:
                                        Tui::cast<TuiVec2>(result)->value = Tui::cast<TuiVec2>(leftResult)->value - (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiVec2>(result)->value -= (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_multiply:
                                        Tui::cast<TuiVec2>(result)->value = Tui::cast<TuiVec2>(leftResult)->value * (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec2>(result)->value *= (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiVec2>(result)->value = Tui::cast<TuiVec2>(leftResult)->value / (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec2>(result)->value /= (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        Tui::vec2 returnResult = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftResult)->value + (Tui::cast<TuiVec2>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        Tui::vec2 returnResult = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftResult)->value - (Tui::cast<TuiVec2>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        Tui::vec2 returnResult = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftResult)->value * (Tui::cast<TuiVec2>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::vec2 returnResult = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftResult)->value / (Tui::cast<TuiVec2>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiVec2>(leftResult)->value += (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiVec2>(leftResult)->value -= (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec2>(leftResult)->value *= (Tui::cast<TuiVec2>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec2>(leftResult)->value /= (Tui::cast<TuiVec2>(rightResult)->value);
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
                                        Tui::cast<TuiVec2>(result)->value = Tui::cast<TuiVec2>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec2>(result)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiVec2>(result)->value = Tui::cast<TuiVec2>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec2>(result)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                        
                                    default:
                                    {
                                        TuiParseError(debugInfo, "attempt to add or subtract number from vec2");
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
                                        Tui::vec2 returnResult = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::vec2 returnResult = Tui::create<TuiVec2>(Tui::cast<TuiVec2>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec2>(leftResult)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec2>(leftResult)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    default:
                                    {
                                        TuiParseError(debugInfo, "attempt to add or subtract number from vec2");
                                        return nullptr;
                                    }
                                        break;
                                };
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo, "expected vec2 or number");
                            return nullptr;
                        }
                        
                        
                        
                        if(inPlaceEnclosingSetRef && Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(Tui::cast<TuiTable>(inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC3:
                    {
                        (*tokenPos)++;
                        Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            TuiParseError(debugInfo, "expected value");
                            return nullptr;
                        }
                        
                        
                        if(rightResult->type() == leftType)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        Tui::cast<TuiVec3>(result)->value = Tui::cast<TuiVec3>(leftResult)->value + (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiVec3>(result)->value += (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_subtract:
                                        Tui::cast<TuiVec3>(result)->value = Tui::cast<TuiVec3>(leftResult)->value - (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiVec3>(result)->value -= (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_multiply:
                                        Tui::cast<TuiVec3>(result)->value = Tui::cast<TuiVec3>(leftResult)->value * (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec3>(result)->value *= (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiVec3>(result)->value = Tui::cast<TuiVec3>(leftResult)->value / (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec3>(result)->value /= (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        Tui::vec3 returnResult = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftResult)->value + (Tui::cast<TuiVec3>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        Tui::vec3 returnResult = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftResult)->value - (Tui::cast<TuiVec3>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        Tui::vec3 returnResult = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftResult)->value * (Tui::cast<TuiVec3>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::vec3 returnResult = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftResult)->value / (Tui::cast<TuiVec3>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiVec3>(leftResult)->value += (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiVec3>(leftResult)->value -= (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec3>(leftResult)->value *= (Tui::cast<TuiVec3>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec3>(leftResult)->value /= (Tui::cast<TuiVec3>(rightResult)->value);
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
                                        Tui::cast<TuiVec3>(result)->value = Tui::cast<TuiVec3>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec3>(result)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiVec3>(result)->value = Tui::cast<TuiVec3>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec3>(result)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                        
                                    default:
                                    {
                                        TuiParseError(debugInfo, "attempt to add or subtract number from vec3");
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
                                        Tui::vec3 returnResult = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::vec3 returnResult = Tui::create<TuiVec3>(Tui::cast<TuiVec3>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec3>(leftResult)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec3>(leftResult)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    default:
                                    {
                                        TuiParseError(debugInfo, "attempt to add or subtract number from vec3");
                                        return nullptr;
                                    }
                                        break;
                                };
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo, "expected vec2 or number");
                            return nullptr;
                        }
                        
                        
                        if(inPlaceEnclosingSetRef && Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(Tui::cast<TuiTable>(inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC4:
                    {
                        (*tokenPos)++;
                        Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            TuiParseError(debugInfo, "expected value");
                            return nullptr;
                        }
                        
                        
                        if(rightResult->type() == leftType)
                        {
                            if(result && result->type() == leftType)
                            {
                                switch (token) {
                                    case Tui_token_add:
                                        Tui::cast<TuiVec4>(result)->value = Tui::cast<TuiVec4>(leftResult)->value + (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiVec4>(result)->value += (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_subtract:
                                        Tui::cast<TuiVec4>(result)->value = Tui::cast<TuiVec4>(leftResult)->value - (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiVec4>(result)->value -= (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_multiply:
                                        Tui::cast<TuiVec4>(result)->value = Tui::cast<TuiVec4>(leftResult)->value * (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec4>(result)->value *= (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiVec4>(result)->value = Tui::cast<TuiVec4>(leftResult)->value / (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec4>(result)->value /= (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                };
                            }
                            else
                            {
                                switch (token) {
                                    case Tui_token_add:
                                    {
                                        Tui::vec4 returnResult = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftResult)->value + (Tui::cast<TuiVec4>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_subtract:
                                    {
                                        Tui::vec4 returnResult = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftResult)->value - (Tui::cast<TuiVec4>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_multiply:
                                    {
                                        Tui::vec4 returnResult = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftResult)->value * (Tui::cast<TuiVec4>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::vec4 returnResult = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftResult)->value / (Tui::cast<TuiVec4>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_addInPlace:
                                        Tui::cast<TuiVec4>(leftResult)->value += (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_subtractInPlace:
                                        Tui::cast<TuiVec4>(leftResult)->value -= (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec4>(leftResult)->value *= (Tui::cast<TuiVec4>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec4>(leftResult)->value /= (Tui::cast<TuiVec4>(rightResult)->value);
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
                                        Tui::cast<TuiVec4>(result)->value = Tui::cast<TuiVec4>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec4>(result)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divide:
                                        Tui::cast<TuiVec4>(result)->value = Tui::cast<TuiVec4>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec4>(result)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                        
                                    default:
                                    {
                                        TuiParseError(debugInfo, "attempt to add or subtract number from vec4");
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
                                        Tui::vec4 returnResult = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftResult)->value * (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                    case Tui_token_divide:
                                    {
                                        Tui::vec4 returnResult = Tui::create<TuiVec4>(Tui::cast<TuiVec4>(leftResult)->value / (Tui::cast<TuiNumber>(rightResult)->value));
                                        return returnResult;
                                    }
                                        break;
                                        
                                    case Tui_token_multiplyInPlace:
                                        Tui::cast<TuiVec4>(leftResult)->value *= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    case Tui_token_divideInPlace:
                                        Tui::cast<TuiVec4>(leftResult)->value /= (Tui::cast<TuiNumber>(rightResult)->value);
                                        break;
                                    default:
                                    {
                                        TuiParseError(debugInfo, "attempt to add or subtract number from vec4");
                                        return nullptr;
                                    }
                                        break;
                                };
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo, "expected vec4 or number");
                            return nullptr;
                        }
                        
                        if(inPlaceEnclosingSetRef && Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(Tui::cast<TuiTable>(inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                        
                    }
                        break;
                        
                    case Tui_ref_type_STRING: //left type string
                    {
                        (*tokenPos)++;
                        Tui::ref rightResult = runExpression(expression, tokenPos, nullptr, parent, tokenMap, callData, debugInfo, setKey, setIndex, enclosingSetRef, subTypeAccessKey, subTypeRef);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        if(result && result->type() == leftType)
                        {
                            switch (token) {
                                case Tui_token_add:
                                    Tui::cast<TuiString>(result)->value = Tui::cast<TuiString>(leftResult)->value + rightResult->getStringValue();
                                    break;
                                case Tui_token_addInPlace:
                                    Tui::cast<TuiString>(result)->value += rightResult->getStringValue();
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case Tui_token_add:
                                {
                                    Tui::string returnResult = Tui::create<TuiString>(Tui::cast<TuiString>(leftResult)->value + rightResult->getStringValue());
                                    return returnResult;
                                }
                                case Tui_token_addInPlace:
                                    Tui::cast<TuiString>(leftResult)->value += rightResult->getStringValue();
                                    break;
                                    
                                default:
                                    TuiParseError(debugInfo, "Invalid token");
                                    break;
                            };
                        }
                        
                        
                        
                        if(inPlaceEnclosingSetRef && Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet && !inPlaceSetKey.empty())
                        {
                            Tui::cast<TuiTable>(inPlaceEnclosingSetRef)->onSet(Tui::cast<TuiTable>(inPlaceEnclosingSetRef), inPlaceSetKey, leftResult);
                        }
                    }
                        break;
                        
                    default:
                    {
                        TuiParseError(debugInfo, "invalid left value:%s", leftResult->getDebugString().c_str());
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
        Tui::ref foundValue = nullptr;
        if(callData->locals.count(token) != 0)
        {
            foundValue = callData->locals[token];
        }
        else if(tokenMap->refsByToken.count(token) != 0)
        {
            foundValue = tokenMap->refsByToken[token];
        }
        
        if(foundValue)
        {
            if(result && result->type() == foundValue->type() && result->type() != Tui_ref_type_BOOL && result->type() != Tui_ref_type_TABLE && result->type() != Tui_ref_type_FUNCTION)
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
            return TUI_NIL;
        }
    }
    
    return nullptr;
}

void loadTokens(Tui::table parent,
                TuiTokenMap* tokenMap,
                TuiFunctionCallData* callData)
{
    for(auto& varNameAndToken : tokenMap->capturedTokensByVarName)
    {
        if(callData->localTokensByStringKey.count(varNameAndToken.first) == 0)
        {
            Tui::table parentTable = callData->parentTable;
            std::set<Tui::table> found;
            while(parentTable)
            {
                if(parentTable->objectsByStringKey.count(varNameAndToken.first) != 0)
                {
                    Tui::ref var = parentTable->objectsByStringKey[varNameAndToken.first];
                    callData->locals[varNameAndToken.second] = var;
                    callData->localTokensByStringKey[varNameAndToken.first] = varNameAndToken.second;
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
}

Tui::ref TuiFunction::runStatement(TuiStatement* statement,
                                  const Tui::ref& result,
                                  const Tui::table& parent,
                                  TuiTokenMap* tokenMap,
                                  TuiFunctionCallData* callData,
                                  TuiDebugInfo* callingDebugInfo,
                                  bool* breakFound)
{
    TuiDebugInfoLine* debugInfoLine = &statement->debugInfoLine;
    
    switch(statement->type)
    {
        case Tui_statement_type_return:
        {
            return TUI_NIL;
        }
            break;
        case Tui_statement_type_break:
        {
            if(breakFound)
            {
                *breakFound = true;
            }
            return nullptr;
        }
            break;
        case Tui_statement_type_returnExpression:
        {
            Tui::ref existingValue = result;
            uint32_t tokenPos = 0;
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            Tui::ref newResult = runExpression(statement->expression, &tokenPos, existingValue, parent, tokenMap, callData, callingDebugInfo);
            TuiDebugInfoPop(callingDebugInfo);
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
            Tui::ref enclosingSetRef = nullptr;
            Tui::ref subTypeRef = nullptr; //this is set only if a Tui_token_childByString is found eg. color.x or foo.table
            
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, callingDebugInfo, &setKey, &setIndex, &enclosingSetRef, &subTypeSetKey, &subTypeRef);
            TuiDebugInfoPop(callingDebugInfo);
            
            
            if(enclosingSetRef)
            {
                if(Tui::cast<TuiTable>(enclosingSetRef)->onSet && !setKey.empty())
                {
                    Tui::cast<TuiTable>(enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                }
            }
        }
            break;
        case Tui_statement_type_value:
        {
            uint32_t tokenPos = 0;
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            Tui::ref newResult = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, callingDebugInfo);
            TuiDebugInfoPop(callingDebugInfo);
            if(newResult)
            {
                Tui::ref copiedResult = newResult->copy();
                parent->arrayObjects.push_back(copiedResult);
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
            Tui::ref enclosingSetRef = nullptr;
            Tui::ref subTypeRef = nullptr; //this is set only if a Tui_token_childByString is found eg. color.x or foo.table
            
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            
            Tui::ref existingValue = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, callingDebugInfo, &setKey, &setIndex, &enclosingSetRef, &subTypeSetKey, &subTypeRef);
            tokenPos++;
            
            Tui::ref newValue = runExpression(statement->expression, &tokenPos, existingValue, parent, tokenMap, callData, callingDebugInfo);
            
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
                                TuiParseError(callingDebugInfo, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(subTypeSetKey[0])
                            {
                                case 'x':
                                    Tui::cast<TuiVec2>(subTypeRef)->value.x = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                                case 'y':
                                    Tui::cast<TuiVec2>(subTypeRef)->value.y = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                            }
                            
                            if(enclosingSetRef && Tui::cast<TuiTable>(enclosingSetRef)->onSet && !setKey.empty())
                            {
                                Tui::cast<TuiTable>(enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                            }
                        }
                            break;
                        case Tui_ref_type_VEC3:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(callingDebugInfo, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(subTypeSetKey[0])
                            {
                                case 'x':
                                    Tui::cast<TuiVec3>(subTypeRef)->value.x = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                                case 'y':
                                    Tui::cast<TuiVec3>(subTypeRef)->value.y = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                                case 'z':
                                    Tui::cast<TuiVec3>(subTypeRef)->value.z = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                            }
                            
                            if(enclosingSetRef && Tui::cast<TuiTable>(enclosingSetRef)->onSet && !setKey.empty())
                            {
                                Tui::cast<TuiTable>(enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                            }
                        }
                            break;
                        case Tui_ref_type_VEC4:
                        {
                            if(newValue->type() != Tui_ref_type_NUMBER)
                            {
                                TuiParseError(callingDebugInfo, "expected number when assigning vector sub-value, got:%s", (newValue ? newValue->getDebugString().c_str() : "nil"));
                            }
                            switch(subTypeSetKey[0])
                            {
                                case 'x':
                                    Tui::cast<TuiVec4>(subTypeRef)->value.x = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                                case 'y':
                                    Tui::cast<TuiVec4>(subTypeRef)->value.y = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                                case 'z':
                                    Tui::cast<TuiVec4>(subTypeRef)->value.z = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                                case 'w':
                                    Tui::cast<TuiVec4>(subTypeRef)->value.w = (Tui::cast<TuiNumber>(newValue)->value);
                                    break;
                            }
                            
                            if(enclosingSetRef && Tui::cast<TuiTable>(enclosingSetRef)->onSet && !setKey.empty())
                            {
                                Tui::cast<TuiTable>(enclosingSetRef)->onSet(enclosingSetRef, setKey, subTypeRef);
                            }
                        }
                            break;
                        default:
                        {
                            TuiParseError(callingDebugInfo, "expected table or vec object when accessing sub-value, got:%s", (subTypeRef ? subTypeRef->getDebugString().c_str() : "nil"));
                            return nullptr;
                        }
                            break;
                    }
                }
                else if(enclosingSetRef)
                {
                    Tui::ref copiedValue = newValue->copy();
                    if(setIndex != -1)
                    {
                        Tui::cast<TuiTable>(enclosingSetRef)->replace(setIndex, copiedValue);
                    }
                    else
                    {
                        Tui::cast<TuiTable>(enclosingSetRef)->set(setKey, copiedValue, false);
                    }
                }
                else
                {
                    Tui::ref copiedValue = newValue->copy();
                    
                    parent->set(statement->varName, copiedValue, false); //set a new local
                    newValue = copiedValue;
                }
            }
            else if(enclosingSetRef && enclosingSetRef->type() == Tui_ref_type_TABLE)
            {
                if(!setKey.empty())
                {
                    if(Tui::cast<TuiTable>(enclosingSetRef)->onSet)
                    {
                        Tui::cast<TuiTable>(enclosingSetRef)->onSet(Tui::cast<TuiTable>(enclosingSetRef), setKey, existingValue);
                    }
                }
            }
            
            bool skipLocalsDueToSubTableVarChainSet = false;
            if(statement->expression->tokens[0] == Tui_token_varChain)
            {
                skipLocalsDueToSubTableVarChainSet = true;
            }
            
            if(!skipLocalsDueToSubTableVarChainSet && setIndex == -1 && (newValue || existingValue))
            {
                std::string& varName = statement->varName;
                if(enclosingSetRef && !setKey.empty()) //remove if this works
                {
                    varName = setKey;
                }
                
                uint32_t token = 0;
                if(tokenMap->localTokensByVarName.count(varName) != 0)
                {
                    token = tokenMap->localTokensByVarName[varName];
                }
                else if(tokenMap->capturedTokensByVarName.count(varName) != 0)
                {
                    token = tokenMap->capturedTokensByVarName[varName];
                }
                
                Tui::ref newValueToUse = newValue;
                if(!newValue && existingValue)
                {
                    newValueToUse = existingValue;
                }
                
                if(token != 0)
                {
                    Tui::ref prevValue = nullptr;
                    if(callData->locals.count(token) != 0)
                    {
                        prevValue = callData->locals[token];
                    }
                    
                    if(newValueToUse != prevValue)
                    {
                        if(newValueToUse)
                        {
                            callData->locals[token] = newValueToUse;
                            callData->localTokensByStringKey[varName] = token;
                        }
                        else
                        {
                            callData->locals.erase(token);
                            callData->localTokensByStringKey.erase(varName);
                        }
                        
                        
                        if(enclosingSetRef)
                        {
                            TuiFunctionCallData* thisCallData = callData;
                            
                            while(thisCallData)
                            {
                                TuiFunctionCallData* parentCallData = thisCallData->parentCallData;
                                if(parentCallData && parentCallData->localTokensByStringKey.count(varName) != 0)
                                {
                                    uint32_t parentLocalSetToken = parentCallData->localTokensByStringKey[varName];
                                    Tui::ref prevParentLocal = parentCallData->locals[parentLocalSetToken];
                                    parentCallData->locals[parentLocalSetToken] = (newValueToUse ? newValueToUse : TUI_NIL);
                                }
                                
                                if(thisCallData->parentTable == enclosingSetRef && (!parentCallData || parentCallData->parentTable != enclosingSetRef))
                                {
                                    break;
                                }
                                thisCallData = thisCallData->parentCallData;
                            }
                        }
                        
                    }
                    else if(newValueToUse)
                    {
                        newValueToUse = nullptr;
                    }
                }
                else
                {
                    if(enclosingSetRef)
                    {
                        TuiFunctionCallData* thisCallData = callData;
                        
                        while(thisCallData)
                        {
                            TuiFunctionCallData* parentCallData = thisCallData->parentCallData;
                            if(parentCallData && parentCallData->localTokensByStringKey.count(varName) != 0)
                            {
                                uint32_t parentLocalSetToken = parentCallData->localTokensByStringKey[varName];
                                Tui::ref prevParentLocal = parentCallData->locals[parentLocalSetToken];
                                parentCallData->locals[parentLocalSetToken] = (newValueToUse ? newValueToUse : TUI_NIL);
                            }
                            
                            if(thisCallData->parentTable == enclosingSetRef && (!parentCallData || parentCallData->parentTable != enclosingSetRef))
                            {
                                break;
                            }
                            thisCallData = thisCallData->parentCallData;
                        }
                    }
                }
            }
            
            TuiDebugInfoPop(callingDebugInfo);
            
        }
            break;
        case Tui_statement_type_functionCall:
        {
            uint32_t tokenPos = 0;
            //debugInfo->lineNumber = statement->lineNumber; //?
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            Tui::ref result = runExpression(statement->expression, &tokenPos, nullptr, parent, tokenMap, callData, callingDebugInfo);
            TuiDebugInfoPop(callingDebugInfo);
        }
            break;
            
        case Tui_statement_type_forValues:  // for(object in table)
        case Tui_statement_type_forKeyedValues:  // for(indexOrKey, object in table)
        {
            TuiForContainerLoopStatement* forStatement = (TuiForContainerLoopStatement*)statement;
            
            
            Tui::table functionStateTable = Tui::create<TuiTable>(parent);
            TuiFunctionCallData scopedCallData;
            scopedCallData.thisTable = callData->thisTable;
            scopedCallData.parentCallData = callData;
            scopedCallData.parentTable = functionStateTable;
            scopedCallData.transientLoopTables = callData->transientLoopTables;
            scopedCallData.transientLoopTables.push_back(functionStateTable);
            
            loadTokens(functionStateTable, &forStatement->outerTokenMap, &scopedCallData);
            
            uint32_t containerTokenPos = 1;
            
            if(statement->type == Tui_statement_type_forKeyedValues)
            {
                containerTokenPos = 2;
            }
            
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            
            Tui::ref collectionRef = runExpression(forStatement->expression, &containerTokenPos, nullptr, functionStateTable, &forStatement->outerTokenMap, &scopedCallData, callingDebugInfo);
            
            if(!collectionRef || collectionRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "expected table object, got:%s", (collectionRef ? collectionRef->getDebugString().c_str() : "nil"));
                return nullptr;
            }
            
            Tui::table containerObject = Tui::cast<TuiTable>(collectionRef);
            
            Tui::ref runResult = nullptr;
            
            if(!containerObject->arrayObjects.empty())
            {
                int i = 0;
                for(Tui::ref object : containerObject->arrayObjects)
                {
                    Tui::table innerFunctionStateTable = Tui::create<TuiTable>(functionStateTable);
                    TuiFunctionCallData innerScopedCallData;
                    innerScopedCallData.thisTable = callData->thisTable;
                    innerScopedCallData.parentCallData = &scopedCallData;
                    innerScopedCallData.parentTable = innerFunctionStateTable;
                    innerScopedCallData.transientLoopTables = scopedCallData.transientLoopTables;
                    innerScopedCallData.transientLoopTables.push_back(innerFunctionStateTable);
                    
                    loadTokens(innerFunctionStateTable, &forStatement->innerTokenMap, &innerScopedCallData);
                    
                    if(forStatement->indexInnerToken != 0)
                    {
                        Tui::number indexNumber = Tui::create<TuiNumber>(i);
                        innerScopedCallData.locals[forStatement->indexInnerToken] = indexNumber;
                        innerScopedCallData.localTokensByStringKey[forStatement->keyOrIndexName] = forStatement->indexInnerToken;
                        innerFunctionStateTable->set(forStatement->keyOrIndexName, indexNumber);
                    }
                    i++;
                    
                    if(object)
                    {
                        innerScopedCallData.locals[forStatement->objectInnerToken] = object;
                        innerScopedCallData.localTokensByStringKey[forStatement->objectName] = forStatement->objectInnerToken;
                        innerFunctionStateTable->set(forStatement->objectName, object);
                    }
                    
                    bool breakFound = false;
                    runResult = runStatementArray(forStatement->statements, result, innerFunctionStateTable, &forStatement->innerTokenMap, &innerScopedCallData, callingDebugInfo, &breakFound);
                    
                    for(auto& tokenAndRef : innerScopedCallData.locals)
                    {
                        tokenAndRef.second = nullptr; //release 3 4->3
                    }
                    for(Tui::function capturedFunction : innerScopedCallData.capturedFunctions)
                    {
                        capturedFunction->releaseAndRemoveTransientLoopTables(); //delete if needed maybe, and if so remove retain when this is added //release 4 3->2
                    }
                    
                    if(runResult || breakFound)
                    {
                        break;
                    }
                }
            }
            
            if(!runResult && !containerObject->objectsByStringKey.empty())
            {
                
                for(auto& kv : containerObject->objectsByStringKey)
                {
                    Tui::table innerFunctionStateTable = Tui::create<TuiTable>(functionStateTable);
                    TuiFunctionCallData innerScopedCallData;
                    innerScopedCallData.thisTable = callData->thisTable;
                    innerScopedCallData.parentCallData = &scopedCallData;
                    innerScopedCallData.parentTable = innerFunctionStateTable;
                    innerScopedCallData.transientLoopTables = scopedCallData.transientLoopTables;
                    innerScopedCallData.transientLoopTables.push_back(innerFunctionStateTable);
                    
                    loadTokens(parent, &forStatement->innerTokenMap, &innerScopedCallData);
                    
                    if(forStatement->indexInnerToken != 0)
                    {
                        Tui::string keyString = Tui::create<TuiString>(kv.first);
                        innerScopedCallData.locals[forStatement->indexInnerToken] = keyString;
                        innerFunctionStateTable->set(forStatement->keyOrIndexName, keyString);
                    }
                    
                    innerScopedCallData.locals[forStatement->objectInnerToken] = kv.second;
                    innerScopedCallData.localTokensByStringKey[forStatement->objectName] = forStatement->objectInnerToken;
                    innerFunctionStateTable->set(forStatement->objectName, kv.second);
                    
                    bool breakFound = false;
                    runResult = runStatementArray(forStatement->statements, result, innerFunctionStateTable, &forStatement->innerTokenMap, &innerScopedCallData, callingDebugInfo, &breakFound);
                    
                    for(auto& tokenAndRef : innerScopedCallData.locals)
                    {
                        tokenAndRef.second = nullptr;
                    }
                    for(Tui::function capturedFunction : innerScopedCallData.capturedFunctions)
                    {
                        capturedFunction->releaseAndRemoveTransientLoopTables();
                    }
                    
                    if(runResult || breakFound)
                    {
                        break;
                    }
                }
            }
            
            if(!runResult && !containerObject->objectsByNumberKey.empty())
            {
                for(auto& kv : containerObject->objectsByNumberKey)
                {
                    Tui::table innerFunctionStateTable = Tui::create<TuiTable>(functionStateTable);
                    TuiFunctionCallData innerScopedCallData;
                    innerScopedCallData.thisTable = callData->thisTable;
                    innerScopedCallData.parentCallData = &scopedCallData;
                    innerScopedCallData.parentTable = innerFunctionStateTable;
                    innerScopedCallData.transientLoopTables = scopedCallData.transientLoopTables;
                    innerScopedCallData.transientLoopTables.push_back(innerFunctionStateTable);
                    
                    loadTokens(parent, &forStatement->innerTokenMap, &innerScopedCallData);
                    
                    if(forStatement->indexInnerToken != 0)
                    {
                        Tui::ref keyNumber = Tui::create<TuiNumber>(kv.first);
                        innerScopedCallData.locals[forStatement->indexInnerToken] = keyNumber;
                        innerFunctionStateTable->set(forStatement->keyOrIndexName, keyNumber);
                    }
                    
                    innerScopedCallData.locals[forStatement->objectInnerToken] = kv.second;
                    innerScopedCallData.localTokensByStringKey[forStatement->objectName] = forStatement->objectInnerToken;
                    innerFunctionStateTable->set(forStatement->objectName, kv.second);
                    
                    bool breakFound = false;
                    runResult = runStatementArray(forStatement->statements, result, innerFunctionStateTable, &forStatement->innerTokenMap, &innerScopedCallData, callingDebugInfo, &breakFound);
                    
                    for(Tui::function const& capturedFunction : innerScopedCallData.capturedFunctions)
                    {
                        capturedFunction->releaseAndRemoveTransientLoopTables();
                    }
                    
                    if(runResult || breakFound)
                    {
                        break;
                    }
                }
            }
            
            
            for(auto& tokenAndRef : scopedCallData.locals)
            {
                tokenAndRef.second= nullptr;
            }
            for(Tui::function capturedFunction : scopedCallData.capturedFunctions)
            {
                capturedFunction->releaseAndRemoveTransientLoopTables();
            }
            
            TuiDebugInfoPop(callingDebugInfo);
            
            if(runResult)
            {
                return runResult;
            }
            
        }
            break;
            
        case Tui_statement_type_forExpressions: // for(i = 0, i < 5, i++) or while(i < 5)
        {
            TuiForExpressionsStatement* forStatement = (TuiForExpressionsStatement*)statement;
            
            uint32_t tokenPos = 0;
            //debugInfo->lineNumber = statement->lineNumber; //?
            
            Tui::table functionStateTable = Tui::create<TuiTable>(parent);
            TuiFunctionCallData scopedCallData;
            scopedCallData.thisTable = callData->thisTable;
            scopedCallData.parentCallData = callData;
            scopedCallData.parentTable = functionStateTable;
            scopedCallData.transientLoopTables = callData->transientLoopTables;
            scopedCallData.transientLoopTables.push_back(functionStateTable);
            
            loadTokens(functionStateTable, &forStatement->outerTokenMap, &scopedCallData);
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            
            if(forStatement->initialStatement)
            {
                Tui::ref runResult = runStatement(forStatement->initialStatement, nullptr, functionStateTable, &forStatement->outerTokenMap, &scopedCallData, callingDebugInfo);
            }
            
            tokenPos = 0;
            Tui::ref continueResult = runExpression(forStatement->continueExpression, &tokenPos, nullptr, functionStateTable, &forStatement->outerTokenMap, &scopedCallData, callingDebugInfo);
            Tui::ref runResult = nullptr;
            
            while(continueResult && continueResult->boolValue())
            {
                Tui::table innerFunctionStateTable = Tui::create<TuiTable>(functionStateTable);
                TuiFunctionCallData innerScopedCallData;
                innerScopedCallData.thisTable = callData->thisTable;
                innerScopedCallData.parentCallData = &scopedCallData;
                innerScopedCallData.parentTable = innerFunctionStateTable;
                innerScopedCallData.transientLoopTables = scopedCallData.transientLoopTables;
                innerScopedCallData.transientLoopTables.push_back(innerFunctionStateTable);
                
                loadTokens(innerFunctionStateTable, &forStatement->innerTokenMap, &innerScopedCallData);
                
                bool breakFound = false;
                runResult = runStatementArray(forStatement->statements, nullptr, innerFunctionStateTable, &forStatement->innerTokenMap, &innerScopedCallData, callingDebugInfo, &breakFound);
                
                
                for(auto& tokenAndRef : innerScopedCallData.locals)
                {
                    tokenAndRef.second= nullptr;
                }
                for(Tui::function capturedFunction : innerScopedCallData.capturedFunctions)
                {
                    capturedFunction->releaseAndRemoveTransientLoopTables();
                }
                
                
                if(runResult || breakFound)
                {
                    break;
                }
                
                if(forStatement->incrementStatement)
                {
                    Tui::ref runResult = runStatement(forStatement->incrementStatement, nullptr, functionStateTable, &forStatement->outerTokenMap, &scopedCallData,callingDebugInfo);
                }
                
                tokenPos = 0;
                Tui::ref newResult = runExpression(forStatement->continueExpression, &tokenPos, continueResult, functionStateTable, &forStatement->outerTokenMap, &scopedCallData, callingDebugInfo);
                if(newResult)
                {
                    continueResult = newResult;
                }
            }
            
            for(auto& tokenAndRef : scopedCallData.locals)
            {
                tokenAndRef.second= nullptr;
            }
            for(Tui::function capturedFunction : scopedCallData.capturedFunctions)
            {
                capturedFunction->releaseAndRemoveTransientLoopTables();
            }
            
            TuiDebugInfoPop(callingDebugInfo);
            
            if(runResult)
            {
                return runResult;
            }
        }
            break;
        case Tui_statement_type_if:
        {
            TuiIfStatement* currentSatement = (TuiIfStatement*)statement;
            TuiDebugInfoPush(callingDebugInfo, debugInfoLine->fileName, debugInfoLine->lineNumber);
            
            while(currentSatement)
            {
                uint32_t tokenPos = 0;
                Tui::ref expressionResult = runExpression(currentSatement->expression, &tokenPos, nullptr, parent, tokenMap, callData, callingDebugInfo);
                
                bool expressionPass = true;
                if(expressionResult)
                {
                    expressionPass = expressionResult->boolValue();
                }
                
                if(expressionPass)
                {
                    Tui::ref runResult = runStatementArray(currentSatement->statements, result, parent, tokenMap, callData, callingDebugInfo, breakFound);
                    
                    if(runResult)
                    {
                        TuiDebugInfoPop(callingDebugInfo);
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
                            Tui::ref runResult = runStatementArray(currentSatement->elseIfStatement->statements, result, parent, tokenMap, callData,callingDebugInfo, breakFound);
                            if(runResult)
                            {
                                TuiDebugInfoPop(callingDebugInfo);
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
            TuiDebugInfoPop(callingDebugInfo);
        }
            break;
        default:
            break;
    }
    return nullptr;
}

    
Tui::ref TuiFunction::runStatementArray(std::vector<TuiStatement*>& statements_,
                                       const Tui::ref& result,
                                       const Tui::table& parent,
                                       TuiTokenMap* tokenMap,
                                       TuiFunctionCallData* callData,
                                       TuiDebugInfo* debugInfo,
                                       bool* breakFound) //static
{
    for(TuiStatement* statement : statements_)
    {
        if(breakFound && *breakFound)
        {
            return nullptr;
        }
        Tui::ref newResult = TuiFunction::runStatement(statement, result, parent, tokenMap, callData, debugInfo, breakFound);
        if(newResult)
        {
            return newResult;
        }
    }
    
    return nullptr;
}

    
TuiFunction::TuiFunction(Tui::table parentTable_)
:TuiRef()
{
    parentTable = parentTable_;
}


TuiFunction::TuiFunction(std::function<Tui::ref(const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> func_)
:TuiRef()
{
    func = func_;
}

void TuiFunction::releaseAndRemoveTransientLoopTables()
{
    retainedTransientLoopTables.clear();
}

Tui::ref TuiFunction::runTableConstruct(const Tui::table& state,
             const Tui::ref& existingResult,
             TuiDebugInfo* callingDebugInfo)
{
    Tui::table functionStateTable = Tui::create<TuiTable>(state);
    TuiFunctionCallData callData;
    callData.parentTable = parentTable;
    callData.thisTable = parentTable;
    
    for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
    {
        if(callData.locals.count(varNameAndToken.second) == 0)
        {
            if(tokenMap.refsByToken.count(varNameAndToken.second) != 0)
            {
                Tui::ref var = tokenMap.refsByToken[varNameAndToken.second];
                callData.locals[varNameAndToken.second] = var;
            }
            else
            {
                Tui::table parentTable = state;
                std::set<Tui::table> found; //I am not entirely sure what is causing circular references, but I think it is unavoidable sometimes, this ensures no infinite loop here.
                while(parentTable)
                {
                    if(parentTable->objectsByStringKey.count(varNameAndToken.first) != 0)
                    {
                        Tui::ref var = parentTable->objectsByStringKey[varNameAndToken.first];
                        callData.locals[varNameAndToken.second] = var;
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
    }
    
    loadTokens(parentTable, &tokenMap, &callData); //not sure if this is required
    
    Tui::ref result = runStatementArray(statements,  existingResult, functionStateTable, &tokenMap, &callData, callingDebugInfo);
    
    for(auto& tokenAndRef : callData.locals)
    {
        tokenAndRef.second= nullptr;
    }
    
    functionStateTable->parentTable = nullptr;
    
    return functionStateTable;
}


Tui::ref TuiFunction::call(const Tui::table& args,
                          const Tui::ref& existingResult,
                          TuiFunctionCallData* incomingCallData,
                          TuiDebugInfo* callingDebugInfo)
{
    if(func)
    {
        Tui::ref result = func(args, existingResult, incomingCallData, callingDebugInfo);
        return result;
    }
    else
    {
        
        Tui::table functionStateTable = Tui::create<TuiTable>(parentTable);
        TuiFunctionCallData callData;
        callData.parentCallData = incomingCallData;
        callData.parentTable = parentTable;
        callData.thisTable = parentTable;
        if(incomingCallData)
        {
            callData.transientLoopTables = incomingCallData->transientLoopTables;
        }
        callData.transientLoopTables.push_back(functionStateTable);
        
        if(args)
        {
            int i = 0;
            int maxArgs = (int)argNames.size();
            for(Tui::ref arg : args->arrayObjects)
            {
                if(i >= maxArgs)
                {
                    //TuiParseWarn(callingDebugInfo, "Too many arguments supplied to function. ignoring:%s", arg->getDebugString().c_str());
                    continue;
                }
                const std::string& argName = argNames[i];
                Tui::ref copiedArg = arg->copy();
                if(tokenMap.capturedTokensByVarName.count(argName) != 0)
                {
                    uint32_t token = tokenMap.capturedTokensByVarName[argName];
                    callData.locals[token] = copiedArg;
                    callData.localTokensByStringKey[argName] = token;
                }
                functionStateTable->set(argName, copiedArg, false);
                i++;
            }
        }
        
        loadTokens(parentTable, &tokenMap, &callData);
        
        
        TuiDebugInfo debugInfoToUse;
        if(callingDebugInfo)
        {
            TuiDebugInfoCopy(callingDebugInfo, &debugInfoToUse);
        }
        TuiDebugInfoPush(&debugInfoToUse, debugInfoLine.fileName, debugInfoLine.lineNumber);
        
        Tui::ref result = runStatementArray(statements,  existingResult, functionStateTable, &tokenMap, &callData, &debugInfoToUse);
        
        
        
        for(auto& tokenAndRef : callData.locals)
        {
            tokenAndRef.second= nullptr;
        }
        for(Tui::function capturedFunction : callData.capturedFunctions)
        {
            capturedFunction->releaseAndRemoveTransientLoopTables();
        }
        
        
        return result;
    }
}


Tui::ref TuiFunction::call(const std::string& debugName,
                          const Tui::ref& arg1,
                          const Tui::ref& arg2,
                          const Tui::ref& arg3,
                          const Tui::ref& arg4,
                          const Tui::ref& arg5,
                          const Tui::ref& arg6,
                          const Tui::ref& arg7,
                          const Tui::ref& arg8)
{
    TuiDebugInfo debugInfo;
    TuiDebugInfoPush(&debugInfo, debugName, 1);
    Tui::table args = nullptr;
    if(arg1)
    {
        args = Tui::create<TuiTable>();
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
                    if(arg5)
                    {
                        args->arrayObjects.push_back(arg5);
                        if(arg6)
                        {
                            args->arrayObjects.push_back(arg6);
                            if(arg7)
                            {
                                args->arrayObjects.push_back(arg7);
                                if(arg8)
                                {
                                    args->arrayObjects.push_back(arg8);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    Tui::ref result = call(args, nullptr, nullptr, &debugInfo);
    
    return result;
}


Tui::ref TuiFunction::call(TuiFunctionCallData* incomingCallData,
                          TuiDebugInfo* callingDebugInfo,
                          const Tui::ref& arg1,
                          const Tui::ref& arg2,
                          const Tui::ref& arg3,
                          const Tui::ref& arg4,
                          const Tui::ref& arg5,
                          const Tui::ref& arg6,
                          const Tui::ref& arg7,
                          const Tui::ref& arg8)
{
    Tui::table args = nullptr;
    if(arg1)
    {
        args = Tui::create<TuiTable>();
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
                    if(arg5)
                    {
                        args->arrayObjects.push_back(arg5);
                        if(arg6)
                        {
                            args->arrayObjects.push_back(arg6);
                            if(arg7)
                            {
                                args->arrayObjects.push_back(arg7);
                                if(arg8)
                                {
                                    args->arrayObjects.push_back(arg8);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    Tui::ref result = call(args, nullptr, incomingCallData, callingDebugInfo);
    
    return result;
}
