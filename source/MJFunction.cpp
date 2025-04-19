#include "MJFunction.h"

#include "MJTable.h"
#include "MJRef.h"

MJFunction::MJFunction()
{
    //state = new MJTable(parent);
}

MJFunction::~MJFunction()
{
    for(MJStatement* statement : statements)
    {
        delete statement;
    }
}

void serializeExpression(const char* str, char** endptr, MJStatement* statement, MJDebugInfo* debugInfo)
{
    const char* s = str;
    int bracketDepth = 0;
    
    bool singleQuote = false;
    bool doubleQuote = false;
    bool escaped = false;
    
    for(;; s++)
    {
        s = skipToNextChar(s, debugInfo, true);
        if(*s == '\'')
        {
            statement->expression += *s;
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
                    singleQuote = true;
                    continue;
                }
            }
        }
        else if(*s == '"')
        {
            statement->expression += *s;
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
                    doubleQuote = true;
                    continue;
                }
            }
        }
        else if(*s == '\\')
        {
            statement->expression += *s;
            if(!escaped)
            {
                escaped = true;
                continue;
            }
        }
        else if(escaped || singleQuote || doubleQuote)
        {
            statement->expression += *s;
        }
        else if(*s == '\n' || *s == '\0')
        {
            if(bracketDepth <= 0)
            {
                break;
            }
            
            debugInfo->lineNumber++;
        }
        else if(!isspace(*s))
        {
            if(*s == '(')
            {
                bracketDepth++;
            }
            else if(*s == ')')
            {
                bracketDepth--;
                if(bracketDepth <= 0)
                {
                    statement->expression += *s;
                    s++;
                    break;
                }
            }
            
            if(bracketDepth <= 0)
            {
                if(*s == '}')
                {
                    break;
                }
            }
            
            statement->expression += *s;
        }
        
        escaped = false;
    }
    statement->expression += '\n'; //bit of a hack, expression string parsing needs a comma or newline to mark the end of the string
    
    s = skipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
}

bool loadFunctionBody(const char* str, char** endptr, MJTable* parent, MJDebugInfo* debugInfo, std::vector<MJStatement*>* statements)
{
    const char* s = str;
    if(*s != '{')
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Function expected opening brace");
        return false;
    }
    s++;
    s = skipToNextChar(s, debugInfo);
    
    while(1)
    {
        if(*s == '}')
        {
            s++;
            break;
        }
        
        if(*s == 'i' && *(s + 1) == 'f' && (*(s + 2) == '(' || isspace(*(s + 2))))
        {
            s+=2;
            s = skipToNextChar(s, debugInfo);
            
            MJIfStatement* statement = new MJIfStatement();
            statement->lineNumber = debugInfo->lineNumber;
            
            serializeExpression(s, endptr, statement, debugInfo);
            
            s = skipToNextChar(*endptr, debugInfo);
            
            bool success = loadFunctionBody(s, endptr, parent, debugInfo, &statement->statements);
            if(!success)
            {
                return false;
            }
            
            s = skipToNextChar(*endptr, debugInfo);
            
            while(1)
            {
                if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
                {
                    s+=4;
                    s = skipToNextChar(s, debugInfo);
                    if(*s == '{')
                    {
                        statement->elseIfStatement = new MJIfStatement();
                        statement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        
                        bool success = loadFunctionBody(s, endptr, parent, debugInfo, &statement->elseIfStatement->statements);
                        if(!success)
                        {
                            return false;
                        }
                        s = skipToNextChar(*endptr, debugInfo);
                    }
                    else if(*s == 'i' && *(s + 1) == 'f')
                    {
                        s+=2;
                        s = skipToNextChar(s, debugInfo);
                        
                        statement->elseIfStatement = new MJIfStatement();
                        statement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        
                        serializeExpression(s, endptr, statement->elseIfStatement, debugInfo);
                        
                        s = skipToNextChar(*endptr, debugInfo);
                        
                        bool success = loadFunctionBody(s, endptr, parent, debugInfo, &statement->elseIfStatement->statements);
                        if(!success)
                        {
                            return false;
                        }
                        s = skipToNextChar(*endptr, debugInfo);
                    }
                    else
                    {
                        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else statement expected 'if' or '{'");
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
            s = skipToNextChar(s, debugInfo);
            if(*s == '}')
            {
                MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_RETURN);
                statements->push_back(statement);
                s++;
                s = skipToNextChar(s, debugInfo, true);
                break;
            }
            else
            {
                MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_RETURN_EXPRESSION);
                statement->lineNumber = debugInfo->lineNumber;
                
                serializeExpression(s, endptr, statement, debugInfo);
                
                s = skipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        
        MJString* varNameRef = MJString::initWithHumanReadableString(s, endptr, debugInfo);
        s = skipToNextChar(*endptr, debugInfo);
        
        if(varNameRef)
        {
            if(varNameRef->isValidFunctionString)
            {
                MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_FUNCTION_CALL);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = varNameRef->value;
                
                serializeExpression(s, endptr, statement, debugInfo); //concats functionName with serialized args (*), stores the lot in statement->expression
                
                s = skipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
            else if(varNameRef->allowAsVariableName)
            {
                if(*s == '=')
                {
                    s++;
                    
                    MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_VAR_ASSIGN);
                    statement->lineNumber = debugInfo->lineNumber;
                    statement->varName = varNameRef->value;
                    
                    serializeExpression(s, endptr, statement, debugInfo);
                    
                    s = skipToNextChar(*endptr, debugInfo, true);
                    
                    statements->push_back(statement);
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                    return false;
                }
            }
            delete varNameRef;
        }
    }
    
    s = skipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
    
    return true;
}

MJFunction* MJFunction::initWithHumanReadableString(const char* str, char** endptr, MJTable* parent, MJDebugInfo* debugInfo) //assumes that '(' is currently in str
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
        
        s = skipToNextChar(s, debugInfo);
        
        MJFunction* mjFunction = new MJFunction();
        mjFunction->debugInfo.fileName = debugInfo->fileName;
        
        std::string currentVarName = "";
        
        for(;; s++)
        {
            s = skipToNextChar(s, debugInfo, true);
            if(*s == ')' || *s == '\0')
            {
                if(!currentVarName.empty())
                {
                    mjFunction->argNames.push_back(currentVarName);
                    //MJLog("found argVarName:%s", currentVarName.c_str());
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
                    //MJLog("found argVarName:%s", currentVarName.c_str());
                    currentVarName = "";
                }
            }
            else if(!isspace(*s))
            {
                currentVarName += *s;
            }
        }
        
        s = skipToNextChar(s, debugInfo);
        
        bool success = loadFunctionBody(s, endptr, parent, debugInfo, &mjFunction->statements);
        if(!success)
        {
            delete mjFunction;
            return nullptr;
        }
        
        s = skipToNextChar(*endptr, debugInfo, true);
        *endptr = (char*)s;
        return mjFunction;
        
    }
    
    return nullptr;
}

MJRef* MJFunction::recursivelyRunIfElseStatement(MJIfStatement* elseIfStatement, MJTable* functionState)
{
    if(elseIfStatement)
    {
        bool elseIfExpressionPass = true;
        if(!elseIfStatement->expression.empty()) //an 'else' statement is passed through here with no expression, which is a pass
        {
            const char* expressionCString = elseIfStatement->expression.c_str();
            char* endPtr;
            
            debugInfo.lineNumber = elseIfStatement->lineNumber;
            
            MJRef* expressionResult = recursivelyLoadValue(expressionCString,
                                                           &endPtr,
                                                           nullptr,
                                                           functionState,
                                                           &debugInfo,
                                                           true,
                                                           false);
            if(expressionResult)
            {
                elseIfExpressionPass = expressionResult->boolValue();
                expressionResult->release();
            }
            else
            {
                elseIfExpressionPass = false;
            }
        }
        
        if(elseIfExpressionPass)
        {
            MJRef* result = runStatementArray(elseIfStatement->statements, functionState);
            
            if(result)
            {
                return result;
            }
        }
    }
    return  nullptr;
}


MJRef* MJFunction::runStatementArray(std::vector<MJStatement*>& statements_, MJTable* functionState)
{
    for(MJStatement* statement : statements_)
    {
        switch(statement->type)
        {
            case MJSTATEMENT_TYPE_RETURN:
            {
                return new MJRef();
            }
                break;
            case MJSTATEMENT_TYPE_RETURN_EXPRESSION:
            {
                const char* cString = statement->expression.c_str();
                char* endPtr;
                
                debugInfo.lineNumber = statement->lineNumber;
                
                MJRef* result = recursivelyLoadValue(cString,
                                        &endPtr,
                                            nullptr,
                                     functionState,
                                                     &debugInfo,
                                     true,
                                                     true);
                
                if(!result)
                {
                    result = new MJRef();
                }
                return result;
            }
                break;
            case MJSTATEMENT_TYPE_VAR_ASSIGN:
            {
                const char* expressionCString = statement->expression.c_str();
                char* endPtr;
                
                debugInfo.lineNumber = statement->lineNumber;
                
                
                
                MJRef* result = recursivelyLoadValue(expressionCString,
                                                     &endPtr,
                                                     nullptr,
                                                     functionState,
                                                     &debugInfo,
                                                     true,
                                                     true);
                
                const char* varNameCString = statement->varName.c_str();
                MJString* variableNameString = MJString::initWithHumanReadableString(varNameCString, &endPtr, &debugInfo);
                
                setVariable(variableNameString,
                            result,
                            functionState,
                            &debugInfo);
                
                variableNameString->release();
                result->release();
                
            }
                break;
            case MJSTATEMENT_TYPE_FUNCTION_CALL:
            {
                const char* cString = statement->expression.c_str();
                char* endPtr;
                
                debugInfo.lineNumber = statement->lineNumber;
                
                MJRef* result = recursivelyLoadValue(cString,
                                        &endPtr,
                                            nullptr,
                                     functionState,
                                                     &debugInfo,
                                     true,
                                                     true);
                result->release();
            }
                break;
            case MJSTATEMENT_TYPE_IF:
            {
                bool expressionPass = true;
                if(!statement->expression.empty()) //an 'else' statement is passed through here with no expression, which is a pass
                {
                    const char* expressionCString = statement->expression.c_str();
                    char* endPtr;
                    
                    debugInfo.lineNumber = statement->lineNumber;
                    
                    MJRef* expressionResult = recursivelyLoadValue(expressionCString,
                                                                   &endPtr,
                                                                   nullptr,
                                                                   functionState,
                                                                   &debugInfo,
                                                                   true,
                                                                   false);
                    
                    if(expressionResult)
                    {
                        expressionPass = expressionResult->boolValue();
                        expressionResult->release();
                    }
                    else
                    {
                        expressionPass = false;
                    }
                }
                if(expressionPass)
                {
                    MJRef* result = runStatementArray(((MJIfStatement*)statement)->statements, functionState);
                    
                    if(result)
                    {
                        return result;
                    }
                }
                else
                {
                    MJRef* result = recursivelyRunIfElseStatement(((MJIfStatement*)statement)->elseIfStatement, functionState);
                    if(result)
                    {
                        return result;
                    }
                }
            }
                break;
            default:
                break;
        }
    }
    
    return nullptr;
}


MJRef* MJFunction::call(MJTable* args, MJTable* state)
{
    MJTable* functionState = new MJTable(state);
    
    int i = 0;
    int maxArgs = (int)argNames.size();
    for(MJRef* arg : args->arrayObjects)
    {
        if(i >= maxArgs)
        {
            MJSError(debugInfo.fileName.c_str(), 0, "Too many arguments supplied to function");
        }
        functionState->objectsByStringKey[argNames[i]] = arg;
        i++;
    }
    
    
    MJRef* result = runStatementArray(statements, functionState);
    
    functionState->release();
    
    return result;
}
