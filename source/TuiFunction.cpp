#include "TuiFunction.h"

#include "TuiTable.h"
#include "TuiRef.h"


void TuiFunction::recursivelySerializeExpression(const char* str,
                            char** endptr,
                            TuiExpression* expression,
                            TuiRef* parent,
                            TuiTokenMap* tokenMap,
                            TuiDebugInfo* debugInfo,
                            bool runLowOperators)
{
    const char* s = str;
    
    uint32_t tokenIndex = (uint32_t)expression->tokens.size();
    expression->tokens.push_back(Tui_TOKEN_pad);
    uint32_t leftTokenTypeMarker = Tui_TOKEN_nil;
    
    
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
        TuiRef* leftValue = TuiTable::initUnknownTypeRefWithHumanReadableString(s, endptr, parent, debugInfo);
        s = skipToNextChar(*endptr, debugInfo, true);
        
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
                    
                    TuiRef* newValueRef = parent->recursivelyFindVariable(varString, debugInfo, true);
                    if(newValueRef)
                    {
                        tokenMap->refsByToken[leftVarToken] = newValueRef;
                    }
                }
                
                expression->tokens.push_back(leftVarToken);
                
                
                if(varString->isValidFunctionString && *s =='(')
                {
                    leftTokenTypeMarker = Tui_TOKEN_functionCall;
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
                    
                    expression->tokens.push_back(Tui_TOKEN_end);
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
            if(leftValue->type() != Tui_ref_type_NIL)
            {
                uint32_t leftVarToken = tokenMap->tokenIndex++;
                tokenMap->refsByToken[leftVarToken] = leftValue;
                expression->tokens.push_back(leftVarToken);
            }
            else
            {
                expression->tokens.push_back(Tui_TOKEN_nil);
            }
        }
    }
        
    char operatorChar = *s;
    char secondOperatorChar = *(s + 1);
    
    if(TuiExpressionOperatorsSet.count(operatorChar) == 0 || (operatorChar == '=' && secondOperatorChar != '=') || (!runLowOperators && (operatorChar == '+' || operatorChar == '-')))
    {
        s = skipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        
        if(leftTokenTypeMarker != Tui_TOKEN_nil)
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
            expression->tokens[tokenIndex] = Tui_TOKEN_add;
        }
            break;
        case '-':
        {
            expression->tokens[tokenIndex] = Tui_TOKEN_subtract;
        }
            break;
        case '*':
        {
            expression->tokens[tokenIndex] = Tui_TOKEN_multiply;
        }
            break;
        case '/':
        {
            expression->tokens[tokenIndex] = Tui_TOKEN_divide;
        }
            break;
        case '>':
        {
            if(secondOperatorChar == '=')
            {
                expression->tokens[tokenIndex] = Tui_TOKEN_greaterEqualTo;
            }
            else
            {
                expression->tokens[tokenIndex] = Tui_TOKEN_greaterThan;
            }
        }
        break;
        case '<':
        {
            if(secondOperatorChar == '=')
            {
                expression->tokens[tokenIndex] = Tui_TOKEN_lessEqualTo;
            }
            else
            {
                expression->tokens[tokenIndex] = Tui_TOKEN_lessThan;
            }
        }
        break;
        case '=':
        {
            expression->tokens[tokenIndex] = Tui_TOKEN_equalTo;
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

TuiForStatement* TuiFunction::serializeForStatement(const char* str, char** endptr, TuiRef* parent,  TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo) //entry point is after 'for'
{
    const char* s = str;
    if(*s == '(')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
    }
    
    TuiForStatement* forStatement = new TuiForStatement();
    forStatement->lineNumber = debugInfo->lineNumber;
    forStatement->expression = new TuiExpression();
    forStatement->continueExpression = new TuiExpression();
    
    TuiString* varNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo);
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
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
            varNameRef->release();
            return nullptr;
        }
    }
    else
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected variable assignment in for loop");
        return nullptr;
    }
    
    if(*s == ',' || *s == '\n')
    {
        s++;
        s = skipToNextChar(s, debugInfo);
    }
    else
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or '\n'");
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
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected ',' or '\n'");
        return nullptr;
    }
    
    //todo this is c/p from below. Should be a factored out
    const char* varNameStartS = s; //rewind if we find a function
    varNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    s = skipToNextChar(*endptr, debugInfo);
    
    if(varNameRef)
    {
        if(varNameRef->isValidFunctionString)
        {
            TuiStatement* incrementStatement = new TuiStatement(TuiSTATEMENT_TYPE_FUNCTION_CALL);
            forStatement->incrementStatement = incrementStatement;
            incrementStatement->lineNumber = debugInfo->lineNumber;
            incrementStatement->expression = new TuiExpression();
            
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
                
                TuiStatement* incrementStatement = new TuiStatement(TuiSTATEMENT_TYPE_VAR_ASSIGN);
                forStatement->incrementStatement = incrementStatement;
                incrementStatement->lineNumber = debugInfo->lineNumber;
                incrementStatement->varName = varNameRef;
                
                
                incrementStatement->expression = new TuiExpression();
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
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
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
}

bool TuiFunction::serializeFunctionBody(const char* str, char** endptr, TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, std::vector<TuiStatement*>* statements)
{
    const char* s = str;
    if(*s != '{')
    {
        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Function expected opening brace");
        return false;
    }
    s++;
    s = skipToNextChar(s, debugInfo);
    
    while(1)
    {
        s = skipToNextChar(s, debugInfo);
        if(*s == '}')
        {
            s++;
            break;
        }
        
        if(*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3))))
        {
            s+=3;
            s = skipToNextChar(s, debugInfo);
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, parent, tokenMap, debugInfo);
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
            
            TuiIfStatement* statement = new TuiIfStatement();
            statement->lineNumber = debugInfo->lineNumber;
            statement->expression = new TuiExpression();
            
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
            
            TuiIfStatement* currentStatement = statement;
            while(1)
            {
                if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
                {
                    s+=4;
                    s = skipToNextChar(s, debugInfo);
                    if(*s == '{')
                    {
                        currentStatement->elseIfStatement = new TuiIfStatement();
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
                        
                        currentStatement->elseIfStatement = new TuiIfStatement();
                        currentStatement->elseIfStatement->lineNumber = debugInfo->lineNumber;
                        currentStatement->elseIfStatement->expression = new TuiExpression();
                        
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
            s = skipToNextChar(s, debugInfo);
            if(*s == '}')
            {
                TuiStatement* statement = new TuiStatement(TuiSTATEMENT_TYPE_RETURN);
                statements->push_back(statement);
                s++;
                s = skipToNextChar(s, debugInfo, true);
                break;
            }
            else
            {
                TuiStatement* statement = new TuiStatement(TuiSTATEMENT_TYPE_RETURN_EXPRESSION);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = new TuiExpression();
                
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, true);
                
                s = skipToNextChar(*endptr, debugInfo, true);
                
                statements->push_back(statement);
            }
        }
        
        if(*s == '}')
        {
            s++;
            break;
        }
        
        TuiString* varNameRef = TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo);
        
        if(varNameRef)
        {
            if(varNameRef->isValidFunctionString)
            {
                TuiStatement* statement = new TuiStatement(TuiSTATEMENT_TYPE_FUNCTION_CALL);
                statement->lineNumber = debugInfo->lineNumber;
                statement->expression = new TuiExpression();
                
                //s++; //'('
                //s = skipToNextChar(s, debugInfo);
                recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, true);
                
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
                s = skipToNextChar(*endptr, debugInfo);
                if(*s == '=')
                {
                    s++;
                    s = skipToNextChar(s, debugInfo);
                    
                    TuiStatement* statement = new TuiStatement(TuiSTATEMENT_TYPE_VAR_ASSIGN);
                    statement->lineNumber = debugInfo->lineNumber;
                    statement->varName = varNameRef;
                    
                    
                    statement->expression = new TuiExpression();
                    recursivelySerializeExpression(s, endptr, statement->expression, parent, tokenMap, debugInfo, true);
                    
                    s = skipToNextChar(*endptr, debugInfo);
                    
                    
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
    
    s = skipToNextChar(s, debugInfo, true);
    *endptr = (char*)s;
    
    return true;
}

TuiFunction* TuiFunction::initWithHumanReadableString(const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo) //assumes that '(' is currently in str
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
        
        TuiFunction* mjFunction = new TuiFunction(parent);
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
        
        if(*s == ')')
        {
            s++;
            s = skipToNextChar(s, debugInfo);
        }
        
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


TuiRef* TuiFunction::runExpression(TuiExpression* expression, uint32_t* tokenIndex, TuiRef* result, TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo)
{
    uint32_t token = expression->tokens[*tokenIndex];
    if(token == Tui_TOKEN_end)
    {
        return nullptr;
    }
    
    while(token == Tui_TOKEN_pad)
    {
        *tokenIndex = *tokenIndex + 1;
        if(*tokenIndex >= expression->tokens.size())
        {
            return nullptr;
        }
        token = expression->tokens[*tokenIndex];
    }
    if(token < Tui_TOKEN_VAR_START_INDEX)
    {
        switch (token) {
            case Tui_TOKEN_functionCall:
            {
                *tokenIndex = *tokenIndex + 1;
                TuiFunction* functionVar = (TuiFunction*)runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                *tokenIndex = *tokenIndex + 1;
                
                TuiTable* args = nullptr;
                TuiRef* arg = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                *tokenIndex = *tokenIndex + 1;
                while(arg)
                {
                    if(!args)
                    {
                        args = new TuiTable(functionState);
                    }
                    args->arrayObjects.push_back(arg);
                    arg->retain();
                    arg = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                    *tokenIndex = *tokenIndex + 1;
                }
                
                TuiRef* functionResult = ((TuiFunction*)functionVar)->call(args, functionState, result);
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
            case Tui_TOKEN_greaterThan:
            case Tui_TOKEN_lessThan:
            case Tui_TOKEN_greaterEqualTo:
            case Tui_TOKEN_equalTo:
            case Tui_TOKEN_lessEqualTo:
            {
                *tokenIndex = *tokenIndex + 1;
                TuiRef* leftResult = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        double left = ((TuiNumber*)leftResult)->value;
                        *tokenIndex = *tokenIndex + 1;
                        
                        TuiRef* rightResult = runExpression(expression, tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                        if(rightResult->type() != leftType)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected number");
                            return nullptr;
                        }
                        
                        if(result && result->type() == Tui_ref_type_BOOL)
                        {
                            switch (token) {
                                case Tui_TOKEN_greaterThan:
                                    ((TuiBool*)result)->value = left > ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_TOKEN_lessThan:
                                    ((TuiBool*)result)->value = left < ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_TOKEN_greaterEqualTo:
                                    ((TuiBool*)result)->value = left >= ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_TOKEN_lessEqualTo:
                                    ((TuiBool*)result)->value = left <= ((TuiNumber*)rightResult)->value;
                                    break;
                                case Tui_TOKEN_equalTo:
                                    ((TuiBool*)result)->value = (left == ((TuiNumber*)rightResult)->value);
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case Tui_TOKEN_greaterThan:
                                    return new TuiBool(left > ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_lessThan:
                                    return new TuiBool(left < ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_greaterEqualTo:
                                    return new TuiBool(left >= ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_lessEqualTo:
                                    return new TuiBool(left <= ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_equalTo:
                                    return new TuiBool(left == ((TuiNumber*)rightResult)->value);
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
            case Tui_TOKEN_add:
            case Tui_TOKEN_subtract:
            case Tui_TOKEN_multiply:
            case Tui_TOKEN_divide:
            {
                *tokenIndex = *tokenIndex + 1;
                TuiRef* leftResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
                if(!leftResult)
                {
                    leftResult = result;
                }
                
                uint32_t leftType = leftResult->type();
                switch (leftType) {
                    case Tui_ref_type_NUMBER:
                    {
                        double left = ((TuiNumber*)leftResult)->value;
                        *tokenIndex = *tokenIndex + 1;
                        TuiRef* rightResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
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
                                case Tui_TOKEN_add:
                                    ((TuiNumber*)result)->value += left;
                                    break;
                                case Tui_TOKEN_subtract:
                                    ((TuiNumber*)result)->value -= left;
                                    break;
                                case Tui_TOKEN_multiply:
                                    ((TuiNumber*)result)->value *= left;
                                    break;
                                case Tui_TOKEN_divide:
                                    ((TuiNumber*)result)->value /= left;
                                    break;
                            };
                        }
                        else
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    return new TuiNumber(left + ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_subtract:
                                    return new TuiNumber(left - ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_multiply:
                                    return new TuiNumber(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
                                    return new TuiNumber(left / ((TuiNumber*)rightResult)->value);
                                    break;
                            };
                        }
                        
                    }
                        break;
                    case Tui_ref_type_VEC2:
                    {
                        dvec2 left = ((TuiVec2*)leftResult)->value;
                        *tokenIndex = *tokenIndex + 1;
                        TuiRef* rightResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        uint32_t resultType = rightResult->type();
                        
                        if(result && resultType == leftType)
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    ((TuiVec2*)result)->value += left;
                                    break;
                                case Tui_TOKEN_subtract:
                                    ((TuiVec2*)result)->value -= left;
                                    break;
                                case Tui_TOKEN_multiply:
                                    ((TuiVec2*)result)->value *= left;
                                    break;
                                case Tui_TOKEN_divide:
                                    ((TuiVec2*)result)->value /= left;
                                    break;
                            };
                        }
                        else if(resultType == leftType)
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    return new TuiVec2(left + ((TuiVec2*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_subtract:
                                    return new TuiVec2(left - ((TuiVec2*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_multiply:
                                    return new TuiVec2(left * ((TuiVec2*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
                                    return new TuiVec2(left / ((TuiVec2*)rightResult)->value);
                                    break;
                            };
                        }
                        else if(resultType == Tui_ref_type_NUMBER)
                        {
                            switch (token) {
                                case Tui_TOKEN_multiply:
                                    return new TuiVec2(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
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
                        *tokenIndex = *tokenIndex + 1;
                        TuiRef* rightResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        uint32_t resultType = rightResult->type();
                        
                        if(result && resultType == leftType)
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    ((TuiVec3*)result)->value += left;
                                    break;
                                case Tui_TOKEN_subtract:
                                    ((TuiVec3*)result)->value -= left;
                                    break;
                                case Tui_TOKEN_multiply:
                                    ((TuiVec3*)result)->value *= left;
                                    break;
                                case Tui_TOKEN_divide:
                                    ((TuiVec3*)result)->value /= left;
                                    break;
                            };
                        }
                        else if(resultType == leftType)
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    return new TuiVec3(left + ((TuiVec3*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_subtract:
                                    return new TuiVec3(left - ((TuiVec3*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_multiply:
                                    return new TuiVec3(left * ((TuiVec3*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
                                    return new TuiVec3(left / ((TuiVec3*)rightResult)->value);
                                    break;
                            };
                        }
                        else if(resultType == Tui_ref_type_NUMBER)
                        {
                            switch (token) {
                                case Tui_TOKEN_multiply:
                                    return new TuiVec3(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
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
                        *tokenIndex = *tokenIndex + 1;
                        TuiRef* rightResult = runExpression(expression, tokenIndex, result, functionState, parent, tokenMap, locals, debugInfo);
                        if(!rightResult)
                        {
                            rightResult = result;
                        }
                        
                        uint32_t resultType = rightResult->type();
                        
                        if(result && resultType == leftType)
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    ((TuiVec4*)result)->value += left;
                                    break;
                                case Tui_TOKEN_subtract:
                                    ((TuiVec4*)result)->value -= left;
                                    break;
                                case Tui_TOKEN_multiply:
                                    ((TuiVec4*)result)->value *= left;
                                    break;
                                case Tui_TOKEN_divide:
                                    ((TuiVec4*)result)->value /= left;
                                    break;
                            };
                        }
                        else if(resultType == leftType)
                        {
                            switch (token) {
                                case Tui_TOKEN_add:
                                    return new TuiVec4(left + ((TuiVec4*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_subtract:
                                    return new TuiVec4(left - ((TuiVec4*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_multiply:
                                    return new TuiVec4(left * ((TuiVec4*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
                                    return new TuiVec4(left / ((TuiVec4*)rightResult)->value);
                                    break;
                            };
                        }
                        else if(resultType == Tui_ref_type_NUMBER)
                        {
                            switch (token) {
                                case Tui_TOKEN_multiply:
                                    return new TuiVec4(left * ((TuiNumber*)rightResult)->value);
                                    break;
                                case Tui_TOKEN_divide:
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
        if(locals.count(token) != 0)
        {
            foundValue = locals[token];
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
            TuiError("Bad token");
        }
    }
    
    return nullptr;
}

TuiRef* TuiFunction::runStatement(TuiStatement* statement, TuiRef* result, TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo)
{
    debugInfo->lineNumber = statement->lineNumber;
    switch(statement->type)
    {
        case TuiSTATEMENT_TYPE_RETURN:
        {
            return new TuiRef(parent);
        }
            break;
        case TuiSTATEMENT_TYPE_RETURN_EXPRESSION:
        {
            uint32_t tokenIndex = 0;
            TuiRef* existingValue = result;
            TuiRef* newResult = runExpression(statement->expression, &tokenIndex, existingValue, functionState, parent, tokenMap, locals, debugInfo);
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
        case TuiSTATEMENT_TYPE_VAR_ASSIGN:
        {
            TuiRef* existingValue = nullptr;
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
            TuiRef* result = runExpression(statement->expression, &tokenIndex, existingValue, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                tokenMap->refsByToken[statement->varToken] = result;
                functionState->objectsByStringKey[statement->varName->value] = result;
                result->retain();
                locals[statement->varToken] = result;
                
            }//else it updated the existing value
            
        }
            break;
        case TuiSTATEMENT_TYPE_FUNCTION_CALL:
        {
            uint32_t tokenIndex = 0;
            debugInfo->lineNumber = statement->lineNumber; //?
            TuiRef* result = runExpression(statement->expression, &tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
            if(result)
            {
                result->release();
            }
        }
            break;
            
        case TuiSTATEMENT_TYPE_FOR:
        {
            TuiForStatement* forStatement = (TuiForStatement*)statement;
            
            TuiRef* existingValue = nullptr;
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
            TuiRef* result = runExpression(forStatement->expression, &tokenIndex, existingValue, functionState, parent, tokenMap, locals, debugInfo);
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
                
                TuiRef* expressionResult = runExpression(forStatement->continueExpression, &tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo); //todo pass in an TuiBool result
                
                bool expressionPass = true;
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
            break;
        case TuiSTATEMENT_TYPE_IF:
        {
            uint32_t tokenIndex = 0;
            TuiIfStatement* currentSatement = (TuiIfStatement*)statement;
            
            while(currentSatement)
            {
                tokenIndex = 0;
                TuiRef* expressionResult = runExpression(currentSatement->expression, &tokenIndex, nullptr, functionState, parent, tokenMap, locals, debugInfo);
                
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

    
TuiRef* TuiFunction::runStatementArray(std::vector<TuiStatement*>& statements_,  TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo) //static
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

    
TuiFunction::TuiFunction(TuiRef* parent_)
:TuiRef(parent_)
{
}


TuiFunction::TuiFunction(std::function<TuiRef*(TuiTable* args, TuiTable* state)> func_, TuiRef* parent_)
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

TuiRef* TuiFunction::call(TuiTable* args, TuiTable* callLocationState, TuiRef* existingResult)
{
    if(func)
    {
        TuiTable* currentCallState = new TuiTable(parent);
        TuiRef* result = func(args, currentCallState);
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
            const std::string& argName = argNames[i];
            if(i >= maxArgs)
            {
                TuiParseWarn(debugInfo.fileName.c_str(), 0, "Too many arguments supplied to function ignoring:%s", argName.c_str());
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
        
        
        TuiRef* result = runStatementArray(statements,  existingResult,  currentCallState, (TuiTable*)parent, &tokenMap, locals, &debugInfo);
        //currentCallState->debugLog();
        currentCallState->release();
        
        return result;
    }
}


TuiRef* TuiFunction::recursivelyFindVariable(TuiString* variableName, TuiDebugInfo* debugInfo, bool searchParents, int varStartIndex)
{
    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to get a variable from within a child function");
    return nullptr;
    //return currentCallState->recursivelyFindVariable(variableName, debugInfo, varStartIndex);
}


bool TuiFunction::recursivelySetVariable(TuiString* variableName, TuiRef* value, TuiDebugInfo* debugInfo, int varStartIndex)
{
    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to set variable within a child function");
    return false;
   // return currentCallState->recursivelySetVariable(variableName, value, debugInfo, varStartIndex);
}
