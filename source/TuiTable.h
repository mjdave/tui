
#ifndef TuiTable_h
#define TuiTable_h

#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <fstream>
#include "glm.hpp"
#include "TuiLog.h"
#include "TuiRef.h"
#include "TuiNumber.h"
#include "TuiString.h"
#include "TuiFunction.h"

class TuiTable : public TuiRef {
public:
    std::vector<TuiRef*> arrayObjects; // these members are public for ease/speed of iteration, but it's often best to use the get/set methods instead
    std::map<uint32_t, TuiRef*> objectsByNumberKey;
    std::map<std::string, TuiRef*> objectsByStringKey;
    
private: //members

public://functions
    
    
    virtual uint8_t type() { return Tui_ref_type_TABLE; }
    virtual std::string getTypeName() {return "table";}
    virtual std::string getStringValue() {return "table";}
    virtual std::string getDebugStringValue() {return getDebugString();}
    virtual bool boolValue() {return true;}
    virtual bool isEqual(TuiRef* other) {return other == this;}
    
    TuiTable(TuiRef* parent_) : TuiRef(parent_) {};
    virtual ~TuiTable() {
        for(TuiRef* ref : arrayObjects)
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
    
    virtual TuiTable* copy()
    {
        TuiError("TuiTable copy() unimplemented");
        return this;
    }
    
    
    static TuiRef* initUnknownTypeRefWithHumanReadableString(const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(isdigit(*s) || ((*s == '-' || *s == '+') && isdigit(*(s + 1))))
        {
            return TuiNumber::initWithHumanReadableString(s, endptr, parent, debugInfo);
        }
        
        TuiFunction* functionRef = TuiFunction::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(functionRef)
        {
            return functionRef;
        }
        
        TuiBool* boolRef = TuiBool::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(boolRef)
        {
            return boolRef;
        }
        
        TuiVec2* vec2Ref = TuiVec2::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(vec2Ref)
        {
            return vec2Ref;
        }
        
        TuiVec3* vec3Ref = TuiVec3::initWithHumanReadableString(str, endptr, parent, debugInfo);
        if(vec3Ref)
        {
            return vec3Ref;
        }
        
        TuiVec4* vec4Ref = TuiVec4::initWithHumanReadableString(str, endptr, parent, debugInfo);
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
            return new TuiRef(parent);
        }
        else if(*s == 'n'
                && *(s + 1) == 'u'
                && *(s + 2) == 'l'
                && *(s + 3) == 'l')
        {
            s+=4;
            *endptr = (char*)s;
            return new TuiRef(parent);
        }
        else if(*s == '{' || *s == '[')
        {
            return TuiTable::initWithHumanReadableString(s, endptr, parent, debugInfo);
        }
        
        return TuiString::initWithHumanReadableString(s, endptr, parent, debugInfo);
    }
    
    
    virtual TuiRef* recursivelyFindVariable(TuiString* variableName, TuiDebugInfo* debugInfo, bool searchParents, int varStartIndex = 0)
    {
        std::vector<TuiVarToken>& vars = variableName->vars;
        
        TuiVarToken& varToken = vars[varStartIndex];
        
        switch(varToken.type)
        {
            case Tui_var_token_type_parent:
            {
                if(varStartIndex == vars.size() - 1)
                {
                    return parent;
                }
                return parent->recursivelyFindVariable(variableName, debugInfo, searchParents, varStartIndex + 1);
            }
                break;
            case Tui_var_token_type_string:
            {
                if(objectsByStringKey.count(varToken.varName) != 0)
                {
                    if(varStartIndex == vars.size() - 1)
                    {
                        return objectsByStringKey[varToken.varName];
                    }
                    else
                    {
                        TuiRef* subtableRef = objectsByStringKey[varToken.varName];
                        if(subtableRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected table, but found:%s in %s", subtableRef->getTypeName().c_str(), variableName->value.c_str());
                            return nullptr;
                        }
                        return subtableRef->recursivelyFindVariable(variableName, debugInfo, searchParents, varStartIndex + 1);
                    }
                }
            }
                break;
            case Tui_var_token_type_arrayIndex:
            {
                if(arrayObjects.size() > varToken.arrayOrSetIndex)
                {
                    return arrayObjects[varToken.arrayOrSetIndex];
                }
            }
                break;
            case Tui_var_token_type_setIndex:
            {
                if(objectsByNumberKey.count(varToken.arrayOrSetIndex) != 0)
                {
                    return objectsByNumberKey[varToken.arrayOrSetIndex];
                }
            }
                break;
        }
        
        if(searchParents && parent && varStartIndex == 0)
        {
            return parent->recursivelyFindVariable(variableName, debugInfo, searchParents);
        }
        
        return nullptr;
    }
    
    virtual bool recursivelySetVariable(TuiString* variableName, TuiRef* value, TuiDebugInfo* debugInfo, int varStartIndex = 0)
    {
        std::vector<TuiVarToken>& vars = variableName->vars;
        
        TuiVarToken& varToken = vars[varStartIndex];
        
        switch(varToken.type)
        {
            case Tui_var_token_type_parent:
            {
                if(varStartIndex == vars.size() - 1)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "attempt to set parent table");
                }
                return parent->recursivelySetVariable(variableName, value, debugInfo, varStartIndex + 1);
            }
                break;
            case Tui_var_token_type_string:
            {
                if(varStartIndex == vars.size() - 1)
                {
                    set(varToken.varName, value);
                    return true;
                }
                else if(objectsByStringKey.count(varToken.varName) != 0)
                {
                    TuiRef* subtableRef = objectsByStringKey[varToken.varName];
                    if(subtableRef->type() != Tui_ref_type_TABLE)
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected table, but found:%s in %s", subtableRef->getTypeName().c_str(), variableName->value.c_str());
                        return false;
                    }
                    return ((TuiTable*)subtableRef)->recursivelySetVariable(variableName, value, debugInfo, varStartIndex + 1);
                }
            }
                break;
            case Tui_var_token_type_arrayIndex:
            {
                if(varToken.arrayOrSetIndex < arrayObjects.size())
                {
                    TuiRef* oldValue = arrayObjects[varToken.arrayOrSetIndex];
                    if(oldValue)
                    {
                        if(oldValue == value)
                        {
                            return true;
                        }
                        oldValue->release();
                    }
                    
                    if(value && value->type() != Tui_ref_type_NIL)
                    {
                        value->retain();
                        arrayObjects[varToken.arrayOrSetIndex] = value;
                    }
                    else
                    {
                        arrayObjects[varToken.arrayOrSetIndex] = nullptr;
                    }
                }
                else if(value && value->type() != Tui_ref_type_NIL)
                {
                    arrayObjects.resize(varToken.arrayOrSetIndex + 1);
                    value->retain();
                    arrayObjects[varToken.arrayOrSetIndex] = value;
                }
                return true;
            }
                break;
            case Tui_var_token_type_setIndex:
            {
                TuiError("Unimplimented");
                return false;
            }
                break;
        }
        
        if(parent && varStartIndex == 0)
        {
            return parent->recursivelySetVariable(variableName, value, debugInfo);
        }
        
        return false;
    }
    
    bool addHumanReadableKeyValuePair(const char* str, char** endptr, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(resultRef && *s == 'r'
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
                *resultRef = new TuiRef(this);
                s++;
                s = tuiSkipToNextChar(s, debugInfo, true);
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
                                                  Tui_operator_level_default,
                                                  true);
                s = tuiSkipToNextChar(*endptr, debugInfo, true);
                *endptr = (char*)s;
                return false;
            }
        }
        
        if(*s == 'f' && *(s + 1) == 'o' && *(s + 2) == 'r' && (*(s + 3) == '(' || isspace(*(s + 3))))
        {
            s+=3;
            s = tuiSkipToNextChar(s, debugInfo);
            
            TuiTokenMap tokenMap;
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, this, &tokenMap, debugInfo);
            if(!statement)
            {
                return false;
            }
            s = tuiSkipToNextChar(*endptr, debugInfo, false);
            
            std::map<uint32_t, TuiRef*> locals;
            TuiRef* result = TuiFunction::runStatement(statement, nullptr, this, (TuiTable*)parent, &tokenMap, locals, debugInfo);
            
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
            s = tuiSkipToNextChar(s, debugInfo);
            
            bool expressionPass = true;
            TuiRef* expressionResult = recursivelyLoadValue(s,
                                                 endptr,
                                                           nullptr,
                                                 nullptr,
                                                 this,
                                                 debugInfo,
                                                            Tui_operator_level_default,
                                              true);
            s = tuiSkipToNextChar(*endptr, debugInfo);
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
                s = tuiSkipToNextChar(s, debugInfo);
                if(*s == '{')
                {
                    s++;
                    ifStatementComplete = true;
                    
                    while(1)
                    {
                        s = tuiSkipToNextChar(s, debugInfo);
                        
                        if(*s == '}' || *s == ']' || *s == ')')
                        {
                            s = tuiSkipToNextChar(s + 1, debugInfo);
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
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "if statement expected '{'");
                    return false;
                }
            }
            else
            {
                s = tuiSkipToNextMatchingChar(s, debugInfo, '}');
                s = tuiSkipToNextChar(s + 1, debugInfo);
            }
            
            
            while(1)
            {
                if(*s == 'e' && *(s + 1) == 'l' && *(s + 2) == 's' && *(s + 3) == 'e')
                {
                    if(ifStatementComplete)
                    {
                        s = tuiSkipToNextMatchingChar(s, debugInfo, '}');
                        s = tuiSkipToNextChar(s + 1, debugInfo);
                    }
                    else
                    {
                        s+=4;
                        s = tuiSkipToNextChar(s, debugInfo);
                        if(*s == '{')
                        {
                            s++;
                            ifStatementComplete = true;
                            
                            while(1)
                            {
                                s = tuiSkipToNextChar(s, debugInfo);
                                
                                if(*s == '}' || *s == ']' || *s == ')')
                                {
                                    s = tuiSkipToNextChar(s + 1, debugInfo);
                                    *endptr = (char*)s;
                                    break;
                                }
                                
                                if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                                {
                                    return false;
                                }
                                s = *endptr;
                            }
                            
                            s = tuiSkipToNextChar(*endptr, debugInfo);
                            break;
                        }
                        else if(*s == 'i' && *(s + 1) == 'f')
                        {
                            s+=2;
                            s = tuiSkipToNextChar(s, debugInfo);
                            
                            bool expressionPass = true;
                            TuiRef* expressionResult = recursivelyLoadValue(s,
                                                                           endptr,
                                                                           nullptr,
                                                                           nullptr,
                                                                           this,
                                                                           debugInfo,
                                                                            Tui_operator_level_default,
                                                                           true);
                            s = tuiSkipToNextChar(*endptr, debugInfo);
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
                                s = tuiSkipToNextChar(s, debugInfo);
                                if(*s == '{')
                                {
                                    s++;
                                    ifStatementComplete = true;
                                    
                                    while(1)
                                    {
                                        s = tuiSkipToNextChar(s, debugInfo);
                                        
                                        if(*s == '}' || *s == ']' || *s == ')')
                                        {
                                            s = tuiSkipToNextChar(s + 1, debugInfo);
                                            *endptr = (char*)s;
                                            break;
                                        }
                                        
                                        if(!addHumanReadableKeyValuePair(s, endptr, debugInfo, resultRef))
                                        {
                                            return false;
                                        }
                                        s = *endptr;
                                    }
                                    
                                    s = tuiSkipToNextChar(*endptr, debugInfo);
                                }
                                else
                                {
                                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else if statement expected '{'");
                                    return false;
                                }
                            }
                            else
                            {
                                s = tuiSkipToNextMatchingChar(s, debugInfo, '}');
                                s = tuiSkipToNextChar(s + 1, debugInfo);
                            }
                        }
                        else
                        {
                            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "else statement expected 'if' or '{'");
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
        TuiRef* keyRef = initUnknownTypeRefWithHumanReadableString(s, endptr, this, debugInfo);
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        
        if(*s == ',' || *s == '\n' || *s == '}' || *s == ']' || *s == ')' || *s == '(' || (TuiExpressionOperatorsSet.count(*s) != 0 && (*s != '=' || *(s + 1) == '=')))
        {
            
            /*bool keyWasFunctionCall = false;
            if(keyRef->type() == Tui_ref_type_STRING)
            {
                if(((TuiString*)keyRef)->isValidFunctionString)
                {
                    keyWasFunctionCall = true;
                }
            }*/
            
            debugInfo->lineNumber = keyStartLineNumber;
            TuiRef* newKeyRef = recursivelyLoadValue(keyStartS,
                                                endptr,
                                                    nullptr,
                                                nullptr,
                                                this,
                                                debugInfo,
                                                     Tui_operator_level_default,
                                                true);
            if(newKeyRef)
            {
                keyRef->release();
                keyRef = newKeyRef;
            }
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
            
            if(*s == '\n')
            {
                debugInfo->lineNumber++;
            }
            
            if((newKeyRef && newKeyRef->type() != Tui_ref_type_NIL))
            {
                arrayObjects.push_back(keyRef);
            }
            
            if(*s == '\0')
            {
                *endptr = (char*)s;
                return false;
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
            s = tuiSkipToNextChar(s, debugInfo);
            
            if(keyRef->type() != Tui_ref_type_STRING)
            {
                TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Table keys must be a string or number only. Found:%s", keyRef->getDebugString().c_str());
                return false;
            }
            else if(!((TuiString*)keyRef)->allowAsVariableName)
            {
                ((TuiString*)keyRef)->allowAsVariableName = true; //required hack to support json with "x": quoted keys
                //((TuiString*)keyRef)->varNames.push_back(((TuiString*)keyRef)->value);
                
                ((TuiString*)keyRef)->vars.resize(((TuiString*)keyRef)->vars.size() + 1);
                ((TuiString*)keyRef)->vars[((TuiString*)keyRef)->vars.size() - 1].type = Tui_var_token_type_string;
                ((TuiString*)keyRef)->vars[((TuiString*)keyRef)->vars.size() - 1].varName = ((TuiString*)keyRef)->value;
            }
            
            TuiRef* valueRef = recursivelyLoadValue(s, endptr, nullptr, nullptr, this, debugInfo, Tui_operator_level_default, true);
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
                
            recursivelySetVariable(((TuiString*)keyRef), valueRef, debugInfo);
            
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
                if(valueRef->type() == Tui_ref_type_STRING && *s == '(')
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Attempt to call non-existent function:%s", ((TuiString*)valueRef)->value.c_str());
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected ',' or newline after '=' or ':' assignment. unexpected character loading table:%c", *s);
                }
                success = false;
            }
            
            delete keyRef;
            keyRef = nullptr;
            
            return success;
            
        }
        else if(*s != '\0')
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected character loading table:%c", *s);
            delete keyRef;
            return false;
        }
            
        //}
        
        return true;
    }
    
    
    static TuiTable* initWithHumanReadableString(const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr) {
        
        TuiTable* table = new TuiTable(parent);
        
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == '{' || *s == '[' || *s == '(') // is added here to allow function arg lists when this method is called directly, but '(' is restricted in initUnknownTypeRefWithHumanReadableString.
        {
            s++;
        }
        else
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected character loading table:%c", *s);
            delete table;
            return nullptr;
        }
            
        while(1)
        {
            s = tuiSkipToNextChar(s, debugInfo);
            
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
        
        for(TuiRef* object : arrayObjects)
        {
            for(int i = 0; i < indent; i++)
            {
                debugString += " ";
            }
            if(object)
            {
                object->printHumanReadableString(debugString, indent);
            }
            else
            {
                debugString += "nil";
            }
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
                debugString += Tui::string_format("%d = ", kv.first);
                kv.second->printHumanReadableString(debugString, indent);
                debugString += ",\n";
            }
            else
            {
                TuiWarn("Nil object for key:%d", kv.first);
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
                TuiWarn("Nil object for key:%s", kv.first.c_str());
            }
        }
        
        for(int i = 0; i < indent-4; i++)
        {
            debugString += " ";
        }
        
        debugString += "}";
    }
    
    void set(const std::string& key, TuiRef* value)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* oldValue = objectsByStringKey[key];
            if(oldValue == value)
            {
                return;
            }
            
            oldValue->release();
            
            if(!value || value->type() == Tui_ref_type_NIL)
            {
                objectsByStringKey.erase(key);
            }
        }
        
        if(value && value->type() != Tui_ref_type_NIL)
        {
            value->retain();
            objectsByStringKey[key] = value;
        }
    }
    
    void set(uint32_t key, TuiRef* value)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            TuiRef* oldValue = objectsByNumberKey[key];
            if(oldValue == value)
            {
                return;
            }
            
            oldValue->release();
            
            if(!value || value->type() == Tui_ref_type_NIL)
            {
                objectsByNumberKey.erase(key);
            }
        }
        
        if(value && value->type() != Tui_ref_type_NIL)
        {
            value->retain();
            objectsByNumberKey[key] = value;
        }
    }
    
    void push(TuiRef* value)
    {
        value->retain();
        arrayObjects.push_back(value);
    }
    
    bool hasKey(const std::string& key)
    {
        return objectsByStringKey.count(key) != 0;
    }
    
    TuiRef* get(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            return objectsByStringKey[key];
        }
        return nullptr;
    }
    
    TuiRef* get(const uint32_t numberKey)
    {
        if(objectsByNumberKey.count(numberKey) != 0)
        {
            return objectsByNumberKey[numberKey];
        }
        return nullptr;
    }
    
    TuiRef* getArray(int arrayIndex)
    {
        if(arrayIndex >= 0 && arrayIndex < arrayObjects.size())
        {
            return arrayObjects[arrayIndex];
        }
        return nullptr;
    }
    
    TuiTable* tableAtArrayIndex(int index)
    {
        if(index >= 0 && index < arrayObjects.size())
        {
            TuiRef* ref = arrayObjects[index];
            if(ref->type() == Tui_ref_type_TABLE)
            {
                return ((TuiTable*)ref);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading tableAtArrayIndex expected table at index:%d", ref->getTypeName().c_str(), index);
            }
        }
        return nullptr;
    }
    
    
    TuiTable* getTable(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_TABLE)
            {
                return ((TuiTable*)ref);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected table:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nullptr;
    }
    
    void setTable(const std::string& key, TuiTable* value)
    {
        set(key, value);
    }
    
    const std::string& getString(const std::string& key)
    {
        static const std::string nilString = "";
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_STRING)
            {
                return ((TuiString*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected string:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilString;
    }
    
    void setString(const std::string& key, const std::string& value)
    {
        TuiString* ref = new TuiString(value);
        set(key, ref);
        ref->release();
    }
    
    const dvec2& getVec2(const std::string& key)
    {
        static const dvec2 nilVec2 = dvec2(0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_VEC2)
            {
                return ((TuiVec2*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected vec2:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilVec2;
    }
    
    void setVec2(const std::string& key, const dvec2& value)
    {
        TuiVec2* ref = new TuiVec2(value);
        set(key, ref);
        ref->release();
    }
    
    const dvec3& getVec3(const std::string& key)
    {
        static const dvec3 nilVec3 = dvec3(0.0,0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_VEC3)
            {
                return ((TuiVec3*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected vec3:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilVec3;
    }
    
    void setVec3(const std::string& key, const dvec3& value)
    {
        TuiVec3* ref = new TuiVec3(value);
        set(key, ref);
        ref->release();
    }
    
    const dvec4& getVec4(const std::string& key)
    {
        static const dvec4 nilVec4 = dvec4(0.0,0.0,0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_VEC4)
            {
                return ((TuiVec4*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected vec4:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nilVec4;
    }
    
    void setVec4(const std::string& key, const dvec4& value)
    {
        TuiVec4* ref = new TuiVec4(value);
        set(key, ref);
        ref->release();
    }
    
    double getDouble(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_NUMBER)
            {
                return ((TuiNumber*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected double:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return 0.0;
    }
    
    void setDouble(const std::string& key, double value)
    {
        TuiNumber* ref = new TuiNumber(value);
        set(key, ref);
        ref->release();
    }
    
    bool getBool(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_BOOL)
            {
                return ((TuiBool*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected bool:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return false;
    }
    
    void setBool(const std::string& key, bool value)
    {
        TuiBool* ref = new TuiBool(value);
        set(key, ref);
        ref->release();
    }
    
    
    
    void* getUserData(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            TuiRef* ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_USERDATA)
            {
                return ((TuiUserData*)ref)->value;
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected userData:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nullptr;
    }
    
    void setUserData(const std::string& key, void* value)
    {
        TuiUserData* ref = new TuiUserData(value);
        set(key, ref);
        ref->release();
    }
    

private:
    
private:
};

#endif 
