
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

namespace tui {

    inline tui::table newTable(const std::string& tableString, const tui::table& parentTable_ = nullptr)
    {
        return tui::create<TuiTable>(tableString, parentTable_);
    }

    inline tui::table newTable(const tui::table& parentTable_ = nullptr) //let's call this newTable, beacuse that makes sense, it's empty
    {
        return tui::create<TuiTable>(parentTable_);
    }

}

class TuiTable : public TuiRef {
public:
    tui::table parentTable = nullptr;
    
    std::vector<tui::ref> arrayObjects; // these members are public for ease/speed of iteration, but it's often best to use the get/set methods instead
    std::map<uint32_t, tui::ref> objectsByNumberKey;
    std::map<std::string, tui::ref> objectsByStringKey;
    
    std::set<uint8_t> set8; //these sets are for when you need small and fast. Use table.set8Add(t, 32) and table.set8Remove(t, 32)
    std::set<uint16_t> set16;
    std::set<uint32_t> set32;
    std::set<uint64_t> set64; //note currently all numbers in tui scripts are read as doubles and stored in TuiNumbers, so a very large integer can loose precision
    
    std::function<void(const tui::ref& table, const std::string& key, const tui::ref& value)> onSet;
    
private:
    
    void printSingleSubObject(std::string& debugString, int indent, const tui::ref& object);

public://functions
    
    TuiTable(tui::table parentTable_ = nullptr) : TuiRef() {parentTable = parentTable_;}
    TuiTable(const std::string& tableString, tui::table parentTable_ = nullptr) : TuiRef() {
        TuiDebugInfo debugInfo;
        TuiDebugInfoPush(&debugInfo, "table string construct", 1);
        const char* cString = tableString.c_str();
        char* endPtr;
        
        TuiTable::initWithHumanReadableString(cString, &endPtr, parentTable_, &debugInfo, nullptr, createFromThis<TuiTable>());
    }
    
    static tui::table initWithHumanReadableString(const char* str, char** endptr, const tui::table& parent, TuiDebugInfo* debugInfo, tui::ref* resultRef = nullptr, const tui::table& inTable = nullptr);
    
    virtual ~TuiTable() {}

    
    virtual uint8_t type() override { return Tui_ref_type_TABLE; }
    virtual std::string getTypeName() override {return "table";}
    virtual std::string getStringValue() override {return "table";}
    virtual std::string getDebugStringValue() override {return getDebugString();}
    virtual bool boolValue() override {return true;}
    virtual bool isEqual(const tui::ref& other) override {return other.get() == this;}
    
    virtual tui::ref copy() override //NOTE! This is not a true copy, use trueCopy() below. copy() is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        return createFromThis();
    }
    
    tui::table trueCopy()
    {
        tui::table tableCopy = tui::create<TuiTable>(parentTable);
        tableCopy->arrayObjects = arrayObjects;
        tableCopy->objectsByStringKey = objectsByStringKey;
        tableCopy->objectsByNumberKey = objectsByNumberKey;
        tableCopy->set8 = set8;
        tableCopy->set16 = set16;
        tableCopy->set32 = set32;
        tableCopy->set64 = set64;
        
        for(tui::ref ref : arrayObjects)
        {
            tableCopy->arrayObjects.push_back(ref->copy());
        }
        for(auto& kv : objectsByNumberKey)
        {
            tableCopy->objectsByNumberKey[kv.first] = kv.second->copy();
        }
        for(auto& kv : objectsByStringKey)
        {
            tableCopy->objectsByStringKey[kv.first] = kv.second->copy();
        }
        return tableCopy;
    }

    
    bool addHumanReadableKeyValuePair(const char* str, char** endptr, TuiDebugInfo* debugInfo, tui::ref* resultRef = nullptr);
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) override;
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override;
    
    void set(const std::string& key, const tui::ref& value, bool useCopy = true)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref oldValue = objectsByStringKey[key];
            if(oldValue == value)
            {
                return;
            }
            
            if(!value || value->type() == Tui_ref_type_NIL)
            {
                objectsByStringKey.erase(key);
            }
        }
        
        if(value && value->type() != Tui_ref_type_NIL)
        {
            objectsByStringKey[key] = (useCopy ? value->copy() : value);
        }
        
        if(onSet)
        {
            onSet(createFromThis<TuiTable>(), key, value);
        }
    }
    
    void set(uint32_t key, const tui::ref& value)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref oldValue = objectsByNumberKey[key];
            if(oldValue == value)
            {
                return;
            }
            
            if(!value || value->type() == Tui_ref_type_NIL)
            {
                objectsByNumberKey.erase(key);
            }
        }
        
        if(value && value->type() != Tui_ref_type_NIL)
        {
            objectsByNumberKey[key] = value->copy();
        }
    }
    
    void push(const tui::ref& value)
    {
        arrayObjects.push_back(value->copy());
    }
    
    void pushString(const std::string& value)
    {
        arrayObjects.push_back(tui::create<TuiString>(value));
    }
    
    void pushDouble(const double& value)
    {
        arrayObjects.push_back(tui::create<TuiNumber>(value));
    }
    
    void pushBool(const bool& value)
    {
        arrayObjects.push_back(TUI_BOOL(value));
    }
    
    void pushVec2(const dvec2& value)
    {
        arrayObjects.push_back(tui::create<TuiVec2>(value));
    }
    
    void pushVec3(const dvec3& value)
    {
        arrayObjects.push_back(tui::create<TuiVec3>(value));
    }
    
    void pushVec4(const dvec4& value)
    {
        arrayObjects.push_back(tui::create<TuiVec4>(value));
    }
    
    void pushMat3(const dmat3& value)
    {
        arrayObjects.push_back(tui::create<TuiMat3>(value));
    }
    
    void insert(int insertIndex, const tui::ref& value)
    {
        if(insertIndex < arrayObjects.size())
        {
            arrayObjects.insert(arrayObjects.begin() + insertIndex, value->copy());
        }
        else if(value && value->type() != Tui_ref_type_NIL)
        {
            arrayObjects.resize(insertIndex + 1);
            arrayObjects[insertIndex] = value->copy();
        }
    }
    
    
    bool remove(int index)
    {
        if(index < 0 || index >= arrayObjects.size())
        {
            return false;
        }
        arrayObjects.erase(arrayObjects.begin() + index);
        return true;
    }
    
    void replace(int replaceIndex, const tui::ref& value) //this is used by table[x] = y. if x <= array.size(), then we will replace the object in the array, otherwise, set an objectByNumberKey value. Generally not a good idea to mix arrays and sets, we just do our best
    {
        
        if(replaceIndex < arrayObjects.size())
        {
            tui::ref existing = arrayObjects[replaceIndex];
            arrayObjects[replaceIndex] = value->copy();
        }
        else if(replaceIndex == arrayObjects.size())
        {
            arrayObjects.push_back(value->copy());
        }
        else
        {            
            objectsByNumberKey[replaceIndex] = value->copy();
        }
    }
    
    bool hasKey(const std::string& key)
    {
        return objectsByStringKey.count(key) != 0;
    }
    
    bool hasKey(uint32_t key)
    {
        return objectsByNumberKey.count(key) != 0;
    }
    
    tui::ref get(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            return objectsByStringKey[key];
        }
        return nullptr;
    }
    
    tui::ref get(const uint32_t numberKey)
    {
        if(objectsByNumberKey.count(numberKey) != 0)
        {
            return objectsByNumberKey[numberKey];
        }
        return nullptr;
    }
    
    tui::ref getArray(int arrayIndex)
    {
        if(arrayIndex >= 0 && arrayIndex < arrayObjects.size())
        {
            return arrayObjects[arrayIndex];
        }
        return nullptr;
    }
    
    tui::table tableAtArrayIndex(int index)
    {
        if(index >= 0 && index < arrayObjects.size())
        {
            tui::ref ref = arrayObjects[index];
            if(ref->type() == Tui_ref_type_TABLE)
            {
                return (tui::cast<TuiTable>(ref));
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading tableAtArrayIndex expected table at index:%d", ref->getTypeName().c_str(), index);
            }
        }
        return nullptr;
    }
    
    
    tui::table getTable(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_TABLE)
            {
                return (tui::cast<TuiTable>(ref));
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected table for key:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nullptr;
    }
    
    void setTable(const std::string& key, const tui::table& value)
    {
        set(key, value);
    }
    
    tui::table getTable(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_TABLE)
            {
                return tui::cast<TuiTable>(ref);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected table for index:%d", ref->getTypeName().c_str(), key);
            }
        }
        return nullptr;
    }
    
    void setTable(uint32_t key, const tui::table& value)
    {
        set(key, value);
    }
    
    const std::string& getString(const std::string& key)
    {
        static const std::string nilString = "";
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_STRING)
            {
                return (tui::cast<TuiString>(ref)->value);
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
        tui::string ref = tui::create<TuiString>(value);
        set(key, ref);
    }
    
    const std::string& getString(uint32_t key)
    {
        static const std::string nilString = "";
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_STRING)
            {
                return (tui::cast<TuiString>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected string:%d", ref->getTypeName().c_str(), key);
            }
        }
        return nilString;
    }
    
    void setString(uint32_t key, const std::string& value)
    {
        tui::string ref = tui::create<TuiString>(value);
        set(key, ref);
    }
    
    const dvec2& getVec2(const std::string& key)
    {
        static const dvec2 nilVec2 = dvec2(0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_VEC2)
            {
                return (tui::cast<TuiVec2>(ref)->value);
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
        tui::vec2 ref = tui::create<TuiVec2>(value);
        set(key, ref);
    }
    
    const dvec2& getVec2(uint32_t key)
    {
        static const dvec2 nilVec2 = dvec2(0.0,0.0);
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_VEC2)
            {
                return (tui::cast<TuiVec2>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected vec2:%d", ref->getTypeName().c_str(), key);
            }
        }
        return nilVec2;
    }
    
    void setVec2(uint32_t key, const dvec2& value)
    {
        tui::vec2 ref = tui::create<TuiVec2>(value);
        set(key, ref);
    }
    
    const dvec3& getVec3(const std::string& key)
    {
        static const dvec3 nilVec3 = dvec3(0.0,0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_VEC3)
            {
                return (tui::cast<TuiVec3>(ref)->value);
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
        tui::vec3 ref = tui::create<TuiVec3>(value);
        set(key, ref);
    }
    
    const dvec3& getVec3(uint32_t key)
    {
        static const dvec3 nilVec3 = dvec3(0.0,0.0,0.0);
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_VEC3)
            {
                return (tui::cast<TuiVec3>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected vec3:%d", ref->getTypeName().c_str(), key);
            }
        }
        return nilVec3;
    }
    
    void setVec3(uint32_t key, const dvec3& value)
    {
        tui::vec3 ref = tui::create<TuiVec3>(value);
        set(key, ref);
    }
    
    const dvec4& getVec4(const std::string& key)
    {
        static const dvec4 nilVec4 = dvec4(0.0,0.0,0.0,0.0);
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_VEC4)
            {
                return (tui::cast<TuiVec4>(ref)->value);
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
        tui::vec4 ref = tui::create<TuiVec4>(value);
        set(key, ref);
    }
    
    const dvec4& getVec4(uint32_t key)
    {
        static const dvec4 nilVec4 = dvec4(0.0,0.0,0.0,0.0);
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_VEC4)
            {
                return (tui::cast<TuiVec4>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected vec4:%d", ref->getTypeName().c_str(), key);
            }
        }
        return nilVec4;
    }
    
    void setVec4(uint32_t key, const dvec4& value)
    {
        tui::vec4 ref = tui::create<TuiVec4>(value);
        set(key, ref);
    }
    
    const dmat3& getMat3(const std::string& key)
    {
        static const dmat3 identity = dmat3(1.0);
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_MAT3)
            {
                return (tui::cast<TuiMat3>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected mat3:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return identity;
    }
    
    void setMat3(const std::string& key, const dmat3& value)
    {
        tui::mat3 ref = tui::create<TuiMat3>(value);
        set(key, ref);
    }
    
    const dmat3& getMat3(uint32_t key)
    {
        static const dmat3 identity = dmat3(1.0);
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_MAT3)
            {
                return (tui::cast<TuiMat3>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected mat3:%d", ref->getTypeName().c_str(), key);
            }
        }
        return identity;
    }
    
    void setMat3(uint32_t key, const dmat3& value)
    {
        tui::mat3 ref = tui::create<TuiMat3>(value);
        set(key, ref);
    }
    
    double getDouble(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_NUMBER)
            {
                return (tui::cast<TuiNumber>(ref)->value);
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
        tui::number ref = tui::create<TuiNumber>(value);
        set(key, ref);
    }
    
    double getDouble(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_NUMBER)
            {
                return (tui::cast<TuiNumber>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected double:%d", ref->getTypeName().c_str(), key);
            }
        }
        return 0.0;
    }
    
    void setDouble(uint32_t key, double value)
    {
        tui::number ref = tui::create<TuiNumber>(value);
        set(key, ref);
    }
    
    uint8_t getInt8(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_8)
            {
                return (tui::cast<TuiNumber8>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int8:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return 0;
    }
    
    void setInt8(const std::string& key, uint8_t value)
    {
        tui::number8 ref = tui::create<TuiNumber8>(value);
        set(key, ref);
    }
    
    
    uint8_t getInt8(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_8)
            {
                return (tui::cast<TuiNumber8>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int8:%d", ref->getTypeName().c_str(), key);
            }
        }
        return 0;
    }
    
    void setInt8(uint32_t key, uint8_t value)
    {
        tui::number8 ref = tui::create<TuiNumber8>(value);
        set(key, ref);
    }
    
    uint16_t getInt16(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_16)
            {
                return (tui::cast<TuiNumber16>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int16:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return 0;
    }
    
    
    void setInt16(const std::string& key, uint16_t value)
    {
        tui::number16 ref = tui::create<TuiNumber16>(value);
        set(key, ref);
    }
    
    
    uint16_t getInt16(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_16)
            {
                return (tui::cast<TuiNumber16>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int16:%d", ref->getTypeName().c_str(), key);
            }
        }
        return 0;
    }
    
    void setInt16(uint32_t key, uint16_t value)
    {
        tui::number16 ref = tui::create<TuiNumber16>(value);
        set(key, ref);
    }
    
    uint32_t getInt32(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_32)
            {
                return (tui::cast<TuiNumber32>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int32:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return 0;
    }
    
    void setInt32(const std::string& key, uint32_t value)
    {
        tui::number32 ref = tui::create<TuiNumber32>(value);
        set(key, ref);
    }
    
    
    
    uint32_t getInt32(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_32)
            {
                return (tui::cast<TuiNumber32>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int32:%d", ref->getTypeName().c_str(), key);
            }
        }
        return 0;
    }
    
    void setInt32(uint32_t key, uint32_t value)
    {
        tui::number32 ref = tui::create<TuiNumber32>(value);
        set(key, ref);
    }
    
    uint64_t getInt64(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_64)
            {
                return (tui::cast<TuiNumber64>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int64:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return 0;
    }
    
    void setInt64(const std::string& key, uint64_t value)
    {
        tui::number64 ref = tui::create<TuiNumber64>(value);
        set(key, ref);
    }
    
    
    uint64_t getInt64(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            if(ref->type() == Tui_ref_type_NUMBER_64)
            {
                return (tui::cast<TuiNumber64>(ref)->value);
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected int64:%d", ref->getTypeName().c_str(), key);
            }
        }
        return 0;
    }
    
    void setInt64(uint32_t key, uint64_t value)
    {
        tui::number64 ref = tui::create<TuiNumber64>(value);
        set(key, ref);
    }
    
    
    bool getBool(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            return ref->boolValue();
        }
        return false;
    }
    
    void setBool(const std::string& key, bool value)
    {
        tui::boolean ref = TUI_BOOL(value);
        set(key, ref);
    }
    
    bool getBool(uint32_t key)
    {
        if(objectsByNumberKey.count(key) != 0)
        {
            tui::ref ref = objectsByNumberKey[key];
            return ref->boolValue();
        }
        return false;
    }
    
    void setBool(uint32_t key, bool value)
    {
        tui::boolean ref = TUI_BOOL(value);
        set(key, ref);
    }
    
    tui::function getFunction(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_FUNCTION)
            {
                return (tui::cast<TuiFunction>(ref));
            }
            else
            {
                TuiError("Found incorrect type (%s) when loading expected function:%s", ref->getTypeName().c_str(), key.c_str());
            }
        }
        return nullptr;
    }
    
    void setFunction(const std::string& key, std::function<tui::ref(const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> value)
    {
        tui::function ref = tui::create<TuiFunction>(value);
        set(key, ref);
    }
    
    void setFunction(const std::string& key, const tui::function& value)
    {
        set(key, value);
    }
    
    void* getUserData(const std::string& key)
    {
        if(objectsByStringKey.count(key) != 0)
        {
            tui::ref ref = objectsByStringKey[key];
            if(ref->type() == Tui_ref_type_USERDATA)
            {
                return (tui::cast<TuiUserData>(ref))->value;
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
        tui::userData ref = tui::create<TuiUserData>(value);
        set(key, ref);
    }
    

private:
    
private:
};

#endif 
