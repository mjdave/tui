
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

#define DEBUG_CHECK_FOR_OVER_RELEASE 0

#define TuiParseError(__fileName__, __lineNumber__, fmt__, ...) TuiLog("\nfile:%s:%d\nError:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__); abort();
#define TuiParseWarn(__fileName__, __lineNumber__, fmt__, ...) TuiLog("\nfile:%s:%d\nWarning:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__)

enum {
    Tui_ref_type_UNDEFINED = 0,
    Tui_ref_type_NIL,
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
    Tui_binary_type_UNDEFINED = 0, //reserved as it may accidentally null terminate strings
    Tui_binary_type_NIL,
    Tui_binary_type_TABLE,
    Tui_binary_type_NUMBER,
    Tui_binary_type_STRING,
    Tui_binary_type_BOOL_TRUE,
    Tui_binary_type_BOOL_FALSE,
    Tui_binary_type_VEC2,
    Tui_binary_type_VEC3,
    Tui_binary_type_VEC4,
    Tui_binary_type_MAT3,
    Tui_binary_type_MAT4,
    Tui_binary_type_END_MARKER,
};

enum {
    Tui_operator_level_default = 0,
    Tui_operator_level_and_or,
    Tui_operator_level_comparison,
    Tui_operator_level_addition_subtraction,
    Tui_operator_level_multiply_divide,
    Tui_operator_level_not
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
    '%',
};

static std::map<char, int> TuiExpressionOperatorsToLevelMap = {
    {'*', Tui_operator_level_multiply_divide},
    {'/', Tui_operator_level_multiply_divide},
    {'%', Tui_operator_level_multiply_divide},
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
            else if(debugInfo && *s == '\n')
            {
                debugInfo->lineNumber++;
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

inline void resizeBufferIfNeeded(std::string& buffer, int* currentOffset, int toAddSize)
{
    if(*currentOffset + toAddSize > buffer.size())
    {
        buffer.resize(*currentOffset + toAddSize);
    }
}

class TuiRef {
    
public: //static functions
    //static TuiRef* initUnknownTypeRefWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, uint32_t variableLoadType, TuiTokenMap* tokenMap = nullptr);
    static TuiTable* createRootTable(); //adds the built in functions 'print', 'require' etc.
    
    static TuiRef* loadBinary(const char* str, int* currentOffset, TuiTable* parent = createRootTable()); //public method to read from data previously serialized with serializeBinary()
    static TuiRef* loadBinary(const std::string& inputString, TuiTable* parent = createRootTable()); // convenience public method
    
    static TuiRef* load(const std::string& filename, TuiTable* parent = createRootTable()); //public method to read from human readable file/string data. supply nullptr as last argument to prevent root table from loading
    static TuiRef* runScriptFile(const std::string& filename, TuiTable* parent = createRootTable());  //public method
    
    static TuiRef* load(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, TuiRef** resultRef = nullptr); //public method
    static TuiRef* loadString(const std::string& inputString, const std::string& debugName = "loadString", TuiTable* parent = createRootTable()); //public method
    
    
    static TuiRef* loadExpression(const char* str,
                                  char** endptr,
                                  TuiRef* existingValue,
                                  TuiRef* leftValue,
                                  TuiTable* parentTable,
                                  TuiDebugInfo* debugInfo,
                                  int operatorLevel = Tui_operator_level_default,
                                  bool allowQuotedStringsAsVariableNames = false); //this is a hack to allow quoted strings as variable names for keys only. This is specifically required to load json files, but applies for all table keys
    
    // parses a variable chain and returns the result eg: foo.bar().array[1+2].x
    // optionally stores the enclosing ref and the final variable name if found
    // call directly for table keys, but via loadExpression for values.
    static TuiRef* loadValue(const char* str,
                             char** endptr,
                             TuiRef* existingValue,
                             TuiTable* parentTable,
                             TuiDebugInfo* debugInfo,
                             bool allowQuotedStringsAsVariableNames,//this is a hack to allow quoted strings as variable names for keys only. This is specifically required to load json files, but applies for all table keys
                             
                             //below are only passed if we are setting a key, giving the caller quick access to the parent to set the value for an uninitialized variable
                             TuiRef** onSetIfNilFoundEnclosingRef = nullptr,
                             std::string* onSetIfNilFoundKey = nullptr,
                             int* onSetIfNilFoundIndex = nullptr, //index todo
                             bool* accessedParentVariable = nullptr);
    
    static TuiBool* logicalNot(TuiRef* value);
    
public: //members
#if DEBUG_CHECK_FOR_OVER_RELEASE
    uint32_t refCount = 2;
#else
    uint32_t refCount = 1;
#endif

public://functions
    TuiRef() {}
    
    virtual ~TuiRef() {}
    
    
    virtual void release() {refCount--; if(refCount == 0) {
#if DEBUG_CHECK_FOR_OVER_RELEASE
        TuiError("Over release");
#else
    delete this;
#endif
    }}
    virtual TuiRef* retain() {refCount++;
        if(refCount > 50)
        {
            TuiWarn("object is likely being leaked");
        }
        return this;
    }
    virtual TuiRef* copy() = 0;
    virtual void assign(TuiRef* other) {};
    virtual bool isEqual(TuiRef* other) {
        return (!other || other->type() == Tui_ref_type_NIL);
    }
    
    
    virtual uint8_t type() { return Tui_ref_type_UNDEFINED; }
    virtual std::string getTypeName() {return "undefined";}
    
    virtual bool boolValue() {return false;}
    
    std::string getDebugString() {
        std::string debugString;
        printHumanReadableString(debugString);
        return debugString;
    }
    
    virtual void debugLog() {
        TuiLog("%s", getDebugString().c_str());
    }
    
    virtual std::string getStringValue() {return "undefined";}
    virtual double getNumberValue() {return 0;}
    virtual std::string getDebugStringValue() {return getStringValue() ;}
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += getStringValue();
    }
    
    std::string serializeHumanReadable() {
        std::string exportString;
        printHumanReadableString(exportString);
        return exportString;
    };
    
    void saveToFile(const std::string& filePath) {
        std::string exportString;
        printHumanReadableString(exportString);
        Tui::writeToFile(filePath, exportString);
    };
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) = 0; //buffer may not be set to the correct length
    
    std::string serializeBinary() //use serializeBinaryToBuffer above for speed, this option is convenient but has slow copies
    {
        std::string buffer;
        int length = 0;
        serializeBinaryToBuffer(buffer, &length);
        buffer.resize(length);
        return buffer;
    }

};


class TuiNil : public TuiRef {

public:
    TuiNil() {}
    virtual ~TuiNil() {}
    virtual TuiRef* copy() {return this;}
    virtual void assign(TuiRef* other) {};
    
    virtual void release() {}
    virtual TuiRef* retain() {return this;}
    
    virtual uint8_t type() { return Tui_ref_type_NIL; }
    virtual std::string getTypeName() {return "nil";}
    virtual std::string getStringValue() {return "nil";}
    virtual bool boolValue() {return false;}
    virtual bool isEqual(TuiRef* other) {return (!other || other == this);}
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset)
    {
        resizeBufferIfNeeded(buffer, currentOffset, 1);
        buffer[(*currentOffset)++] = Tui_binary_type_NIL;
    }

private:
    
private:
};

static TuiNil* TUI_NIL = new TuiNil();

class TuiUserData : public TuiRef {
public:
    void* value;

public:
    TuiUserData(void* value_);
    virtual ~TuiUserData() {}
    
    virtual TuiRef* copy() //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        retain();
        return this;
    }
    
    virtual void assign(TuiRef* other) {
        value = ((TuiUserData*)other)->value;
    };
    
    virtual uint8_t type() { return Tui_ref_type_USERDATA; }
    virtual std::string getTypeName() {return "userData";}
    virtual std::string getStringValue() {
        return Tui::string_format("%p", value);
    }
    virtual bool boolValue() {return value != nullptr;}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_USERDATA && ((TuiUserData*)other)->value == value;}
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset)
    {
        TuiError("Userdata objects do not support binary serialization");
    }

private:
    
private:
};

#endif
