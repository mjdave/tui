
#ifndef TuiRef_h
#define TuiRef_h

#include <stdio.h>
#include <string>
#include <set>
#include <map>

#include "glm.hpp"
#include "TuiLog.h"
#include "TuiFileUtils.h"
#include "TuiStringUtils.h"
#include "TuiStatement.h"

class TuiTable;
class TuiString;
class TuiRef;
class TuiBool;

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

enum {
    Tui_operator_level_default = 0,
    Tui_operator_level_and_or,
    Tui_operator_level_comparison,
    Tui_operator_level_addition_subtraction,
    Tui_operator_level_multiply_divide,
    Tui_operator_level_not
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
    '!',
};

static std::map<char, int> TuiExpressionOperatorsToLevelMap = {
    {'*', Tui_operator_level_multiply_divide},
    {'/', Tui_operator_level_multiply_divide},
    {'+', Tui_operator_level_addition_subtraction},
    {'-', Tui_operator_level_addition_subtraction},
    {'>', Tui_operator_level_comparison},
    {'<', Tui_operator_level_comparison},
    {'=', Tui_operator_level_comparison},
    {'!', Tui_operator_level_not},
};

inline const char* tuiSkipToNextChar(const char* str, TuiDebugInfo* debugInfo = nullptr, bool stopAtNewLine = false)
{
    const char* s = str;
    bool lineComment = false;
    bool blockComment = false;
    for(;; s++)
    {
        if(*s == '\0')
        {
            return s;
        }
        else if(blockComment)
        {
            if(*s == '*' && *(s+1) == '/')
            {
                blockComment = false;
                s++;
            }
        }
        else if(*s == '/' && (*(s+1) == '*' || *(s+1) == '/'))
        {
            if(*(s+1) == '*')
            {
                blockComment = true;
            }
            else
            {
                lineComment = true;
            }
        }
        else if(*s == '#')
        {
            lineComment = true;
        }
        else if(*s == '\n')
        {
            lineComment = false;
            if(stopAtNewLine)
            {
                return s;
            }
            else if(debugInfo)
            {
                debugInfo->lineNumber++;
            }
        }
        else if(!lineComment && !isspace(*s))
        {
            return s;
        }
    }
}

inline const char* tuiSkipToNextMatchingChar(const char* str, TuiDebugInfo* debugInfo, char matchChar)
{
    const char* s = str;
    bool lineComment = false;
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
            lineComment = true;
        }
        else if(*s == '\n')
        {
            lineComment = false;
            debugInfo->lineNumber++;
        }
    }
}

inline bool checkSymbolNameComplete(const char* str)
{
    return *tuiSkipToNextChar(str) != *str;
}

class TuiRef {
    
public: //static functions
    static TuiRef* initUnknownTypeRefWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo);
    static TuiTable* createRootTable(); //adds the built in functions 'print', 'require' etc.
    
    static TuiRef* load(const std::string& filename, TuiTable* parent = createRootTable()); //public method to read from human readable file/string data. supply nullptr as last argument to prvent root table from loading
    static TuiRef* runScriptFile(const std::string& filename, TuiTable* parent = createRootTable(), TuiRef** resultRef = nullptr);  //public method
    
    static TuiRef* load(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr); //public method
    static TuiRef* load(const std::string& inputString, const std::string& debugName, TuiTable* parent); //public method
    
    //static TuiRef* initWithBinaryData(void* data); //todo
    //static TuiRef* initWithBinaryFile(const std::string& filePath);  //todo
    static TuiRef* initWithHumanReadableString(const std::string& stringData, TuiTable* parent) {return new TuiRef(parent);} //internal, loads a single value/expression
    //static TuiRef* initWithHumanReadableFile(const std::string& filePath);
    
    
    
    static TuiRef* loadVariableIfAvailable(TuiString* variableName,
                                           TuiRef* existingValue,
                                           const char* str,
                                           char** endptr,
                                           TuiTable* parentTable,
                                           TuiTokenMap* tokenMap,
                                           std::map<uint32_t,TuiRef*>* locals,
                                           TuiDebugInfo* debugInfo);

    static bool setVariable(TuiString* variableName,
                            TuiRef* value,
                            TuiTable* parentTable,
                            TuiTokenMap* tokenMap,
                            std::map<uint32_t,TuiRef*>* locals,
                            TuiDebugInfo* debugInfo);

    static TuiRef* recursivelyLoadValue(const char* str,
                                        char** endptr,
                                        TuiRef* existingValue,
                                        TuiRef* leftValue,
                                        TuiTable* parentTable,
                                        TuiTokenMap* tokenMap,
                                        std::map<uint32_t,TuiRef*>* locals,
                                        TuiDebugInfo* debugInfo,
                                        int operatorLevel,
                                        bool allowNonVarStrings);
    
    //loadValue calls initUnknownTypeRefWithHumanReadableString, and if it is a string, it calls TuiRef::loadVariableIfAvailable which also will call it if it is a function.
    static TuiRef* loadValue(const char* str,
                             char** endptr,
                             TuiRef* existingValue,
                             TuiTable* parentTable,
                             TuiTokenMap* tokenMap,
                             std::map<uint32_t,TuiRef*>* locals,
                             TuiDebugInfo* debugInfo,
                             bool allowNonVarStrings);
    
    static TuiBool* logicalNot(TuiRef* value);
    
public: //members
    TuiTable* parent = nullptr; //this is only stored by tables and functions, variables don't use it currently.
    uint8_t refCount = 1;

public://functions
    TuiRef(TuiTable* parent_ = nullptr) {parent = parent_;}
    
    virtual ~TuiRef() {}
    
    
    void release() {refCount--; if(refCount == 0) { delete this;}}
    void retain() {refCount++;}
    virtual TuiRef* copy() {return new TuiRef(parent);};
    virtual void assign(TuiRef* other) {};
    virtual bool isEqual(TuiRef* other) {return true;}
    
    
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
        Tui::writeToFile(filePath, exportString);
    };
    

};

class TuiUserData : public TuiRef { //todo TuiUserData is not fully implemented or tested
public:
    void* value;

public:
    TuiUserData(void* value_, TuiTable* parent_ = nullptr) : TuiRef(parent_) {value = value_;}
    virtual ~TuiUserData() {};
    virtual TuiUserData* copy() {return new TuiUserData(value, parent);};
    
    virtual uint8_t type() { return Tui_ref_type_USERDATA; }
    virtual std::string getTypeName() {return "userData";}
    virtual std::string getStringValue() {
        return Tui::string_format("%p", value);
    }

private:
    
private:
};

#endif
