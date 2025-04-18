#include "MJFunction.h"

#include "MJTable.h"
#include "MJRef.h"

MJFunction::MJFunction(MJTable* parent)
{
    state = new MJTable(parent);
}

MJFunction::~MJFunction()
{
    delete state;
    for(MJStatement* statement : statements)
    {
        delete statement;
    }
}

void serializeExpression(const char* str, char** endptr, MJStatement* statement, MJDebugInfo* debugInfo)
{
    const char* s = str;
    int bracketDepth = 0;
    
    for(;; s++)
    {
        if(*s == '\n')
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
    }
    statement->expression += '\n'; //bit of a hack, expression string parsing needs a comma or newline to mark the end of the string
    
    *endptr = (char*)s;
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
        
        MJFunction* mjFunction = new MJFunction(parent);
        mjFunction->debugInfo.fileName = debugInfo->fileName;
        
        std::string currentVarName = "";
        
        for(;; s++)
        {
            if(*s == ')')
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
        
        if(*s != '{')
        {
            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Function expected opening brace");
            delete mjFunction;
            return nullptr;
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
            if(*s == 'r'
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
                    mjFunction->statements.push_back(statement);
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
                    
                    mjFunction->statements.push_back(statement);
                }
            }
            
            /*MJFunction* functionRef = MJFunction::initWithHumanReadableString(s, endptr, parent, debugInfo);
            if(functionRef)
            {
                MJError("unimplemented")
            }*/
            
            MJString* varNameRef = MJString::initWithHumanReadableString(s, endptr, debugInfo);
            s = skipToNextChar(*endptr, debugInfo);
            
            if(varNameRef)
            {
                if(varNameRef->isValidFunctionString)
                {
                    /*MJTable* argsArrayTable = MJTable::initWithHumanReadableString(s, endptr, parentTable, debugInfo);
                    s = skipToNextChar(*endptr, debugInfo, true);
                    
                    MJRef* result = ((MJFunction*)newValueRef)->call(argsArrayTable, parentTable);
                    
                    *endptr = (char*)s;
                    delete valueRef;
                    return result;*/
                }
                
                if(varNameRef->allowAsVariableName)
                {
                    if(*s == '=')
                    {
                        s++;
                        
                        MJStatement* statement = new MJStatement(MJSTATEMENT_TYPE_VAR_ASSIGN);
                        statement->lineNumber = debugInfo->lineNumber;
                        statement->varName = varNameRef->value;
                        
                        serializeExpression(s, endptr, statement, debugInfo);
                        
                        s = skipToNextChar(*endptr, debugInfo, true);
                        
                        mjFunction->statements.push_back(statement);
                    }
                    else
                    {
                        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "expected '=' after:%s", varNameRef->getDebugString().c_str());
                    }
                }
                delete varNameRef;
            }
            
            //MJSWarn(debugInfo->fileName.c_str(), debugInfo->lineNumber, "valueRef:%s", valueRef->getDebugString().c_str())
        }
        
        s = skipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        return mjFunction;
        
    }
    
    return nullptr;
}



MJRef* MJFunction::call(MJTable* args, MJTable* state)
{
    MJTable* functionState = new MJTable(state);
    
    int i = 0;
    int maxArgs = argNames.size();
    for(MJRef* arg : args->arrayObjects)
    {
        if(i >= maxArgs)
        {
            MJSError(debugInfo.fileName.c_str(), 0, "Too many arguments supplied to function");
        }
        functionState->objectsByStringKey[argNames[i]] = arg;
        i++;
    }
    
    for(MJStatement* statement : statements)
    {
        switch(statement->type)
        {
            case MJSTATEMENT_TYPE_RETURN:
            {
                functionState->release();
                return nullptr;
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
                                     true);
                
                functionState->release();
                return result;
            }
                break;
            case MJSTATEMENT_TYPE_VAR_ASSIGN:
            {
                const char* cString = statement->expression.c_str();
                char* endPtr;
                
                debugInfo.lineNumber = statement->lineNumber;
                
                
                
                MJRef* result = recursivelyLoadValue(cString,
                                                     &endPtr,
                                                     nullptr,
                                                     functionState,
                                                     &debugInfo,
                                                     true);
                
                if(result && result->type() != MJREF_TYPE_NIL)
                {
                    functionState->objectsByStringKey[statement->varName] = result;
                }
                else
                {
                    functionState->objectsByStringKey.erase(statement->varName);
                }
                
            }
                break;
            default:
                break;
        }
    }
    
    
    functionState->release();
    return nullptr;
}
