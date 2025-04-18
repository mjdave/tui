//
//  Database.h
//  Ambience
//
//  Created by David Frampton on 31/10/17.
//Copyright © 2017 Majic Jungle. All rights reserved.
//

#ifndef MJRef_h
#define MJRef_h

#include <stdio.h>
#include <string>
#include <set>

#include "glm.hpp"
#include "MJLog.h"
#include "FileUtils.h"
#include "StringUtils.h"

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
};

inline const char* skipToNextChar(const char* str, MJDebugInfo* debugInfo, bool stopAtNewLine = false)
{
    const char* s = str;
    bool comment = false;
    for(;; s++)
    {
        if(*s == '#')
        {
            comment = true;
        }
        else if(*s == '\n')
        {
            comment = false;
            if(stopAtNewLine)
            {
                break;
            }
            else
            {
                debugInfo->lineNumber++;
            }
        }
        else if(!comment && !isspace(*s))
        {
            break;
        }
    }
    
    return s;
}



class MJRef {
public: //members
    uint8_t refCount = 1;

public://functions
    MJRef() {}
    
    virtual ~MJRef() {}
    
    
    void release() {refCount--; if(refCount == 0) { delete this;}}
    void retain() {refCount++;}
    virtual MJRef* copy() {return new MJRef();};
    
    
    //static MJRef* initWithBinaryData(void* data);
    //static MJRef* initWithBinaryFile(const std::string& filePath);
    static MJRef* initWithHumanReadableString(const std::string& stringData) {return new MJRef();}
    //static MJRef* initWithHumanReadableFile(const std::string& filePath);
    
    //virtual size_t size();
    virtual uint8_t type() { return MJREF_TYPE_NIL; }
    virtual std::string getTypeName() {return "nil";}
    virtual bool allowExpressions() {return false;}
    
    std::string getDebugString() {
        std::string debugString;
        printHumanReadableString(debugString);
        return debugString;
    }
    
    virtual void debugLog() {
        MJLog("%s", getDebugString().c_str());
    }
    
    virtual std::string getStringValue() {return "nil";}
    //virtual uint64_t generateHash() {return 0;}
    
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += getStringValue();
    }
    
    
    void saveToFile(const std::string& filePath) {
        std::string exportString;
        printHumanReadableString(exportString);
        writeToFile(filePath, exportString);
    };
    
   // std::string serialize();
    //bool serializeToPath();
    
    //bool initWithData(const std::string& serialized);
    //bool initWithFile(const std::string& serialized);
    
private: //members
    
private: //functions

};

class MJUserData : public MJRef {
public: //members
    void* value;

public://functions
    MJUserData(void* value_) {value = value_;}
    virtual ~MJUserData() {};
    virtual MJUserData* copy() {return new MJUserData(value);};
    
    virtual uint8_t type() { return MJREF_TYPE_USERDATA; }
    virtual std::string getTypeName() {return "userData";}
    virtual std::string getStringValue() {
        return string_format("%p", value);
    }

private:
    
private:
};
/*
class MJFunction : public MJRef {
public: //members
    void* value;

public://functions
    MJFunction(void* value_) {value = value_;}
    virtual ~MJFunction() {};
    
    virtual uint8_t type() { return MJREF_TYPE_FUNCTION; }
    virtual std::string getTypeName() {return "function";}
    virtual std::string getStringValue() {
        return "function";
    }

private:
    
private:
};*/


#endif /* MJRef_h */
