
#ifndef MJTable_h
#define MJTable_h

#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <fstream>
#include "glm.hpp"
#include "MJLog.h"
#include "MJRef.h"
#include "MJNumber.h"
#include "MJString.h"
#include "MJFunction.h"

class MJTable : public MJRef {
public:
    std::vector<MJRef*> arrayObjects; // these members are public for ease/speed of iteration, but it's often best to use the get/set methods instead
    std::map<uint32_t, MJRef*> objectsByNumberKey;
    std::map<std::string, MJRef*> objectsByStringKey;
    
private: //members

public://functions
    
    
    virtual uint8_t type() { return MJREF_TYPE_TABLE; }
    virtual std::string getTypeName() {return "table";}
    virtual std::string getStringValue() {return "table";}
    virtual std::string getDebugStringValue() {return getDebugString();}
    virtual bool boolValue() {return true;}
    
    MJTable(MJRef* parent_) : MJRef(parent_) {};
    virtual ~MJTable() {
        for(MJRef* ref : arrayObjects)
        {
            ref->release();
        }
        for(auto& kv : objectsByNumberKey)
        {
            kv.second->release();
        }
        for(auto& kv : objectsByStringKey)
        {
            kv.second->release();
        }
    };
    
    virtual MJTable* copy()
    {
        MJError("MJTable copy() unimplemented");
        return this;
    }
    
    
    static MJRef* initUnknownTypeRefWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = skipToNextChar(str, debugInfo);
        
        if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            return MJNumber::initWithHumanReadableString(s, endptr, parent, debugInfo);
        }
        
        MJFunction* functionRef = MJFunction::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(functionRef)
        {
            return functionRef;
        }
        
        MJBool* boolRef = MJBool::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(boolRef)
        {
            return boolRef;
        }
        
        MJVec2* vec2Ref = MJVec2::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(vec2Ref)
        {
            return vec2Ref;
        }
        
        MJVec3* vec3Ref = MJVec3::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(vec3Ref)
        {
            return vec3Ref;
        }
        
        MJVec4* vec4Ref = MJVec4::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(vec4Ref)
        {
            return vec4Ref;
        }
        
        if(*s == 'n'
            && *(s + 1) == 'i'
            && *(s + 2) == 'l')
        {
            s+=3;
            *endptr = (char*)s;
            return new MJRef(parent);
        }
        else if(*s == '{' || *s == '[')
        {
            return MJTable::initWithHumanReadableString(s, endptr, parent, debugInfo);
        }
        
        return MJString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    }
    
    
    virtual MJRef* recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, bool searchParents, int varStartIndex = 0)
    {
        //const std::string variableNameString = variableName->value;
        
        std::vector<std::string>& varNames = variableName->varNames;
        
        if(varNames.size() > varStartIndex)
        {
            if(varNames[varStartIndex][0] == '.')
            {
                MJRef* tableOrFunction = parent;
                
                for(int i = varStartIndex + 1; i < variableName->varNames.size(); i++)
                {
                    if(varNames[i][0] == '.')
                    {
                        tableOrFunction = tableOrFunction->parent;
                        if(!tableOrFunction)
                        {
                            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "No parent found at level:%d for:%s", i + 1, variableName->value.c_str());
                            return nullptr;
                        }
                    }
                    else
                    {
                        return tableOrFunction->recursivelyFindVariable(variableName, debugInfo, searchParents, i);
                    }
                }
                
                return tableOrFunction;
            }
            
            if(objectsByStringKey.count(varNames[varStartIndex]) != 0)
            {
                if(varNames.size() > varStartIndex + 1)
                {
                    MJRef* subtableRef = objectsByStringKey[varNames[varStartIndex]];
                    if(subtableRef->type() != MJREF_TYPE_TABLE)
                    {
                        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected table, but found:%s in %s", subtableRef->getTypeName().c_str(), variableName->value.c_str());
                        return nullptr;
                    }
                    
                    for(int i = varStartIndex + 1; i < variableName->varNames.size(); i++)
                    {
                        if(i == variableName->varNames.size() - 1)
                        {
                            if(((MJTable*)subtableRef)->objectsByStringKey.count(varNames[i]) != 0)
                            {
                                return ((MJTable*)subtableRef)->objectsByStringKey[varNames[i]];
                            }
                            return nullptr;
                        }
                        
                        if(((MJTable*)subtableRef)->objectsByStringKey.count(varNames[i]) != 0)
                        {
                            subtableRef = objectsByStringKey[varNames[i]];
                            if(subtableRef->type() != MJREF_TYPE_TABLE)
                            {
                                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected table, but found:%s in %s", subtableRef->getTypeName().c_str(), variableName->value.c_str());
                                return nullptr;
                            }
                        }
                    }
                }
                
                return objectsByStringKey[variableName->varNames[varStartIndex]];
            }
        }
        
        if(searchParents && parent)
        {
            return parent->recursivelyFindVariable(variableName, debugInfo, searchParents);
        }
        
        return nullptr;
    }
    
    virtual bool recursivelySetVariable(MJString* variableName, MJRef* value, MJDebugInfo* debugInfo, int varStartIndex = 0)
    {
        std::vector<std::string>& varNames = variableName->varNames;
        
        if(varNames.size() > varStartIndex)
        {
            if(varNames[varStartIndex][0] == '.')
            {
                MJRef* tableOrFunction = parent;
                
                for(int i = varStartIndex + 1; i < variableName->varNames.size(); i++)
                {
                    if(varNames[i][0] == '.')
                    {
                        tableOrFunction = tableOrFunction->parent;
                        if(!tableOrFunction)
                        {
                            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "No parent found at level:%d for:%s", (i + 1), variableName->value.c_str());
                            return false;
                        }
                    }
                    else
                    {
                        return tableOrFunction->recursivelySetVariable(variableName, value, debugInfo, i);
                    }
                }
                
                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Bad variable name:%s", variableName->value.c_str());
                return false;
            }
            
            if(objectsByStringKey.count(varNames[varStartIndex]) != 0)
            {
                if(varNames.size() > varStartIndex + 1)
                {
                    MJRef* subtableRef = objectsByStringKey[varNames[varStartIndex]];
                    if(subtableRef->type() != MJREF_TYPE_TABLE)
                    {
                        MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected table, but found:%s in %s", subtableRef->getTypeName().c_str(), variableName->value.c_str());
                        return false;
                    }
                    
                    for(int i = varStartIndex + 1; i < variableName->varNames.size(); i++)
                    {
                        if(i == variableName->varNames.size() - 1)
                        {
                            ((MJTable*)subtableRef)->set(variableName->varNames[i], value);
                            return true;
                        }
                        
                        if(((MJTable*)subtableRef)->objectsByStringKey.count(varNames[i]) != 0)
                        {
                            subtableRef = objectsByStringKey[varNames[i]];
                            if(subtableRef->type() != MJREF_TYPE_TABLE)
                            {
                                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected table, but found:%s in %s", subtableRef->getTypeName().c_str(), variableName->value.c_str());
                                return false;
                            }
                        }
                    }
                }
            }
            
            set(variableName->varNames[varStartIndex], value);
            return true;
        }
        
        if(parent)
        {
            return parent->recursivelySetVariable(variableName, value, debugInfo);
        }
        
        return false;
    }
    
    
    
    
    bool addHumanReadableKeyValuePair(const char* str, char** endptr, MJDebugInfo* debugInfo, MJRef** resultRef = nullptr) {
        const char* s = skipToNextChar(str, debugInfo);
        
        if(resultRef && *s == 'r'
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
                *resultRef = new MJRef(this);
                s++;
                s = skipToNextChar(s, debugInfo, true);
                *endptr = (char*)s;
                return false;
            }
            else
            {
                *resultRef = recursivelyLoadValue(s,
                                                     endptr,
                                                  nullptr,
                                                     nullptr,
                                                     this,
                                                     debugInfo,
                                                     true,
                                                  true);
                s = skipToNextChar(*endptr, debugInfo, true);
                *endptr = (char*)s;
                return false;
            }
        }
        
        if(*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3))))
        {
            s+=3;
            s = skipToNextChar(s, debugInfo);
            
            MJTokenMap tokenMap;
            
            MJStatement* statement = MJFunction::serializeForStatement(s, endptr, parent, &tokenMap, debugInfo);
            if(!statement)
            {
                return false;
            }
            s = skipToNextChar(*endptr, debugInfo, false);
            
            MJRef* result = nullptr; //TODOD!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //MJRef* result = MJFunction::runStatement(statement, this, (MJTable*)parent, &tokenMap, debugInfo);
            
            if(result)
            {
                *resultRef = result;
                *endptr = (char*)s;
                return false;
            }
            *endptr = (char*)s;
            return true;
        }
        
        if(*s == 'i' && *(s + 1) == 'f' && (*(s + 2) == '(' || isspace(*(s + 2))))
        {
            s+=2;
            s = skipToNextChar(s, debugInfo);
            
            bool expressionPass = true;
            MJRef* expressionResult = recursivelyLoadValue(s,
                                                 endptr,
                                                           nullptr,
                                                 nullptr,
                                                 this,
                                                 debugInfo,
                                                 true,
                                              true);
            s = skipToNextChar(*endptr, debugInfo);
            if(expressionResult)
            {
                expressionPass = expressionResult->boolValue();
                expressionResult->release();
            }
            else
            {
                expressionPass = false;
            }
            
            bool ifStatementComplete = false;
            
            if(expressionPass)
            {
                s = skipToNextChar(s, debugInfo);
                if(*s == '{')
                {
                    s++;
                    ifStatementComplete = true;
                    
                    while(1)
                    {
                        s = skipToNextChar(s, debugInfo);
                        
                        if(*s == '}' || *s == ']' || *s == ')')
                        {
                            s = skipToNextChar(s + 1, debugInfo);
                            *endptr = (char*)s;
                            break;
                        }
                        
                        if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                        {
                            //s = *endptr;
                           // break;
                            return false;
                        }
                        s = *endptr;
                    }
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "if statement expected '{'");
                    return false;
                }
            }
            else
            {
                s = skipToNextMatchingChar(s, debugInfo, '}');
                s = skipToNextChar(s + 1, debugInfo);
            }
            
            
            while(1)
            {
                if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
                {
                    if(ifStatementComplete)
                    {
                        s = skipToNextMatchingChar(s, debugInfo, '}');
                        s = skipToNextChar(s + 1, debugInfo);
                    }
                    else
                    {
                        s+=4;
                        s = skipToNextChar(s, debugInfo);
                        if(*s == '{')
                        {
                            s++;
                            ifStatementComplete = true;
                            
                            while(1)
                            {
                                s = skipToNextChar(s, debugInfo);
                                
                                if(*s == '}' || *s == ']' || *s == ')')
                                {
                                    s = skipToNextChar(s + 1, debugInfo);
                                    *endptr = (char*)s;
                                    break;
                                }
                                
                                if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                                {
                                    return false;
                                }
                                s = *endptr;
                            }
                            
                            s = skipToNextChar(*endptr, debugInfo);
                            break;
                        }
                        else if(*s == 'i' && *(s + 1) == 'f')
                        {
                            s+=2;
                            s = skipToNextChar(s, debugInfo);
                            
                            bool expressionPass = true;
                            MJRef* expressionResult = recursivelyLoadValue(s,
                                                                           endptr,
                                                                           nullptr,
                                                                           nullptr,
                                                                           this,
                                                                           debugInfo,
                                                                           true,
                                                                           true);
                            s = skipToNextChar(*endptr, debugInfo);
                            if(expressionResult)
                            {
                                expressionPass = expressionResult->boolValue();
                                expressionResult->release();
                            }
                            else
                            {
                                expressionPass = false;
                            }
                            
                            
                            if(expressionPass)
                            {
                                s = skipToNextChar(s, debugInfo);
                                if(*s == '{')
                                {
                                    s++;
                                    ifStatementComplete = true;
                                    
                                    while(1)
                                    {
                                        s = skipToNextChar(s, debugInfo);
                                        
                                        if(*s == '}' || *s == ']' || *s == ')')
                                        {
                                            s = skipToNextChar(s + 1, debugInfo);
                                            *endptr = (char*)s;
                                            break;
                                        }
                                        
                                        if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                                        {
                                            return false;
                                        }
                                        s = *endptr;
                                    }
                                    
                                    s = skipToNextChar(*endptr, debugInfo);
                                }
                                else
                                {
                                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else if statement expected '{'");
                                    return false;
                                }
                            }
                            else
                            {
                                s = skipToNextMatchingChar(s, debugInfo, '}');
                                s = skipToNextChar(s + 1, debugInfo);
                            }
                        }
                        else
                        {
                            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else statement expected 'if' or '{'");
                            return false;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
            *endptr = (char*)s;
            return true; //return now, we are done
        }
        
        const char* keyStartS = s; // rewind to here if we find an array object
        int keyStartLineNumber = debugInfo->lineNumber;
        MJRef* keyRef = initUnknownTypeRefWithHumanReadableString(s, endptr, this, debugInfo);
        
        s = skipToNextChar(*endptr, debugInfo, true);
        
        if(*s == ',' || *s == '\n' || *s == '}' || *s == ']' || *s == ')' || *s == '(' || (MJExpressionOperatorsSet.count(*s) != 0 && (*s != '=' || *(s + 1) == '=')))
        {
            
            bool keyWasFunctionCall = false;
            if(keyRef->type() == MJREF_TYPE_STRING)
            {
                if(((MJString*)keyRef)->isValidFunctionString)
                {
                    keyWasFunctionCall = true;
                }
                
                /*MJRef* newKeyRef = loadVariableIfAvailable((MJString*)keyRef, s, endptr, this, debugInfo);
                if(newKeyRef)
                {
                    keyRef->release();
                    keyRef = newKeyRef;
                }
                s = skipToNextChar(*endptr, debugInfo, true);*/
            }
            
            debugInfo->lineNumber = keyStartLineNumber;
            MJRef* newKeyRef = recursivelyLoadValue(keyStartS,
                                                endptr,
                                                    nullptr,
                                                nullptr,
                                                this,
                                                debugInfo,
                                                true,
                                                true);
            if(newKeyRef)
            {
                keyRef->release();
                keyRef = newKeyRef;
            }
            s = skipToNextChar(*endptr, debugInfo, true);
            
            
            if(*s == '\n')
            {
                debugInfo->lineNumber++;
            }
            
            if(!keyWasFunctionCall || (newKeyRef && newKeyRef->type() != MJREF_TYPE_NIL))
            {
                arrayObjects.push_back(keyRef);
            }
            
            if(*s == '}' || *s == ']' || *s == ')') // ')' is added here to terminate function arg lists, but '(' is not valid generally.
            {
                s++;
                *endptr = (char*)s;
                return false;
            }
            
            s++;
            *endptr = (char*)s;
            
        }
        else if(*s == '=' || *s == ':')
        {
            s++;
            s = skipToNextChar(s, debugInfo);
            
            if(keyRef->type() != MJREF_TYPE_STRING)
            {
                MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Table keys must be a string or number only. Found:%s", keyRef->getDebugString().c_str());
                return false;
            }
            else if(!((MJString*)keyRef)->allowAsVariableName)
            {
                ((MJString*)keyRef)->allowAsVariableName = true; //required hack to support json with "x": quoted keys
                ((MJString*)keyRef)->varNames.push_back(((MJString*)keyRef)->value);
            }
            
            MJRef* valueRef = recursivelyLoadValue(s, endptr, nullptr, nullptr, this, debugInfo, true, true);
            s = skipToNextChar(*endptr, debugInfo, true);
                
            recursivelySetVariable(((MJString*)keyRef), valueRef, debugInfo);
            
            bool success = true;
            
            
            if(*s == ',' || *s == '\n')
            {
                if(*s == '\n')
                {
                    debugInfo->lineNumber++;
                }
                s++;
                *endptr = (char*)s;
            }
            else if(*s == '}' || *s == ']')
            {
                s++;
                *endptr = (char*)s;
                success = false;
            }
            else if(*s != '\0')
            {
                if(valueRef->type() == MJREF_TYPE_STRING && *s == '(')
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Attempt to call non-existent function:%s", ((MJString*)valueRef)->value.c_str());
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected ',' or newline after '=' or ':' assignment. unexpected character loading table:%c", *s);
                }
                success = false;
            }
            
            delete keyRef;
            keyRef = nullptr;
            
            return success;
            
        }
        else if(*s != '\0')
        {
            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected character loading table:%c", *s);
            delete keyRef;
            return false;
        }
            
        //}
        
        return true;
    }
    
    
    static MJTable* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo, MJRef** resultRef = nullptr) {
        
        MJTable* table = new MJTable(parent);
        
        const char* s = skipToNextChar(str, debugInfo);
        
        if(*s == '{' || *s == '[' || *s == '(') // is added here to allow function arg lists when this method is called directly, but '(' is restricted in initUnknownTypeRefWithHumanReadableString.
        {
            s++;
        }
        else
        {
            MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected character loading table:%c", *s);
            delete table;
            return nullptr;
        }
            
        while(1)
        {
            s = skipToNextChar(s, debugInfo);
            
            if(*s == '\0')
            {
                break;
            }
            
            if(*s == '}' || *s == ']' || *s == ')')
            {
                s++;
                *endptr = (char*)s;
                break;
            }
            
            if(!table->addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
            {
                break;
            }
            s = *endptr;
        }
        
        
        return table;
    }
    
    
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += "{\n";
        indent = indent + 4;
        
        for(MJRef* object : arrayObjects)
        {
            for(int i = 0; i < indent; i++)
            {
                debugString += " ";
            }
            object->printHumanReadableString(debugString, indent);
            debugString += ",\n";
        }
        
        for(auto& kv : objectsByNumberKey)
        {
            if(kv.second)
            {
                for(int i = 0; i < indent; i++)
                {
                    debugString += " ";
                }
                debugString += string_format("%d = ", kv.first);
                kv.second->printHumanReadableString(debugString, indent);
                debugString += ",\n";
            }
            else
            {
                MJWarn("Nil object for key:%d", kv.first);
            }
        }
        
        for(auto& kv : objectsByStringKey)
        {
            if(kv.second)
            {
                for(int i = 0; i < indent; i++)
                {
                    debugString += " ";
                }
                debugString += "\"" + kv.first + "\" = "; //todo escape things correctly?
                kv.second->printHumanReadableString(debugString, indent);
                debugString += ",\n";
            }
            else
            {
                MJWarn("Nil object for key:%s", kv.first.c_str());
            }
        }
        
        for(int i = 0; i < indent-4; i++)
        {
            debugString += " ";
        }
        
        debugString += "}";
    }
    
    void set(const std::string& key, MJRef* value)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* oldValue = objectsByStringKey[key];
            if(oldValue == value)
            {
                return;
            }
            
            oldValue->release();
            
            if(!value || value->type() == MJREF_TYPE_NIL)
            {
                objectsByStringKey.erase(key);
            }
        }
        
        if(value && value->type() != MJREF_TYPE_NIL)
        {
            value->retain();
            objectsByStringKey[key] = value;
        }
    }
    
    void set(uint32_t key, MJRef* value)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            MJRef* oldValue = objectsByNumberKey[key];
            if(oldValue == value)
            {
                return;
            }
            
            oldValue->release();
            
            if(!value || value->type() == MJREF_TYPE_NIL)
            {
                objectsByNumberKey.erase(key);
            }
        }
        
        if(value && value->type() != MJREF_TYPE_NIL)
        {
            value->retain();
            objectsByNumberKey[key] = value;
        }
    }
    
    void push(MJRef* value)
    {
        value->retain();
        arrayObjects.push_back(value);
    }
    
    bool hasKey(const std::string& key)
    {
        return objectsByStringKey.count(key) != 0;
    }
    
    MJRef* get(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            return objectsByStringKey[key];
        }
        return nullptr;
    }
    
    MJRef* get(const uint32_t numberKey)
    {
        if(objectsByNumberKey.count(numberKey) != 0)
        {
            return objectsByNumberKey[numberKey];
        }
        return nullptr;
    }
    
    MJRef* getArray(int arrayIndex)
    {
        if(arrayIndex >= 0 && arrayIndex < arrayObjects.size())
        {
            return arrayObjects[arrayIndex];
        }
        return nullptr;
    }
    
    MJTable* tableAtArrayIndex(int index)
    {
        if(index >= 0 && index < arrayObjects.size())
        {
            MJRef* ref = arrayObjects[index];
            if(ref->type() == MJREF_TYPE_TABLE)
            {
                return ((MJTable*)ref);
            }
            else
            {
                MJError("Found incorrect type (%s) when loading tableAtArrayIndex expected table at index:%d", ref->getTypeName().c_str(), index);
            }
        }
        return nullptr;
    }
    
    
    MJTable* getTable(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_TABLE)
            {
                return ((MJTable*)ref);
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected table:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nullptr;
    }
    
    void setTable(const std::string& key, MJTable* value)
    {
        set(key, value);
    }
    
    const std::string& getString(const std::string& key)
    {
        static const std::string nilString = "";
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_STRING)
            {
                return ((MJString*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected string:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilString;
    }
    
    void setString(const std::string& key, const std::string& value)
    {
        MJString* ref = new MJString(value);
        set(key, ref);
        ref->release();
    }
    
    const dvec2& getVec2(const std::string& key)
    {
        static const dvec2 nilVec2 = dvec2(0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_VEC2)
            {
                return ((MJVec2*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected vec2:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilVec2;
    }
    
    void setVec2(const std::string& key, const dvec2& value)
    {
        MJVec2* ref = new MJVec2(value);
        set(key, ref);
        ref->release();
    }
    
    const dvec3& getVec3(const std::string& key)
    {
        static const dvec3 nilVec3 = dvec3(0.0,0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_VEC3)
            {
                return ((MJVec3*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected vec3:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilVec3;
    }
    
    void setVec3(const std::string& key, const dvec3& value)
    {
        MJVec3* ref = new MJVec3(value);
        set(key, ref);
        ref->release();
    }
    
    const dvec4& getVec4(const std::string& key)
    {
        static const dvec4 nilVec4 = dvec4(0.0,0.0,0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_VEC4)
            {
                return ((MJVec4*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected vec4:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilVec4;
    }
    
    void setVec4(const std::string& key, const dvec4& value)
    {
        MJVec4* ref = new MJVec4(value);
        set(key, ref);
        ref->release();
    }
    
    double getDouble(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_NUMBER)
            {
                return ((MJNumber*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected double:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return 0.0;
    }
    
    void setDouble(const std::string& key, double value)
    {
        MJNumber* ref = new MJNumber(value);
        set(key, ref);
        ref->release();
    }
    
    bool getBool(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_BOOL)
            {
                return ((MJBool*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected bool:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return false;
    }
    
    void setBool(const std::string& key, bool value)
    {
        MJBool* ref = new MJBool(value);
        set(key, ref);
        ref->release();
    }
    
    
    
    void* getUserData(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            MJRef* ref = objectsByStringKey[key];
            if(ref->type() == MJREF_TYPE_USERDATA)
            {
                return ((MJUserData*)ref)->value;
            }
            else
            {
                MJError("Found incorrect type (%s) when loading expected userData:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nullptr;
    }
    
    void setUserData(const std::string& key, void* value)
    {
        MJUserData* ref = new MJUserData(value);
        set(key, ref);
        ref->release();
    }
    

private:
    
private:
};

#endif 
