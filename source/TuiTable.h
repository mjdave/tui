
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
    TuiTable* parentTable = nullptr;
    
    std::vector<TuiRef*> arrayObjects; // these members are public for ease/speed of iteration, but it's often best to use the get/set methods instead
    std::map<uint32_t, TuiRef*> objectsByNumberKey;
    std::map<std::string, TuiRef*> objectsByStringKey;
    
    std::function<void(TuiRef* table, const std::string& key, TuiRef* value)> onSet;
    
private: //members

public://functions
    
    TuiTable(TuiTable* parentTable_ = nullptr) : TuiRef() {parentTable = parentTable_;}
    TuiTable(const std::string& tableString, TuiTable* parentTable_ = nullptr) : TuiRef() {
        TuiDebugInfo debugInfo;
        debugInfo.fileName = "table string constructor";
        const char* cString = tableString.c_str();
        char* endPtr;
        
        TuiTable::initWithHumanReadableString(cString, &endPtr, parentTable_, &debugInfo, nullptr, this);
    }
    
    static TuiTable* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr, TuiTable* inTable = nullptr);
    
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
    
    
    virtual uint8_t type() { return Tui_ref_type_TABLE; }
    virtual std::string getTypeName() {return "table";}
    virtual std::string getStringValue() {return "table";}
    virtual std::string getDebugStringValue() {return getDebugString();}
    virtual bool boolValue() {return true;}
    virtual bool isEqual(TuiRef* other) {return other == this;}
    
    virtual TuiRef* copy() //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        retain();
        return this;
    }

    
    bool addHumanReadableKeyValuePair(const char* str, char** endptr, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr);
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0);
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset);
    
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
        
        if(onSet)
        {
            onSet(this, key, value);
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
    
    void pushString(const std::string& value)
    {
        arrayObjects.push_back(new TuiString(value));
    }
    
    void pushDouble(const double& value)
    {
        arrayObjects.push_back(new TuiNumber(value));
    }
    
    void pushBool(const bool& value)
    {
        arrayObjects.push_back(TUI_BOOL(value));
    }
    
    void pushVec2(const dvec2& value)
    {
        arrayObjects.push_back(new TuiVec2(value));
    }
    
    void pushVec3(const dvec3& value)
    {
        arrayObjects.push_back(new TuiVec3(value));
    }
    
    void pushVec4(const dvec4& value)
    {
        arrayObjects.push_back(new TuiVec4(value));
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
    
    
    bool remove(int index)
    {
        if(index < 0 || index >= arrayObjects.size())
        {
            return false;
        }
        arrayObjects[index]->release();
        arrayObjects.erase(arrayObjects.begin() + index);
        return true;
    }
    
    void replace(int replaceIndex, TuiRef* value) //this is used by table[x] = y. if x <= array.size(), then we will replace the object in the array, otherwise, set an objectByNumberKey value. Generally not a good idea to mix arrays and sets, we just do our best
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
        else if(replaceIndex == arrayObjects.size())
        {
            arrayObjects.push_back(value);
        }
        else
        {
            if(objectsByNumberKey.count(replaceIndex) != 0)
            {
                objectsByNumberKey[replaceIndex]->release();
            }
            
            objectsByNumberKey[replaceIndex] = value;
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
    
    void setFunction(const std::string& key, std::function<TuiRef*(TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> value)
    {
        TuiFunction* ref = new TuiFunction(value);
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
