#include "MJFunction.h"

#include "MJTable.h"
#include "MJRef.h"


void MJFunction::recursivelySerializeExpression(const char* str,
                            char** endptr,
                            MJExpression* expression,
                            MJRef* parent,
                            MJTokenMap* tokenMap,
                            MJDebugInfo* debugInfo,
                            bool runLowOperators)
{
    const char* s = str;
    
    uint32_t tokenIndex = (uint32_t)expression->tokens.size();
    expression->tokens.push_back(MJ_TOKEN_pad);
    uint32_t leftTokenTypeMarker = MJ_TOKEN_nil;
    
    
    if(*s == '(')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
        recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, true);
        s = skipToNextChar(*endptr, debugInfo, true);
        s++; //')'
        s = skipToNextChar(s, debugInfo, true);
    }
    else
    {
        MJRef* leftValue = MJTable::initUnknownTypeRefWithHumanReadableString(s, endptr, parent, debugInfo);
        s = skipToNextChar(*endptr, debugInfo, true);
        
        if(leftValue->type() == MJREF_TYPE_STRING)
        {
            MJString* varString = ((MJString*)leftValue);
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
                    
                    MJRef* newValueRef = parent->recursivelyFindVariable(varString, debugInfo, true);
                    if(newValueRef)
                    {
                        tokenMap->refsByToken[leftVarToken] = newValueRef;
                    }
                    //MJRef* newValueRef = loadVariableIfAvailable(varString, existingRef, s, endptr, (MJTable*)parent, debugInfo); //cast dangerous?
                    
                   // s = skipToNextChar(*endptr, debugInfo, true);
                    
                }
                
                expression->tokens.push_back(leftVarToken);
                
                
                if(varString->isValidFunctionString && *s =='(')
                {
                    leftTokenTypeMarker = MJ_TOKEN_functionCall;
                    s++;
                    
                    while(*s != ')' && *s != '\0')
                    {
                        if(*s == ',')
                        {
                            s++;
                        }
                        recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, true);
                        s = skipToNextChar(*endptr, debugInfo, true);
                    }
                    s++;
                    
                    expression->tokens.push_back(MJ_TOKEN_end);
                    s = skipToNextChar(s, debugInfo, true);
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
            if(leftValue->type() != MJREF_TYPE_NIL)
            {
                uint32_t leftVarToken = tokenMap->tokenIndex++;
                tokenMap->refsByToken[leftVarToken] = leftValue;
                expression->tokens.push_back(leftVarToken);
            }
            else
            {
                expression->tokens.push_back(MJ_TOKEN_nil);
            }
        }
    }
        
    char operatorChar = *s;
    char secondOperatorChar = *(s + 1);
    
    if(MJExpressionOperatorsSet.count(operatorChar) == 0 || (operatorChar == '=' && secondOperatorChar != '=') || (!runLowOperators && (operatorChar == '+' || operatorChar == '-')))
    {
        s = skipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        
        if(leftTokenTypeMarker != MJ_TOKEN_nil)
        {
            expression->tokens[tokenIndex] = leftTokenTypeMarker;
        }
        
        return;
    }
    
    s++;
    
    if(secondOperatorChar == '=' && (operatorChar == '=' || operatorChar == '>' || operatorChar == '<') )
    {
        s++;
    }
    
    s = skipToNextChar(s, debugInfo, true);
    
    switch(operatorChar)
    {
        case '+':
        {
            expression->tokens[tokenIndex] = MJ_TOKEN_add;
        }
            break;
        case '-':
        {
            expression->tokens[tokenIndex] = MJ_TOKEN_subtract;
        }
            break;
        case '*':
        {
            expression->tokens[tokenIndex] = MJ_TOKEN_multiply;
        }
            break;
        case '/':
        {
            expression->tokens[tokenIndex] = MJ_TOKEN_divide;
        }
            break;
        case '>':
        {
            if(secondOperatorChar == '=')
            {
                expression->tokens[tokenIndex] = MJ_TOKEN_greaterEqualTo;
            }
            else
            {
                expression->tokens[tokenIndex] = MJ_TOKEN_greaterThan;
            }
        }
        break;
        case '<':
        {
            if(secondOperatorChar == '=')
            {
                expression->tokens[tokenIndex] = MJ_TOKEN_lessEqualTo;
            }
            else
            {
                expression->tokens[tokenIndex] = MJ_TOKEN_lessThan;
            }
        }
        break;
        case '=':
        {
            expression->tokens[tokenIndex] = MJ_TOKEN_equalTo;
        }
        break;
        default:
            break;
    }
    
    
    recursivelySerializeExpression(s,
                                   endptr,
                                   expression,
                                   parent,
                                   tokenMap,
                                   debugInfo,
                                   false);
    
    s = skipToNextChar(*endptr, debugInfo, true);
    *endptr = (char*)s;
    
}

MJForStatement* MJFunction::serializeForStatement(const char* str, char** endptr, MJRef* parent,  MJTokenMap* tokenMap, MJDebugInfo* debugInfo) //entry point is after 'for'
{
    const char* s = str;
    if(*s == '(')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
    }
    
    MJForStatement* forStatement = new MJForStatement();
    forStatement->lineNumber = debugInfo->lineNumber;
    forStatement->expression = new MJExpression();
    forStatement->continueExpression = new MJExpression();
    
    MJString* varNameRef = MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    
    if(varNameRef && varNameRef->allowAsVariableName)
    {
        if(*s == '=')
        {
            s++;
            s = skipToNextChar(s, debugInfo);
            
            forStatement->varName = varNameRef;
            recursivelySerializeExpression(s, endptr, forStatement->expression, parent, tokenMap, debugInfo, true);
            s = skipToNextChar(*endptr, debugInfo, true);
            
            if(tokenMap->tokensByVarNames.count(varNameRef->value) != 0)
            {
                forStatement->varToken = tokenMap->tokensByVarNames[varNameRef->value];
            }
            else
            {
                forStatement->varToken = tokenMap->tokenIndex++;
                tokenMap->tokensByVarNames[varNameRef->value] = forStatement->varToken;
            }
        }
        else
        {
            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
            varNameRef->release();
            return nullptr;
        }
    }
    else
    {
        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected variable assignment in for loop");
        return nullptr;
    }
    
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
    
    
    recursivelySerializeExpression(s, endptr, forStatement->continueExpression, parent, tokenMap, debugInfo, true);
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
    
    //todo this is c/p from below. Should be a factored out
    const char* varNameStartS = s; //rewind if we find a function
    varNameRef = MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    
    if(varNameRef)
    {
        if(varNameRef->isValidFunctionString)
        {
            MJStatement* incrementStatement = new MJStatement(MJSTATEMENT_TYPE_FUNCTION_CALL);
            forStatement->incrementStatement = incrementStatement;
            incrementStatement->lineNumber = debugInfo->lineNumber;
            incrementStatement->expression = new MJExpression();
            
            //s++; //'('
            //s = skipToNextChar(s, debugInfo);
            recursivelySerializeExpression(varNameStartS, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, true);
            
            s = skipToNextChar(*endptr, debugInfo, true);
            if(*s == ')')
            {
                s++;
                s = skipToNextChar(s, debugInfo, true);
            }
            
            varNameRef->release();
        }
        else if(varNameRef->allowAsVariableName)
        {
            if(*s == '=')
            {
                s++;
                s = skipToNextChar(s, debugInfo);
                
                MJStatement* incrementStatement = new MJStatement(MJSTATEMENT_TYPE_VAR_ASSIGN);
                forStatement->incrementStatement = incrementStatement;
                incrementStatement->lineNumber = debugInfo->lineNumber;
                incrementStatement->varName = varNameRef;
                
                
                incrementStatement->expression = new MJExpression();
                recursivelySerializeExpression(s, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, true);
                
                s = skipToNextChar(*endptr, debugInfo, true);
                
                
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
                varNameRef->release();
                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                return nullptr;
            }
        }
    }
    
    if(*s == ')')
    {
        s++;
    }
    s = skipToNextChar(s, debugInfo);
    
    bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &forStatement->statements);
    if(!success)
    {
        return nullptr;
    }
    
    s = skipToNextChar(*endptr, debugInfo);
    
    return forStatement;
    
    /*const char* s = str;
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
        statement->varName = varNameRef;
        
        if(tokenMap->tokensByVarNames.count(varNameRef->value) != 0)
        {
            statement->varToken = tokenMap->tokensByVarNames[varNameRef->value];
        }
        else
        {
            statement->varToken = tokenMap->tokenIndex++;
            tokenMap->tokensByVarNames[varNameRef->value] = statement->varToken;
        }
        
        if(*s == '=')
        {
            s++;
            
            recursivelySerializeExpression(s,
                                        endptr,
                                           statement->expression,
                                        parent,
                                        tokenMap,
                                        debugInfo,
                                        true);
            
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
    
    recursivelySerializeExpression(s, endptr, statement->continueExpression, parent, tokenMap, debugInfo, true);
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
            
            recursivelySerializeExpression(s, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, true);
            
            s = skipToNextChar(*endptr, debugInfo, true);
            varNameRef->release();
        }
        else if(varNameRef->allowAsVariableName)
        {
            if(*s == '=')
            {
                s++;
                
                MJStatement* incrementStatement = new MJStatement(MJSTATEMENT_TYPE_VAR_ASSIGN);
                statement->incrementStatement = incrementStatement;
                incrementStatement->lineNumber = debugInfo->lineNumber;
                incrementStatement->varName = varNameRef;
                
                if(tokenMap->tokensByVarNames.count(varNameRef->value) != 0)
                {
                    incrementStatement->varToken = tokenMap->tokensByVarNames[varNameRef->value];
                }
                else
                {
                    incrementStatement->varToken = tokenMap->tokenIndex++;
                    tokenMap->tokensByVarNames[varNameRef->value] = incrementStatement->varToken;
                }
                
                recursivelySerializeExpression(s, endptr, incrementStatement->expression, parent, tokenMap, debugInfo, true);
                
                s = skipToNextChar(*endptr, debugInfo, true);
            }
            else
            {
                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                return nullptr;
            }
        }
    }
    
    s = skipToNextChar(s, debugInfo, false);
    
    bool success = MJFunction::serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &statement->statements);
    if(!success)
    {
        return nullptr;
    }
    
    return statement;*/
}

bool MJFunction::serializeFunctionBody(const char* str, char** endptr, MJRef* parent, MJTokenMap* tokenMap, MJDebugInfo* debugInfo, std::vector<MJStatement*>* statements)
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
            
            MJStatement* statement = MJFunction::serializeForStatement(s, endptr, parent, tokenMap, debugInfo);
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
            statement->expression = new MJExpression();
            
            recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, true);
            
            s = skipToNextChar(*endptr, debugInfo);
            if(*s == ')')
            {
                s++;
                s = skipToNextChar(s, debugInfo);
            }
            
            bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &statement->statements);
            if(!success)
            {
                return false;
            }
            
            s = skipToNextChar(*endptr, debugInfo);
            
            MJIfStatement* currentStatement = statement;
            while(1)
            {
                if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
                {
                    s+=4;
                    s = skipToNextChar(s, debugInfo);
                    if(*s == '{')
                    {
                        currentStatement->elseIfStatement = new MJIfStatement();
                        currentStatement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        
                        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &currentStatement->elseIfStatement->statements);
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
                        
                        currentStatement->elseIfStatement = new MJIfStatement();
                        currentStatement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        currentStatement->elseIfStatement->expression = new MJExpression();
                        
                        recursivelySerializeExpression(s, endptr, currentStatement->elseIfStatement->expression, parent, tokenMap, debugInfo, true);
                        
                        s = skipToNextChar(*endptr, debugInfo);
                        
                        if(*s == ')')
                        {
                            s++;
                            s = skipToNextChar(s, debugInfo);
                        }
                        
                        bool success = serializeFunctionBody(s, endptr, parent, tokenMap, debugInfo, &currentStatement->elseIfStatement->statements);
                        if(!success)
                        {
                            return false;
                        }
                        s = skipToNextChar(*endptr, debugInfo);
                        currentStatement = currentStatement->elseIfStatement;
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
                statement->expression = new MJExpression();
                
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, true);
                
                s = skipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        
        const char* varNameStartS = s; //rewind if we find a function
        MJString* varNameRef = MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
        s = skipToNextChar(*endptr, debugInfo);
        
        if(varNameRef)
        {
            if(varNameRef->isValidFunctionString)
            {
                MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_FUNCTION_CALL);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = new MJExpression();
                
                //s++; //'('
                //s = skipToNextChar(s, debugInfo);
                recursivelySerializeExpression(varNameStartS, endptr, statement->expression, parent, tokenMap, debugInfo, true);
                
                s = skipToNextChar(*endptr, debugInfo, true);
                if(*s == ')')
                {
                    s++;
                    s = skipToNextChar(s, debugInfo, true);
                }
                
                statements->push_back(statement);
                varNameRef->release();
            }
            else if(varNameRef->allowAsVariableName)
            {
                if(*s == '=')
                {
                    s++;
                    s = skipToNextChar(s, debugInfo);
                    
                    MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_VAR_ASSIGN);
                    statement->lineNumber = debugInfo->lineNumber;
                    statement->varName = varNameRef;
                    
                    
                    statement->expression = new MJExpression();
                    recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, true);
                    
                    s = skipToNextChar(*endptr, debugInfo, true);
                    
                    
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
                    varNameRef->release();
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                    return false;
                }
            }
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
        
        bool success = serializeFunctionBody(s, endptr, parent, &mjFunction->tokenMap, debugInfo, &mjFunction->statements);
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


MJRef* MJFunction::runExpression(MJExpression* expression, uint32_t* tokenIndex, MJRef* result, MJTable* functionState, MJTable* parent, MJTokenMap* tokenMap, std::map<uint32_t, MJRef*>& locals, MJDebugInfo* debugInfo)
{
    uint32_t token = expression->tokens[*tokenIndex];
    if(token == MJ_TOKEN_end)
    {
        return nullptr;
    }
    
    while(token == MJ_TOKEN_pad)
    {
        *tokenIndex = *tokenIndex + 1;
        if(*tokenIndex >= expression->tokens.size())
        {
            return nullptr;
        }
        token = expression->tokens[*tokenIndex];
    }
    if(token < MJ_TOKEN_VAR_START_INDEX)
    {
        switch (token) {
            case MJ_TOKEN_functionCall:
            {
                *tokenIndex = *tokenIndex + 1;
                MJFunction* functionVar = (MJFunction*)runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                *tokenIndex = *tokenIndex + 1;
                
                MJTable* args = nullptr;
                MJRef* arg = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                *tokenIndex = *tokenIndex + 1;
                while(arg)
                {
                    if(!args)
                    {
                        args = new MJTable(functionState);
                    }
                    args->arrayObjects.push_back(arg);
                    arg->retain();
                    arg = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                    *tokenIndex = *tokenIndex + 1;
                }
                
                MJRef* functionResult = ((MJFunction*)functionVar)->call(args, functionState, result);
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
            case MJ_TOKEN_greaterThan:
            case MJ_TOKEN_lessThan:
            case MJ_TOKEN_greaterEqualTo:
            case MJ_TOKEN_lessEqualTo:
            {
                *tokenIndex = *tokenIndex + 1;
                MJRef* leftResult = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case MJREF_TYPE_NUMBER:
                    {
                        double left = ((MJNumber*)leftResult)->value;
                        *tokenIndex = *tokenIndex + 1;
                        static MJNumber rightResult(0);
                        if(runExpression(expression, tokenIndex, &rightResult, functionState, parent, tokenMap, locals, debugInfo))
                        {
                            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                            return nullptr;
                        }
                        
                        if(result && result->type() == MJREF_TYPE_BOOL)
                        {
                            switch (token) {
                                case MJ_TOKEN_greaterThan:
                                    ((MJBool*)result)->value = left > rightResult.value;
                                    break;
                                case MJ_TOKEN_lessThan:
                                    ((MJBool*)result)->value = left < rightResult.value;
                                    break;
                                case MJ_TOKEN_greaterEqualTo:
                                    ((MJBool*)result)->value = left >= rightResult.value;
                                    break;
                                case MJ_TOKEN_lessEqualTo:
                                    ((MJBool*)result)->value = left <= rightResult.value;
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case MJ_TOKEN_greaterThan:
                                    return new MJBool(left > rightResult.value);
                                    break;
                                case MJ_TOKEN_lessThan:
                                    return new MJBool(left < rightResult.value);
                                    break;
                                case MJ_TOKEN_greaterEqualTo:
                                    return new MJBool(left >= rightResult.value);
                                    break;
                                case MJ_TOKEN_lessEqualTo:
                                    return new MJBool(left <= rightResult.value);
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
            case MJ_TOKEN_add:
            case MJ_TOKEN_subtract:
            case MJ_TOKEN_multiply:
            case MJ_TOKEN_divide:
            {
                *tokenIndex = *tokenIndex + 1;
                MJRef* leftResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
                if(!leftResult)
                {
                    leftResult = result;
                }
                
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case MJREF_TYPE_NUMBER:
                    {
                        double left = ((MJNumber*)leftResult)->value;
                        *tokenIndex = *tokenIndex + 1;
                        MJRef* rightResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        if(rightResult->type() != leftType)
                        {
                            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                            return nullptr;
                        }
                        
                        if(result && result->type() == leftType)
                        {
                            switch (token) {
                                case MJ_TOKEN_add:
                                    ((MJNumber*)result)->value += left;
                                    break;
                                case MJ_TOKEN_subtract:
                                    ((MJNumber*)result)->value -= left;
                                    break;
                                case MJ_TOKEN_multiply:
                                    ((MJNumber*)result)->value *= left;
                                    break;
                                case MJ_TOKEN_divide:
                                    ((MJNumber*)result)->value /= left;
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case MJ_TOKEN_add:
                                    return new MJNumber(left + ((MJNumber*)rightResult)->value);
                                    break;
                                case MJ_TOKEN_subtract:
                                    return new MJNumber(left - ((MJNumber*)rightResult)->value);
                                    break;
                                case MJ_TOKEN_multiply:
                                    return new MJNumber(left * ((MJNumber*)rightResult)->value);
                                    break;
                                case MJ_TOKEN_divide:
                                    return new MJNumber(left / ((MJNumber*)rightResult)->value);
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
                
            default:
            {
                MJError("Unimplemented");
            }
                break;
        }
    }
    else
    {
        MJRef* foundValue = nullptr;
        if(locals.count(token) != 0)
        {
            foundValue = locals[token];
        }
        else if(tokenMap->refsByToken.count(token) != 0)
        {
            foundValue = tokenMap->refsByToken[token];
        }
        
        if(foundValue && foundValue->type() != MJREF_TYPE_NIL)
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
            MJError("Bad token");
        }
    }
    
    return nullptr;
}

MJRef* MJFunction::runStatement(MJStatement* statement, MJRef* result, MJTable* functionState, MJTable* parent, MJTokenMap* tokenMap, std::map<uint32_t, MJRef*>& locals, MJDebugInfo* debugInfo)
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
            uint32_t tokenIndex = 0;
            MJRef* existingValue = result;
            MJRef* newResult = runExpression(statement->expression, &tokenIndex, existingValue, functionState, parent, tokenMap, locals, debugInfo);
            if(newResult)
            {
                return newResult;
            }
            if(result)
            {
                return result;
            }
            return new MJRef(parent);
        }
            break;
        case MJSTATEMENT_TYPE_VAR_ASSIGN:
        {
            MJRef* existingValue = nullptr;
            if(locals.count(statement->varToken) != 0)
            {
                existingValue = locals[statement->varToken];
            }
            else if(functionState->objectsByStringKey.count(statement->varName->value) != 0)
            {
                existingValue = functionState->objectsByStringKey[statement->varName->value];
                locals[statement->varToken] = existingValue;
            }
            else
            {
                existingValue = functionState->recursivelyFindVariable(statement->varName, debugInfo, false);
                if(existingValue)
                {
                    locals[statement->varToken] = existingValue;
                }
            }
            
            uint32_t tokenIndex = 0;
            MJRef* result = runExpression(statement->expression, &tokenIndex, existingValue, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                tokenMap->refsByToken[statement->varToken] = result;
                functionState->objectsByStringKey[statement->varName->value] = result;
                result->retain();
                locals[statement->varToken] = result;
                
            }//else it updated the existing value
            
        }
            break;
        case MJSTATEMENT_TYPE_FUNCTION_CALL:
        {
            uint32_t tokenIndex = 0;
            debugInfo->lineNumber = statement->lineNumber; //?
            MJRef* result = runExpression(statement->expression, &tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                result->release();
            }
        }
            break;
            
        case MJSTATEMENT_TYPE_FOR:
        {
            MJForStatement* forStatement = (MJForStatement*)statement;
            
            MJRef* existingValue = nullptr;
            if(locals.count(forStatement->varToken) != 0)
            {
                existingValue = locals[forStatement->varToken];
            }
            else if(functionState->objectsByStringKey.count(forStatement->varName->value) != 0)
            {
                existingValue = functionState->objectsByStringKey[forStatement->varName->value];
                locals[forStatement->varToken] = existingValue;
            }
            else
            {
                existingValue = functionState->recursivelyFindVariable(forStatement->varName, debugInfo, false);
                if(existingValue)
                {
                    locals[forStatement->varToken] = existingValue;
                }
            }
            
            uint32_t tokenIndex = 0;
            MJRef* result = runExpression(forStatement->expression, &tokenIndex, existingValue, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                tokenMap->refsByToken[forStatement->varToken] = result;
                functionState->objectsByStringKey[forStatement->varName->value] = result;
                result->retain();
                locals[forStatement->varToken] = result;
                
            }
            
            
            bool run = true;
            while(run)
            {
                uint32_t tokenIndex = 0;
                debugInfo->lineNumber = statement->lineNumber; //?
                
                MJRef* expressionResult = runExpression(forStatement->continueExpression, &tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo); //todo pass in an MJBool result
                
                bool expressionPass = true;
                if(expressionResult)
                {
                    expressionPass = expressionResult->boolValue();
                }
                
                if(expressionPass)
                {
                    MJRef* runResult = runStatementArray(forStatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                    
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
                    MJRef* runResult = runStatement(forStatement->incrementStatement, result, functionState, parent, tokenMap, locals, debugInfo);
                    if(runResult)
                    {
                        return runResult;
                    }
                }
            }
            
            /*const char* expressionCString = statement->expression.c_str(); //var assign
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
            const std::string& varNameStdString = statement->varName->value;
            
            if(functionState->objectsByStringKey.count(varNameStdString) != 0)
            {
                prevI = functionState->objectsByStringKey[varNameStdString];
            }
            
            functionState->objectsByStringKey[varNameStdString] = iterator;
            
            
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
                        functionState->objectsByStringKey[varNameStdString] = prevI;
                    }
                    else
                    {
                        functionState->objectsByStringKey.erase(varNameStdString);
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
                            functionState->objectsByStringKey[varNameStdString] = prevI;
                        }
                        else
                        {
                            functionState->objectsByStringKey.erase(varNameStdString);
                        }
                        iterator->release();
                        return result;
                    }
                }
                
            }
            
            if(prevI)
            {
                functionState->objectsByStringKey[varNameStdString] = prevI;
            }
            else
            {
                functionState->objectsByStringKey.erase(varNameStdString);
            }
            iterator->release();*/
        }
            break;
        case MJSTATEMENT_TYPE_IF:
        {
            uint32_t tokenIndex = 0;
            MJIfStatement* currentSatement = (MJIfStatement*)statement;
            
            while(currentSatement)
            {
                MJRef* expressionResult = runExpression(currentSatement->expression, &tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                
                bool expressionPass = true;
                if(expressionResult)
                {
                    expressionPass = expressionResult->boolValue();
                }
                
                if(expressionPass)
                {
                    MJRef* runResult = runStatementArray(currentSatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
                    
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
                            MJRef* runResult = runStatementArray(currentSatement->elseIfStatement->statements, result, functionState, parent, tokenMap, locals, debugInfo);
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
            
            /*bool expressionPass = true;
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
            }*/
        }
            break;
        default:
            break;
    }
    return nullptr;
}

    
MJRef* MJFunction::runStatementArray(std::vector<MJStatement*>& statements_,  MJRef* result,  MJTable* functionState, MJTable* parent, MJTokenMap* tokenMap, std::map<uint32_t, MJRef*>& locals, MJDebugInfo* debugInfo) //static
{
    for(MJStatement* statement : statements_)
    {
        MJRef* newResult = MJFunction::runStatement(statement, result, functionState, parent, tokenMap, locals, debugInfo);
        if(newResult)
        {
            return newResult;
        }
    }
    
    return nullptr;
}

    
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

MJRef* MJFunction::call(MJTable* args, MJTable* callLocationState, MJRef* existingResult)
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
        std::map<uint32_t, MJRef*> locals;
        
        int i = 0;
        int maxArgs = (int)argNames.size();
        for(MJRef* arg : args->arrayObjects)
        {
            const std::string& argName = argNames[i];
            if(i >= maxArgs)
            {
                MJSWarn(debugInfo.fileName.c_str(), 0, "Too many arguments supplied to function ignoring:%s", argName.c_str());
                continue;
            }
            currentCallState->objectsByStringKey[argName] = arg;
            arg->retain();
            if(tokenMap.tokensByVarNames.count(argName) != 0)
            {
                locals[tokenMap.tokensByVarNames[argName]] = arg;
            }
            
            i++;
        }
        
        
        MJRef* result = runStatementArray(statements,  existingResult,  currentCallState, (MJTable*)parent, &tokenMap, locals, &debugInfo);
        //MJRef* result = runStatementArray(statements, currentCallState, (MJTable*)parent, &debugInfo); //TODO!!
        //currentCallState->debugLog();
        currentCallState->release();
        
        return result;
    }
}


MJRef* MJFunction::recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, bool searchParents, int varStartIndex)
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
