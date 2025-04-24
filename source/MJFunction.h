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
    MJSTATEMENT_TYPE_FOR,
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


class MJForStatement : public MJStatement {
public://functions
    std::vector<MJStatement*> statements;
    std::string continueExpression;
    MJStatement* incrementStatement = nullptr;
    
    MJForStatement() : MJStatement(MJSTATEMENT_TYPE_FOR) {}
};

class MJFunction : public MJRef {
    
public: //static functions
    static void serializeExpression(const char* str, char** endptr, std::string& expression, MJDebugInfo* debugInfo);
    static MJFunction* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo);
    static bool loadFunctionBody(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo, std::vector<MJStatement*>* statements);
    static MJRef* runStatement(MJStatement* statement, MJTable* functionState, MJTable* parent, MJDebugInfo* debugInfo);
    static MJRef* runStatementArray(std::vector<MJStatement*>& statements, MJTable* functionState, MJTable* parent, MJDebugInfo* debugInfo);
    static MJRef* recursivelyRunIfElseStatement(MJIfStatement* elseIfStatement, MJTable* functionState, MJTable* parent, MJDebugInfo* debugInfo);
    static MJForStatement* loadForStatement(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo);

public: //class members
    std::vector<std::string> argNames;
    std::vector<MJStatement*> statements;
    std::function<MJRef*(MJTable* args, MJTable* state)> func;
    
    MJDebugInfo debugInfo;
    uint32_t functionLineNumber; //save a copy so we can change it in debugInfo, which will save a string copy
    
public: //class functions
    MJFunction(MJRef* parent_);
    MJFunction(std::function<MJRef*(MJTable* args, MJTable* state)> func_, MJRef* parent_);
    virtual ~MJFunction();
    
    virtual MJFunction* copy()
    {
        return new MJFunction(parent);
    }
    
    
    virtual uint8_t type() { return MJREF_TYPE_FUNCTION; }
    virtual std::string getTypeName() {return "function";}
    virtual std::string getStringValue() {return "function";}
    
    virtual bool boolValue() {return true;}
    
    MJRef* call(MJTable* args, MJTable* state);
    //void call(MJTable* args, std::function<void(MJRef*)> callback);
    
    virtual MJRef* recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, int varStartIndex = 0);
    virtual bool recursivelySetVariable(MJString* variableName, MJRef* value, MJDebugInfo* debugInfo, int varStartIndex = 0);
    
private:
};

#endif
