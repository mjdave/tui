
#ifndef TuiRef_h
#define TuiRef_h

#include <stdio.h>
#include <string>
#include <set>

#include "glm.hpp"
#include "TuiLog.h"
#include "TuiFileUtils.h"
#include "TuiStringUtils.h"

class TuiTable;
class TuiString;
class TuiRef;

#define TuiParseError(__fileName__, __lineNumber__, fmt__, ...) TuiLog("error %s:%d:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__)
#define TuiParseWarn(__fileName__, __lineNumber__, fmt__, ...) TuiLog("warning %s:%d:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__)

enum {
    Tui_ref_type_NIL = 0,
    Tui_ref_type_TABLE,
    Tui_ref_type_NUMBER,
    Tui_ref_type_STRING,
    Tui_ref_type_BOOL,
    Tui_ref_type_VEC2,
    Tui_ref_type_VEC3,
    Tui_ref_type_VEC4,
    Tui_ref_type_MAT3,
    Tui_ref_type_MAT4,
    Tui_ref_type_USERDATA,
    Tui_ref_type_FUNCTION,
    Tui_ref_type_EXPRESSION,
};

struct TuiDebugInfo {
    std::string fileName;
    int lineNumber = 1;
};

static std::set<char> TuiExpressionOperatorsSet = {
    '*',
    '/',
    '+',
    '-',
    '>',
    '<',
    '=',
};

inline const char* skipToNextChar(const char* str, TuiDebugInfo* debugInfo, bool stopAtNewLine = false)
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

inline const char* skipToNextMatchingChar(const char* str, TuiDebugInfo* debugInfo, char matchChar)
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


TuiRef* loadVariableIfAvailable(TuiString* variableName,
                               TuiRef* existingValue,
                               const char* str,
                               char** endptr,
                               TuiTable* parentTable,
                               TuiDebugInfo* debugInfo);

bool setVariable(TuiString* variableName,
                            TuiRef* value,
                               TuiTable* parentTable,
                               TuiDebugInfo* debugInfo);

TuiRef* recursivelyLoadValue(const char* str,
                            char** endptr,
                            TuiRef* existingValue,
                            TuiRef* leftValue,
                            TuiTable* parentTable,
                            TuiDebugInfo* debugInfo,
                            bool runLowOperators,
                            bool allowNonVarStrings);

class TuiRef {
public: //members
    TuiRef* parent = nullptr; //this is only stored by tables and functions, variables don't use it currently.
    uint8_t refCount = 1;

public://functions
    TuiRef(TuiRef* parent_ = nullptr) {parent = parent_;}
    
    virtual ~TuiRef() {}
    
    
    void release() {refCount--; if(refCount == 0) { delete this;}}
    void retain() {refCount++;}
    virtual TuiRef* copy() {return new TuiRef(parent);};
    virtual void assign(TuiRef* other) {};
    
    
    static TuiTable* createRootTable();
    
    static TuiRef* load(const std::string& filename, TuiTable* parent = createRootTable()); //public method to read from human readable file/string data. supply nullptr as last argument to prvent root table from loading
    static TuiRef* runScriptFile(const std::string& filename, TuiTable* parent = createRootTable());  //public method
    
    static TuiRef* load(const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr); //public method
    static TuiRef* load(const std::string& inputString, const std::string& debugName, TuiTable* parent); //public method
    
    //static TuiRef* initWithBinaryData(void* data); //todo
    //static TuiRef* initWithBinaryFile(const std::string& filePath);  //todo
    static TuiRef* initWithHumanReadableString(const std::string& stringData, TuiRef* parent) {return new TuiRef(parent);} //internal, loads a single value/expression
    //static TuiRef* initWithHumanReadableFile(const std::string& filePath);
    
    
    virtual uint8_t type() { return Tui_ref_type_NIL; }
    virtual std::string getTypeName() {return "nil";}
    
    virtual bool boolValue() {return false;}
    
    std::string getDebugString() {
        std::string debugString;
        printHumanReadableString(debugString);
        return debugString;
    }
    
    virtual void debugLog() {
        TuiLog("%s", getDebugString().c_str());
    }
    
    virtual std::string getStringValue() {return "nil";}
    virtual std::string getDebugStringValue() {return getStringValue() ;}
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += getStringValue();
    }
    
    void saveToFile(const std::string& filePath) {
        std::string exportString;
        printHumanReadableString(exportString);
        writeToFile(filePath, exportString);
    };
    
    virtual TuiRef* recursivelyFindVariable(TuiString* variableName, TuiDebugInfo* debugInfo, bool searchParents, int varStartIndex = 0) {return nullptr;} //valid for tables and functions only
    virtual bool recursivelySetVariable(TuiString* variableName, TuiRef* value, TuiDebugInfo* debugInfo, int varStartIndex = 0) {return false;};
    

};

class TuiUserData : public TuiRef {
public:
    void* value;

public:
    TuiUserData(void* value_, TuiRef* parent_ = nullptr) : TuiRef(parent_) {value = value_;}
    virtual ~TuiUserData() {};
    virtual TuiUserData* copy() {return new TuiUserData(value, parent);};
    
    virtual uint8_t type() { return Tui_ref_type_USERDATA; }
    virtual std::string getTypeName() {return "userData";}
    virtual std::string getStringValue() {
        return string_format("%p", value);
    }

private:
    
private:
};

#endif
