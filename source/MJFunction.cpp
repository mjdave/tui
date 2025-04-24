#include "MJFunction.h"

#include "MJTable.h"
#include "MJRef.h"

MJFunction::MJFunction(MJRef* parent_)
:MJRef(parent_)
{
}


MJFunction::MJFunction(std::function<MJRef*(MJTable* args, MJTable* state)> func_, MJRef* parent_)
:MJRef(parent_)
{
    func = func_;
}

MJFunction::~MJFunction()
{
    for(MJStatement* statement : statements)
    {
        delete statement;
    }
}

void MJFunction::serializeExpression(const char* str, char** endptr, std::string& expression, MJDebugInfo* debugInfo)
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
            expression += *s;
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
            expression += *s;
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
            expression += *s;
            if(!escaped)
            {
                escaped = true;
                continue;
            }
        }
        else if(escaped || singleQuote || doubleQuote)
        {
            expression += *s;
        }
        else if(*s == '\n' || *s == '\0' || *s == ',')
        {
            if(bracketDepth <= 0)
            {
                break;
            }
            
            if(*s == '\n')
            {
                debugInfo->lineNumber++;
            }
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
                    if(bracketDepth == 0)
                    {
                        expression += *s;
                    }
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
            
            expression += *s;
        }
        
        escaped = false;
    }
    expression += '\n'; //bit of a hack, expression string parsing needs a comma or newline to mark the end of the string
    
    s = skipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
}

MJForStatement* MJFunction::loadForStatement(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) //entry point is after 'for'
{
    const char* s = str;
    if(*s == '(')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
    }
    
    
    MJForStatement* statement = new MJForStatement();
    statement->lineNumber = debugInfo->lineNumber;
    

    MJString* varNameRef = MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    
    if(varNameRef && varNameRef->allowAsVariableName)
    {
        statement->varName = varNameRef->value;
        if(*s == '=')
        {
            s++;
            
            MJFunction::serializeExpression(s, endptr, statement->expression, debugInfo);
            
            s = skipToNextChar(*endptr, debugInfo, true);
        }
        else
        {
            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "for statement expected '=' after:%s", varNameRef->getDebugString().c_str());
            return nullptr;
        }
    }
    else
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "for statement bad initial var name:%s", (varNameRef ? varNameRef->value.c_str() : "nil"));
        return nullptr;
    }
    
    varNameRef->release();//todo we will want to reuse this
        
    
    if(*s == ',' || *s == '\n')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
    }
    else
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or '\n'");
        return nullptr;
    }
    
    MJFunction::serializeExpression(s, endptr, statement->continueExpression, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    
    if(*s == ',' || *s == '\n')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
    }
    else
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or '\n'");
        return nullptr;
    }
    
    
    varNameRef = MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    
    if(varNameRef)
    {
        if(varNameRef->isValidFunctionString)
        {
            MJStatement* incrementStatement = new MJStatement(MJSTATEMENT_TYPE_FUNCTION_CALL);
            statement->incrementStatement = incrementStatement;
            incrementStatement->lineNumber = debugInfo->lineNumber;
            incrementStatement->expression = varNameRef->value;
            
            MJFunction::serializeExpression(s, endptr, incrementStatement->expression, debugInfo); //concats functionName with serialized args (*), stores the lot in statement->expression
            
            s = skipToNextChar(*endptr, debugInfo, true);
        }
        else if(varNameRef->allowAsVariableName)
        {
            if(*s == '=')
            {
                s++;
                
                MJStatement* incrementStatement = new MJStatement(MJSTATEMENT_TYPE_VAR_ASSIGN);
                statement->incrementStatement = incrementStatement;
                incrementStatement->lineNumber = debugInfo->lineNumber;
                incrementStatement->varName = varNameRef->value;
                
                MJFunction::serializeExpression(s, endptr, incrementStatement->expression, debugInfo);
                
                s = skipToNextChar(*endptr, debugInfo, true);
            }
            else
            {
                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                return nullptr;
            }
        }
        varNameRef->release();
    }
    
    /*MJFunction::serializeExpression(s, endptr, statement->incrementExpression, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    if(*s == ')')
    {
        s++;
    }
    else
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ')'");
        return false;
    }*/
    
    s = skipToNextChar(s, debugInfo, false);
    
    bool success = MJFunction::loadFunctionBody(s, endptr, parent, debugInfo, &statement->statements);
    if(!success)
    {
        return nullptr;
    }
    
    return statement;
}

bool MJFunction::loadFunctionBody(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo, std::vector<MJStatement*>* statements)
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
        
        if(*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3))))
        {
            s+=3;
            s = skipToNextChar(s, debugInfo);
            
            MJStatement* statement = MJFunction::loadForStatement(s, endptr, parent, debugInfo);
            if(!statement)
            {
                return false;
            }
            statements->push_back(statement);
            s = skipToNextChar(*endptr, debugInfo, false);
            continue;
        }
        
        if(*s == 'i' && *(s + 1) == 'f' && (*(s + 2) == '(' || isspace(*(s + 2))))
        {
            s+=2;
            s = skipToNextChar(s, debugInfo);
            
            MJIfStatement* statement = new MJIfStatement();
            statement->lineNumber = debugInfo->lineNumber;
            
            MJFunction::serializeExpression(s, endptr, statement->expression, debugInfo);
            
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
                        
                        MJFunction::serializeExpression(s, endptr, statement->elseIfStatement->expression, debugInfo);
                        
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
                
                MJFunction::serializeExpression(s, endptr, statement->expression, debugInfo);
                
                s = skipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        
        MJString* varNameRef = MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
        s = skipToNextChar(*endptr, debugInfo);
        
        if(varNameRef)
        {
            if(varNameRef->isValidFunctionString)
            {
                MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_FUNCTION_CALL);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = varNameRef->value;
                
                MJFunction::serializeExpression(s, endptr, statement->expression, debugInfo); //concats functionName with serialized args (*), stores the lot in statement->expression
                
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
                    
                    MJFunction::serializeExpression(s, endptr, statement->expression, debugInfo);
                    
                    s = skipToNextChar(*endptr, debugInfo, true);
                    
                    statements->push_back(statement);
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                    return false;
                }
            }
            varNameRef->release();
        }
    }
    
    s = skipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
    
    return true;
}

MJFunction* MJFunction::initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) //assumes that '(' is currently in str
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
        
        MJFunction* mjFunction = new MJFunction(parent);
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

MJRef* MJFunction::recursivelyRunIfElseStatement(MJIfStatement* elseIfStatement, MJTable* functionState, MJTable* parent, MJDebugInfo* debugInfo)
{
    if(elseIfStatement)
    {
        bool elseIfExpressionPass = true;
        if(!elseIfStatement->expression.empty()) //an 'else' statement is passed through here with no expression, which is a pass
        {
            const char* expressionCString = elseIfStatement->expression.c_str();
            char* endPtr;
            
            debugInfo->lineNumber = elseIfStatement->lineNumber;
            
            MJRef* expressionResult = recursivelyLoadValue(expressionCString,
                                                           &endPtr,
                                                           nullptr,
                                                           functionState,
                                                           debugInfo,
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
            MJRef* result = runStatementArray(elseIfStatement->statements, functionState, parent, debugInfo);
            
            if(result)
            {
                return result;
            }
        }
    }
    return  nullptr;
}

MJRef* MJFunction::runStatement(MJStatement* statement, MJTable* functionState, MJTable* parent, MJDebugInfo* debugInfo)
{
    switch(statement->type)
    {
        case MJSTATEMENT_TYPE_RETURN:
        {
            return new MJRef(parent);
        }
            break;
        case MJSTATEMENT_TYPE_RETURN_EXPRESSION:
        {
            const char* cString = statement->expression.c_str();
            char* endPtr;
            
            debugInfo->lineNumber = statement->lineNumber;
            
            MJRef* result = recursivelyLoadValue(cString,
                                    &endPtr,
                                        nullptr,
                                 functionState,
                                                 debugInfo,
                                 true,
                                                 true);
            
            if(!result)
            {
                result = new MJRef(parent);
            }
            return result;
        }
            break;
        case MJSTATEMENT_TYPE_VAR_ASSIGN:
        {
            const char* expressionCString = statement->expression.c_str();
            char* endPtr;
            
            debugInfo->lineNumber = statement->lineNumber;
            
            
            
            MJRef* result = recursivelyLoadValue(expressionCString,
                                                 &endPtr,
                                                 nullptr,
                                                 functionState,
                                                 debugInfo,
                                                 true,
                                                 true);
            
            const char* varNameCString = statement->varName.c_str();
            MJString* variableNameString = MJString::initWithHumanReadableString(varNameCString, &endPtr, parent, debugInfo);
            
            setVariable(variableNameString,
                        result,
                        functionState,
                        debugInfo);
            
            variableNameString->release();
            result->release();
            
        }
            break;
        case MJSTATEMENT_TYPE_FUNCTION_CALL:
        {
            const char* cString = statement->expression.c_str();
            char* endPtr;
            
            debugInfo->lineNumber = statement->lineNumber;
            
            MJRef* result = recursivelyLoadValue(cString,
                                    &endPtr,
                                        nullptr,
                                 functionState,
                                                 debugInfo,
                                 true,
                                                 true);
            if(result)
            {
                result->release();
            }
        }
            break;
            
        case MJSTATEMENT_TYPE_FOR:
        {
            const char* expressionCString = statement->expression.c_str(); //var assign
            char* endPtr;
            
            debugInfo->lineNumber = statement->lineNumber;
            
            MJRef* result = recursivelyLoadValue(expressionCString,
                                                 &endPtr,
                                                 nullptr,
                                                 functionState,
                                                 debugInfo,
                                                 true,
                                                 true);
            
            MJNumber* iterator = (MJNumber*)result;
            
            MJRef* prevI = nullptr;
            
            if(functionState->objectsByStringKey.count(statement->varName) != 0)
            {
                prevI = functionState->objectsByStringKey[statement->varName];
            }
            
            functionState->objectsByStringKey[statement->varName] = iterator;
            
            
            bool run = true;
            while(run)
            {
                bool expressionPass = true;
                if(!((MJForStatement*)statement)->continueExpression.empty())
                {
                    const char* expressionCString = ((MJForStatement*)statement)->continueExpression.c_str();
                    char* endPtr;
                    
                    debugInfo->lineNumber = statement->lineNumber;
                    
                    MJRef* expressionResult = recursivelyLoadValue(expressionCString,
                                                                   &endPtr,
                                                                   nullptr,
                                                                   functionState,
                                                                   debugInfo,
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
                
                if(!expressionPass)
                {
                    break;
                }
                
                MJRef* result = runStatementArray(((MJForStatement*)statement)->statements, functionState, parent, debugInfo);
                
                if(result)
                {
                    if(prevI)
                    {
                        functionState->objectsByStringKey[statement->varName] = prevI;
                    }
                    else
                    {
                        functionState->objectsByStringKey.erase(statement->varName);
                    }
                    iterator->release();
                    return result;
                }
                
                
                if(((MJForStatement*)statement)->incrementStatement)
                {
                    MJRef* result = runStatement(((MJForStatement*)statement)->incrementStatement, functionState, parent, debugInfo);
                    if(result)
                    {
                        if(prevI)
                        {
                            functionState->objectsByStringKey[statement->varName] = prevI;
                        }
                        else
                        {
                            functionState->objectsByStringKey.erase(statement->varName);
                        }
                        iterator->release();
                        return result;
                    }
                }
                
            }
            
            if(prevI)
            {
                functionState->objectsByStringKey[statement->varName] = prevI;
            }
            else
            {
                functionState->objectsByStringKey.erase(statement->varName);
            }
            iterator->release();
        }
            break;
        case MJSTATEMENT_TYPE_IF:
        {
            bool expressionPass = true;
            if(!statement->expression.empty()) //an 'else' statement is passed through here with no expression, which is a pass
            {
                const char* expressionCString = statement->expression.c_str();
                char* endPtr;
                
                debugInfo->lineNumber = statement->lineNumber;
                
                MJRef* expressionResult = recursivelyLoadValue(expressionCString,
                                                               &endPtr,
                                                               nullptr,
                                                               functionState,
                                                               debugInfo,
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
                MJRef* result = runStatementArray(((MJIfStatement*)statement)->statements, functionState, parent, debugInfo);
                
                if(result)
                {
                    return result;
                }
            }
            else
            {
                MJRef* result = recursivelyRunIfElseStatement(((MJIfStatement*)statement)->elseIfStatement, functionState, parent, debugInfo);
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
    return nullptr;
}

MJRef* MJFunction::runStatementArray(std::vector<MJStatement*>& statements_, MJTable* functionState, MJTable* parent, MJDebugInfo* debugInfo) //static
{
    for(MJStatement* statement : statements_)
    {
        MJRef* result = MJFunction::runStatement(statement, functionState, parent, debugInfo);
        if(result)
        {
            return result;
        }
    }
    
    return nullptr;
}


MJRef* MJFunction::call(MJTable* args, MJTable* callLocationState)
{
    if(func)
    {
        MJTable* currentCallState = new MJTable(parent);
        MJRef* result = func(args, currentCallState);
        currentCallState->release();
        return result;
    }
    else
    {
        MJTable* currentCallState = new MJTable(parent);
        
        int i = 0;
        int maxArgs = (int)argNames.size();
        for(MJRef* arg : args->arrayObjects)
        {
            if(i >= maxArgs)
            {
                MJSError(debugInfo.fileName.c_str(), 0, "Too many arguments supplied to function");
            }
            currentCallState->objectsByStringKey[argNames[i]] = arg;
            i++;
        }
        
        
        MJRef* result = runStatementArray(statements, currentCallState, (MJTable*)parent, &debugInfo);
        
        currentCallState->release();
        
        return result;
    }
}


MJRef* MJFunction::recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, int varStartIndex)
{
    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to get a variable from within a child function");
    return nullptr;
    //return currentCallState->recursivelyFindVariable(variableName, debugInfo, varStartIndex);
}


bool MJFunction::recursivelySetVariable(MJString* variableName, MJRef* value, MJDebugInfo* debugInfo, int varStartIndex)
{
    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to set variable within a child function");
    return false;
   // return currentCallState->recursivelySetVariable(variableName, value, debugInfo, varStartIndex);
}
