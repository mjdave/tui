
#ifndef MJRef_h
#define MJRef_h

#include <stdio.h>
#include <string>
#include <set>

#include "glm.hpp"
#include "MJLog.h"
#include "MJFileUtils.h"
#include "MJStringUtils.h"

class MJTable;
class MJString;
class MJRef;

#define MJSError(__fileName__, __lineNumber__, fmt__, ...) MJLog("error %s:%d:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__)
#define MJSWarn(__fileName__, __lineNumber__, fmt__, ...) MJLog("warning %s:%d:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__)

enum {
    MJREF_TYPE_NIL = 0,
    MJREF_TYPE_TABLE,
    MJREF_TYPE_NUMBER,
    MJREF_TYPE_STRING,
    MJREF_TYPE_BOOL,
    MJREF_TYPE_VEC2,
    MJREF_TYPE_VEC3,
    MJREF_TYPE_VEC4,
    MJREF_TYPE_MAT3,
    MJREF_TYPE_MAT4,
    MJREF_TYPE_USERDATA,
    MJREF_TYPE_FUNCTION,
    MJREF_TYPE_EXPRESSION,
};

struct MJDebugInfo {
    std::string fileName;
    int lineNumber = 1;
};

static std::set<char> MJExpressionOperatorsSet = {
    '*',
    '/',
    '+',
    '-',
    '>',
    '<',
    '=',
};

inline const char* skipToNextChar(const char* str, MJDebugInfo* debugInfo, bool stopAtNewLine = false)
{
    const char* s = str;
    bool comment = false;
    for(;; s++)
    {
        if(*s == '\0')
        {
            return s;
        }
        else if(*s == '#' || (*s == '/' && *(s+1) == '/'))
        {
            comment = true;
        }
        else if(*s == '\n')
        {
            comment = false;
            if(stopAtNewLine)
            {
                return s;
            }
            else
            {
                debugInfo->lineNumber++;
            }
        }
        else if(!comment && !isspace(*s))
        {
            return s;
        }
    }
}

inline const char* skipToNextMatchingChar(const char* str, MJDebugInfo* debugInfo, char matchChar)
{
    const char* s = str;
    bool comment = false;
    for(;; s++)
    {
        if(*s == matchChar)
        {
            return s;
        }
        else if(*s == '\0')
        {
            return s;
        }
        else if(*s == '#' || (*s == '/' && *(s+1) == '/'))
        {
            comment = true;
        }
        else if(*s == '\n')
        {
            comment = false;
            debugInfo->lineNumber++;
        }
    }
}


MJRef* loadVariableIfAvailable(MJString* variableName,
                               const char* str,
                               char** endptr,
                               MJTable* parentTable,
                               MJDebugInfo* debugInfo);

bool setVariable(MJString* variableName,
                            MJRef* value,
                               MJTable* parentTable,
                               MJDebugInfo* debugInfo);

MJRef* recursivelyLoadValue(const char* str,
                            char** endptr,
                            MJRef* leftValue,
                            MJTable* parentTable,
                            MJDebugInfo* debugInfo,
                            bool runLowOperators,
                            bool allowNonVarStrings);

class MJRef {
public: //members
    MJRef* parent = nullptr; //this is only stored by tables and functions, variables don't use it currently.
    uint8_t refCount = 1;

public://functions
    MJRef(MJRef* parent_ = nullptr) {parent = parent_;}
    
    virtual ~MJRef() {}
    
    
    void release() {refCount--; if(refCount == 0) { delete this;}}
    void retain() {refCount++;}
    virtual MJRef* copy() {return new MJRef(parent);};
    
    
    static MJTable* createRootTable();
    
    static MJRef* load(const std::string& filename, MJTable* parent = createRootTable()); //public method to read from human readable file/string data
    static MJRef* runScriptFile(const std::string& filename, MJTable* parent = createRootTable());  //public method
    
    static MJRef* load(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo, MJRef** resultRef = nullptr); //public method
    static MJRef* load(const std::string& inputString, const std::string& debugName, MJTable* parent); //public method
    
    //static MJRef* initWithBinaryData(void* data); //todo
    //static MJRef* initWithBinaryFile(const std::string& filePath);  //todo
    static MJRef* initWithHumanReadableString(const std::string& stringData, MJRef* parent) {return new MJRef(parent);} //internal, loads a single value/expression
    //static MJRef* initWithHumanReadableFile(const std::string& filePath);
    
    
    virtual uint8_t type() { return MJREF_TYPE_NIL; }
    virtual std::string getTypeName() {return "nil";}
    
    virtual bool boolValue() {return false;}
    
    std::string getDebugString() {
        std::string debugString;
        printHumanReadableString(debugString);
        return debugString;
    }
    
    virtual void debugLog() {
        MJLog("%s", getDebugString().c_str());
    }
    
    virtual std::string getStringValue() {return "nil";}
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += getStringValue();
    }
    
    void saveToFile(const std::string& filePath) {
        std::string exportString;
        printHumanReadableString(exportString);
        writeToFile(filePath, exportString);
    };
    
    virtual MJRef* recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, int varStartIndex = 0) {return nullptr;} //valid for tables and functions only
    virtual bool recursivelySetVariable(MJString* variableName, MJRef* value, MJDebugInfo* debugInfo, int varStartIndex = 0) {return false;};
    

};

class MJUserData : public MJRef {
public:
    void* value;

public:
    MJUserData(void* value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJUserData() {};
    virtual MJUserData* copy() {return new MJUserData(value, parent);};
    
    virtual uint8_t type() { return MJREF_TYPE_USERDATA; }
    virtual std::string getTypeName() {return "userData";}
    virtual std::string getStringValue() {
        return string_format("%p", value);
    }

private:
    
private:
};

#endif
