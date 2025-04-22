#ifndef MJFunction_h
#define MJFunction_h

#include <stdio.h>
#include <string>
#include <set>
#include "glm.hpp"
#include "MJLog.h"

#include "MJRef.h"

class MJTable;

enum {
    MJSTATEMENT_TYPE_RETURN = 0,
    MJSTATEMENT_TYPE_RETURN_EXPRESSION,
    MJSTATEMENT_TYPE_VAR_ASSIGN,
    MJSTATEMENT_TYPE_FUNCTION_CALL,
    MJSTATEMENT_TYPE_IF,
};

class MJStatement {
public: //members
    uint32_t lineNumber;
    uint32_t type;
    std::string varName;
    std::string expression;
    
public://functions
    MJStatement(uint8_t type_) {type = type_;}
    virtual ~MJStatement() {}
};

class MJIfStatement : public MJStatement {
public://functions
    std::vector<MJStatement*> statements;
    MJIfStatement* elseIfStatement = nullptr;
    MJIfStatement() : MJStatement(MJSTATEMENT_TYPE_IF) {}
};

class MJFunction : public MJRef {
public: //members
    
    std::vector<std::string> argNames;
    std::vector<MJStatement*> statements;
    
    MJDebugInfo debugInfo;
    uint32_t functionLineNumber; //save a copy so we can change it in debugInfo, which will save a string copy

public://functions
    MJFunction(MJRef* parent_);
    MJFunction(std::function<MJRef*(MJTable* args)>, MJRef* parent_);
    virtual ~MJFunction();
    
    virtual MJFunction* copy()
    {
        return new MJFunction(parent);
    }
    
    static MJFunction* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo);
    
    virtual uint8_t type() { return MJREF_TYPE_FUNCTION; }
    virtual std::string getTypeName() {return "function";}
    virtual std::string getStringValue() {return "function";}
    
    virtual bool boolValue() {return true;}
    
    MJRef* runStatementArray(std::vector<MJStatement*>& statements, MJTable* functionState);
    
    MJRef* call(MJTable* args, MJTable* state);
    //void call(MJTable* args, std::function<void(MJRef*)> callback);
    
    virtual MJRef* recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, int varStartIndex = 0);
    virtual bool recursivelySetVariable(MJString* variableName, MJRef* value, MJDebugInfo* debugInfo, int varStartIndex = 0);

protected:
    
    MJRef* recursivelyRunIfElseStatement(MJIfStatement* elseIfStatement, MJTable* functionState);
    
private:
};

#endif
