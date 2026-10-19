
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
#include "TuiBuiltInFunctions.h"
#include "TuiPointer.h"

class TuiTable;
class TuiString;
class TuiRef;
class TuiBool;

#define DEBUG_CHECK_FOR_OVER_RELEASE 0 //used internally for debugging tui bugs

//#define TuiParseError(__fileName__, __lineNumber__, fmt__, ...) TuiLog("\nfile:%s:%d\nError:" fmt__, __fileName__, __lineNumber__, ##__VA_ARGS__); abort(); //note this will exit the program due to the call to abort()

inline void TuiPrintDebugBacktrace(TuiDebugInfo* debugInfo)
{
    for(int i = 0; i < debugInfo->lines.size(); i++)
    {
        TuiDebugInfoLine& line = debugInfo->lines[i];
        
        std::string fileName = line.fileName;
        
        if(i == debugInfo->lines.size() - 1)
        {
            fileName = "Error:" + fileName;
        }
        else if(fileName.size() > 64)
        {
            fileName = "from:..." + fileName.substr(fileName.size() - 64);
        }
        else
        {
            fileName = "from:" + fileName;
        }
        
        TuiLog("%s:%d", fileName.c_str(), line.lineNumber);
    }
}

#define TuiParseError(__debugInfo__, fmt__, ...) TuiPrintDebugBacktrace(__debugInfo__); TuiLog("Error:" fmt__, ##__VA_ARGS__); abort(); //note this will exit the program due to the call to abort()
#define TuiParseWarn(__debugInfo__, fmt__, ...) TuiPrintDebugBacktrace(__debugInfo__); TuiLog("Warning:" fmt__, ##__VA_ARGS__)

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
    Tui_ref_type_NUMBER_8,
    Tui_ref_type_NUMBER_16,
    Tui_ref_type_NUMBER_32,
    Tui_ref_type_NUMBER_64
};

enum { // used when serializing to binary, changing any existing values will break backwards compatibility
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
    Tui_binary_type_NUMBER_8,
    Tui_binary_type_NUMBER_16,
    Tui_binary_type_NUMBER_32,
    Tui_binary_type_NUMBER_64,
    Tui_binary_type_NUMBER_8_SET,
    Tui_binary_type_NUMBER_16_SET,
    Tui_binary_type_NUMBER_32_SET,
    Tui_binary_type_NUMBER_64_SET,
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
                debugInfo->currentLine->lineNumber++;
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
                debugInfo->currentLine->lineNumber++;
            }
        }
        else if(!lineComment && !isspace(*s))
        {
            return s;
        }
    }
}

inline const char* tuiSkipToAfterNextClosingBrace(const char* str, TuiDebugInfo* debugInfo, bool openingBraceAlreadyAdded)
{
    const char* s = str - 1; //the while loop below adds 1 but I hate do whiles, so this hack is here
    int depthCount = (openingBraceAlreadyAdded ? 1 : 0);
    while(true)
    {
        s = tuiSkipToNextChar(s + 1, debugInfo);
        if(*s == '}')
        {
            depthCount--;
            if(depthCount == 0)
            {
                s = tuiSkipToNextChar(s + 1, debugInfo);
                return s;
            }
        }
        else if(*s == '{')
        {
            depthCount++;
        }
        else if(*s == '\0')
        {
            return s;
        }
    }
}

inline const char* tuiSkipToNextMatchingChar(const char* str, TuiDebugInfo* debugInfo, char matchChar)
{
    const char* s = str;
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
        else if(*s == '\n')
        {
            debugInfo->currentLine->lineNumber++;
        }
    }
}

inline std::string getVariableNameForDebug(const char* str)
{
    const char* sTmp = str;
    std::string keyString;
    bool done = false;
    while(!done)
    {
        switch (*sTmp) {
            case '\0':
            case ' ':
            case '=':
            case ',':
            case ')':
            case ']':
            case '\n':
            {
                done = true;
                break;
            }
            break;
                
            default:
            {
                if(TuiExpressionOperatorsSet.count(*sTmp) != 0)
                {
                    done = true;
                    break;
                }
                
                keyString += *sTmp;
            }
            break;
        }
        sTmp++;
    }
    return keyString;
}

inline bool checkSymbolNameComplete(const char* str)
{
    if(*str == '\0' || *str == '#' || *str == '\n' || *str == ',' || isspace(*str) || *str == ')' || *str == '}' || *str == ']' || TuiExpressionOperatorsSet.count(*str) != 0)
    {
        return true;
    }
    return *tuiSkipToNextChar(str) != *str;
}

inline void resizeBufferIfNeeded(std::string& buffer, int* currentOffset, int toAddSize)
{
    if(*currentOffset + toAddSize > buffer.size())
    {
        buffer.resize(*currentOffset + toAddSize);
    }
}

class TuiRef : public TuiPointerSource<TuiRef> {
    
public: // public static functions to load tui refs from files and data.
    
    //load from human readable tui code in memory
    static Tui::ref load(const char* str, char** endptr, Tui::table parent, TuiDebugInfo* debugInfo, Tui::ref* resultRef = nullptr); //load human readable
    static Tui::ref loadString(const std::string& inputString, Tui::table parent = Tui::getRootTable(), TuiDebugInfo* callingDebugInfo = nullptr); //convenience method: std::string
    static Tui::ref loadString(const std::string& inputString, const std::string& debugName = "loadString", Tui::table parent = Tui::getRootTable()); //convenience method: alternative
    
    //load from human readable tui code files
    static Tui::ref runScriptFile(const std::string& path, Tui::table parent = Tui::getRootTable(), TuiDebugInfo* callingDebugInfo = nullptr, Tui::ref resultRef = nullptr); // convenience method: as above, but human readable from file. If the file returns a result, it is stored in resultRef.
    
    
    //deserialize from binary serialized tui data in memory
    static Tui::ref loadBinaryString(const char* str, int* currentOffset, Tui::table parent = Tui::getRootTable()); // public method to read from data previously serialized with serializeBinary()
    static Tui::ref loadBinaryString(const std::string& inputString, Tui::table parent = Tui::getRootTable()); // convenience method: as above, but std::string
    
    //deserialize from binary serialized tui data files
    static Tui::ref loadBinary(const std::string& path, Tui::table parent = Tui::getRootTable()); // convenience method: as above, but load from file, calling loadBinaryString internally
    
    static Tui::ref require(const std::string& path, Tui::table rootTable); //convenience method for the built-in require function, which caches the loaded result
    
    // below here are public methods for convenience, however they are generally only useful internally
    
public: // internal static functions
    static Tui::ref loadExpression(const char* str,
                                  char** endptr,
                                  Tui::ref existingValue,
                                  Tui::ref leftValue,
                                  Tui::table parentTable,
                                  TuiDebugInfo* debugInfo,
                                  int operatorLevel = Tui_operator_level_default); //this is a hack to allow quoted strings as variable names for keys only. This is specifically required to load json files, but applies for all table keys
    
    // parses a variable chain and returns the result eg: foo.bar().array[1+2].x
    // optionally stores the enclosing ref and the final variable name if found
    // call directly for table keys, but via loadExpression for values.
    static Tui::ref loadValue(const char* str,
                             char** endptr,
                             Tui::ref existingValue,
                             Tui::table parentTable,
                             TuiDebugInfo* debugInfo,
                             
                             //below are only passed if we are setting a key, giving the caller quick access to the parent to set the value for an uninitialized variable
                             Tui::ref* onSetIfNilFoundEnclosingRef = nullptr,
                             std::string* onSetIfNilFoundKey = nullptr,
                             int* onSetIfNilFoundIndex = nullptr, //index todo
                             bool* accessedParentVariable = nullptr);
    
    static Tui::boolean logicalNot(Tui::ref value);
    
public: //members
#if DEBUG_CHECK_FOR_OVER_RELEASE
    uint32_t refCount = 2;
#else
    uint32_t refCount = 1;
#endif

public://functions
    TuiRef() {}
    virtual ~TuiRef() {}
    
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
    
    
    void saveBinary(const std::string& filePath) {
        std::string exportString;
        int bufferLength = 0;
        serializeBinaryToBuffer(exportString, &bufferLength);
        exportString.resize(bufferLength);
        Tui::writeToFile(filePath, exportString);
    };
    
    virtual Tui::ref copy() = 0;
    virtual void assign(const Tui::ref& other) {}
    virtual bool isEqual(const Tui::ref& other) {
        return (!other || other->type() == Tui_ref_type_NIL);
    }
    
    
    virtual uint8_t type() { return Tui_ref_type_UNDEFINED; }
    virtual std::string getTypeName() {return "undefined";}
    
    virtual bool boolValue() {return false;}
    
    virtual bool isNumber() {
        uint8_t thisType = type();
        return (thisType == Tui_ref_type_NUMBER ||
                thisType == Tui_ref_type_NUMBER_8 ||
                thisType == Tui_ref_type_NUMBER_16 ||
                thisType == Tui_ref_type_NUMBER_32 ||
                thisType == Tui_ref_type_NUMBER_64);
    }
    
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

protected:
    // Helper to create a TuiPointer instance from this object
    // Has a built-in cast, defaults to TuiRef
    // NOTE: This can only be used if the TuiRef object is managed by a TuiPointer,
    // otherwise it will throw a weak pointer exception
    template<typename T = TuiRef>
    inline TuiPointer<T> createFromThis() {
        return Tui::cast<T>(shared_from_this());
    }
};


class TuiNil : public TuiRef {

public:
    TuiNil() {}
    virtual ~TuiNil() {}
    virtual Tui::ref copy() override {return createFromThis();}
    virtual void assign(const Tui::ref& other) override {}

    virtual uint8_t type() override { return Tui_ref_type_NIL; }
    virtual std::string getTypeName() override {return "nil";}
    virtual std::string getStringValue() override {return "nil";}
    virtual bool boolValue() override {return false;}
    virtual bool isEqual(const Tui::ref& other) override {return (!other || other->type() == Tui_ref_type_NIL );}
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 1);
        buffer[(*currentOffset)++] = Tui_binary_type_NIL;
    }

private:
    
private:
};

static Tui::nil TUI_NIL = Tui::create<TuiNil>();

class TuiUserData : public TuiRef {
public:
    void* value;

public:
    TuiUserData(void* value_);
    virtual ~TuiUserData() {}
    
    virtual Tui::ref copy() override //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        return createFromThis();
    }
    
    virtual void assign(const Tui::ref& other) override {
        value = (Tui::cast<TuiUserData>(other))->value;
    };
    
    virtual uint8_t type() override { return Tui_ref_type_USERDATA; }
    virtual std::string getTypeName() override {return "userData";}
    virtual std::string getStringValue() override {
        return Tui::string_format("%p", value);
    }
    virtual bool boolValue() override {return value != nullptr;}
    virtual bool isEqual(const Tui::ref& other) override {return other && other->type() == Tui_ref_type_USERDATA && (Tui::cast<TuiUserData>(other))->value == value;}
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        TuiError("Userdata objects do not support binary serialization");
    }

private:
    
private:
};

#endif
