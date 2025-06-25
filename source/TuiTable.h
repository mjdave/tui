
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
    
    TuiTable(TuiTable* parent_) : TuiRef(parent_) {};
    virtual ~TuiTable() {
        for(TuiRef* ref : arrayObjects)
        {
            if(ref)
            {
                ref->release();
            }
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
    
    virtual TuiRef* copy() //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        retain();
        return this;
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
                if(resultRef)
                {
                    *resultRef = TUI_NIL;
                }
                s++;
                s = tuiSkipToNextChar(s, debugInfo, true);
                *endptr = (char*)s;
                return false;
            }
            else
            {
                
                TuiRef* valueRef = TuiRef::loadExpression(s,
                                                          endptr,
                                                          nullptr,
                                                          nullptr,
                                                          this,
                                                          debugInfo);
                
                if(resultRef)
                {
                    if(valueRef)
                    {
                        *resultRef = valueRef;
                    }
                    else
                    {
                        *resultRef = TUI_NIL;
                    }
                }
                else
                {
                    valueRef->release();
                }
                
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
            
            TuiStatement* statement = TuiFunction::serializeForStatement(s, endptr, this, &tokenMap, debugInfo, true);
            if(!statement)
            {
                return false;
            }
            s = tuiSkipToNextChar(*endptr, debugInfo, false);
            
            
            // the code below up until calling the statement is very similar to TuiFunction::call()
            // changes made here should probably be made there or it all could be factored out.
            TuiFunctionCallData callData;
            
            for(auto& varNameAndToken : tokenMap.capturedTokensByVarName)
            {
                TuiTable* parentRef = this;
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
            
            
            for(auto& parentDepthAndToken : tokenMap.capturedParentTokensByDepthCount)
            {
                if(callData.locals.count(parentDepthAndToken.first) == 0)
                {
                    TuiTable* parentRef = parent->parent;
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
            
            TuiRef* result = TuiFunction::runStatement(statement,  nullptr, parent, &tokenMap, &callData, debugInfo);
            
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
            
            TuiRef* expressionResult = TuiRef::loadExpression(s,
                                                              endptr,
                                                              nullptr, //existing
                                                              nullptr, //leftValue
                                                              this, //parent
                                                              debugInfo);
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
                            
                            TuiRef* expressionResult = TuiRef::loadExpression(s,
                                                                             endptr,
                                                                             nullptr, //existing
                                                                             nullptr, //leftValue
                                                                             this, //parent
                                                                             debugInfo);
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
        
        TuiRef* enclosingRef = nullptr;
        std::string finalKey = "";
        int finalIndex = -1;
        bool accessedParentVariable = false;
        
        //todo should this be released?
        TuiRef* existingObjectRef = loadValue(s,
                                              endptr,
                                              nullptr,
                                              this,
                                              debugInfo,
                                              true,
                                              &enclosingRef,
                                              &finalKey,
                                              &finalIndex,
                                              &accessedParentVariable);
        
        s = tuiSkipToNextChar(*endptr, debugInfo, true);
        if((*s == '=' && *(s + 1) != '=') || *s == ':')
        {
            s++;
            s = tuiSkipToNextChar(s, debugInfo);
            
            if(accessedParentVariable)
            {
                existingObjectRef = nullptr;
            }
            
            TuiRef* valueRef = TuiRef::loadExpression(s,
                                                      endptr,
                                                      existingObjectRef,
                                                      nullptr,
                                                      this,
                                                      debugInfo);
            
            s = tuiSkipToNextChar(*endptr, debugInfo, true);
            
            // set value if valueRef isn't nil, setting enclosing->finalKey to nil if needed
            if(enclosingRef) //watch out, all this stuff might not be called on a successful copy into the existing keyRef
            {
                if(valueRef && valueRef->type() != Tui_ref_type_NIL)
                {
                    if(!finalKey.empty())
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        ((TuiTable*)enclosingRef)->set(finalKey, valueRef);
                    }
                    else if(finalIndex >= 0)
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        ((TuiTable*)enclosingRef)->replace(finalIndex, valueRef);
                    }
                }
            }
            else if(existingObjectRef)
            {
                if(valueRef && valueRef->type() != Tui_ref_type_NIL)
                {
                    if(!finalKey.empty())
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        ((TuiTable*)enclosingRef)->set(finalKey, valueRef);
                    }
                    else if(finalIndex >= 0)
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        ((TuiTable*)enclosingRef)->replace(finalIndex, valueRef);
                    }
                }
                else
                {
                    if(!finalKey.empty())
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        ((TuiTable*)enclosingRef)->set(finalKey, nullptr);
                    }
                    else if(finalIndex >= 0)
                    {
                        if(enclosingRef->type() != Tui_ref_type_TABLE)
                        {
                            TuiError("Unimplemented");
                        }
                        ((TuiTable*)enclosingRef)->replace(finalIndex, valueRef);
                    }
                }
            }
            else
            {
                TuiError("Something went wrong");
            }
            
            
            if(*s == ',' || *s == '\n')
            {
                if(*s == '\n')
                {
                    debugInfo->lineNumber++;
                }
                s++;
                *endptr = (char*)s;
                return true;
            }
            else if(*s == '}' || *s == ']')
            {
                s++;
                *endptr = (char*)s;
                return false;
            }
            else if(*s != '\0')
            {
               TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected ',' or newline after '=' or ':' assignment. unexpected character loading table:%c", *s);
                return false;
            }
            
            return true;
        }
        else
        {
            bool operatorOr = (*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2));
            bool operatorAnd = (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3));
            
            if(operatorOr || operatorAnd || *s == ',' || *s == '\n' || *s == '}' || *s == ']' || *s == ')' || TuiExpressionOperatorsSet.count(*s) != 0)
            {
                //todo memory problems in here
                TuiRef* leftValue = existingObjectRef;
                
                if(!leftValue && !finalKey.empty())
                {
                    leftValue = new TuiString(finalKey);
                }
                
                TuiRef* valueRef = leftValue;
                if(valueRef)
                {
                    valueRef = TuiRef::loadExpression(s,
                                                      endptr,
                                                      nullptr,
                                                      leftValue,
                                                      this,
                                                      debugInfo);
                    
                    if(!valueRef)
                    {
                        valueRef = leftValue; //dubious
                    }
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                }
                
                if(*s == '\n')
                {
                    debugInfo->lineNumber++;
                }
                
                if(valueRef)
                {
                    arrayObjects.push_back(valueRef);
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
        }
        
        return true;
    }
    
    
    static TuiTable* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr) {
        
        TuiTable* table = new TuiTable(parent);
        
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == '{' || *s == '[' || *s == '(') // is added here to allow function arg lists when this method is called directly, but '(' is restricted in initUnknownTypeRefWithHumanReadableString.
        {
            s++;
        }
        else
        {
            TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "unexpected character loading table:%c", *s);
            table->release();
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
    
    void insert(int insertIndex, TuiRef* value)
    {
        value->retain();
        if(insertIndex < arrayObjects.size())
        {
            arrayObjects.insert(arrayObjects.begin() + insertIndex, value);
        }
        else if(value && value->type() != Tui_ref_type_NIL)
        {
            arrayObjects.resize(insertIndex + 1);
            arrayObjects[insertIndex] = value;
        }
    }
    
    
    void replace(int replaceIndex, TuiRef* value)
    {
        value->retain();
        if(replaceIndex < arrayObjects.size())
        {
            TuiRef* existing = arrayObjects[replaceIndex];
            if(existing)
            {
                existing->release();
            }
            arrayObjects[replaceIndex] = value;
        }
        else if(value && value->type() != Tui_ref_type_NIL)
        {
            arrayObjects.resize(replaceIndex + 1);
            arrayObjects[replaceIndex] = value;
        }
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
        TuiBool* ref = TUI_BOOL(value);
        set(key, ref);
    }
    
    void setFunction(const std::string& key, std::function<TuiRef*(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> value)
    {
        TuiFunction* ref = new TuiFunction(value, this);
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
