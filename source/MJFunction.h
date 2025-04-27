#ifndef MJFunction_h
#define MJFunction_h

#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include "glm.hpp"
#include "MJLog.h"

#include "MJRef.h"
#include "MJString.h"

class MJTable;


enum {
    MJ_TOKEN_nil = 0,
    MJ_TOKEN_pad,
    MJ_TOKEN_functionCall,
    MJ_TOKEN_end,
    MJ_TOKEN_add,
    MJ_TOKEN_subtract,
    MJ_TOKEN_divide,
    MJ_TOKEN_multiply,
    MJ_TOKEN_equalTo,
    MJ_TOKEN_lessThan,
    MJ_TOKEN_greaterThan,
    MJ_TOKEN_greaterEqualTo,
    MJ_TOKEN_lessEqualTo,
    MJ_TOKEN_VAR_START_INDEX
};

enum {
    MJSTATEMENT_TYPE_RETURN = 0,
    MJSTATEMENT_TYPE_RETURN_EXPRESSION,
    MJSTATEMENT_TYPE_VAR_ASSIGN,
    MJSTATEMENT_TYPE_FUNCTION_CALL,
    MJSTATEMENT_TYPE_IF,
    MJSTATEMENT_TYPE_FOR,
};


struct MJTokenMap {
    uint32_t tokenIndex = MJ_TOKEN_VAR_START_INDEX;
    std::map<uint32_t, MJRef*> refsByToken;
    std::map<std::string, uint32_t> tokensByVarNames;
};

struct MJExpression {
    std::vector<uint32_t> tokens;
};

class MJStatement {
public: //members
    uint32_t lineNumber;
    uint32_t type;
    MJString* varName = nullptr;
    uint32_t varToken = 0;
    MJExpression* expression = nullptr;
    
public://functions
    MJStatement(uint8_t type_) {type = type_;}
    virtual ~MJStatement() { if(expression) { delete expression;}}
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
    MJExpression* continueExpression = nullptr;
    MJStatement* incrementStatement = nullptr;
    
    MJForStatement() : MJStatement(MJSTATEMENT_TYPE_FOR) {}
};

class MJFunction : public MJRef {
    
public: //static functions
    //static void serializeExpression(const char* str, char** endptr, MJExpression* expression, MJTable* parent, MJTokenMap* tokenMap, MJDebugInfo* debugInfo);
    static MJFunction* initWithHumanReadableString( const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo);
    
    static void recursivelySerializeExpression(     const char* str, char** endptr, MJExpression* expression,   MJRef* parent, MJTokenMap* tokenMap, MJDebugInfo* debugInfo, bool runLowOperators);
    static bool serializeFunctionBody(                   const char* str, char** endptr,                             MJRef* parent, MJTokenMap* tokenMap, MJDebugInfo* debugInfo, std::vector<MJStatement*>* statements);
    static MJForStatement* serializeForStatement(        const char* str, char** endptr,                             MJRef* parent, MJTokenMap* tokenMap, MJDebugInfo* debugInfo);
    
    
    static MJRef* runExpression(MJExpression* expression, uint32_t* tokenIndex,  MJRef* result,  MJTable* functionState, MJTable* parent, MJTokenMap* tokenMap, std::map<uint32_t, MJRef*>& locals, MJDebugInfo* debugInfo);
    static MJRef* runStatement(                 MJStatement* statement,         MJRef* result,  MJTable* functionState, MJTable* parent, MJTokenMap* tokenMap, std::map<uint32_t, MJRef*>& locals, MJDebugInfo* debugInfo);
    static MJRef* runStatementArray(std::vector<MJStatement*>& statements,      MJRef* result,  MJTable* functionState, MJTable* parent, MJTokenMap* tokenMap, std::map<uint32_t, MJRef*>& locals, MJDebugInfo* debugInfo);

public: //class members
    std::vector<std::string> argNames;
    std::vector<MJStatement*> statements;
    std::function<MJRef*(MJTable* args, MJTable* state)> func;
    
    MJTokenMap tokenMap;
    
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
    
    MJRef* call(MJTable* args, MJTable* state, MJRef* existingResult);
    //void call(MJTable* args, std::function<void(MJRef*)> callback);
    
    virtual MJRef* recursivelyFindVariable(MJString* variableName, MJDebugInfo* debugInfo, bool searchParents, int varStartIndex = 0);
    virtual bool recursivelySetVariable(MJString* variableName, MJRef* value, MJDebugInfo* debugInfo, int varStartIndex = 0);
    
private:
};

#endif
