#ifndef TuiFunction_h
#define TuiFunction_h

#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"
#include "TuiString.h"

class TuiTable;


enum {
    Tui_TOKEN_nil = 0,
    Tui_TOKEN_pad,
    Tui_TOKEN_functionCall,
    Tui_TOKEN_end,
    Tui_TOKEN_add,
    Tui_TOKEN_subtract,
    Tui_TOKEN_divide,
    Tui_TOKEN_multiply,
    Tui_TOKEN_equalTo,
    Tui_TOKEN_lessThan,
    Tui_TOKEN_greaterThan,
    Tui_TOKEN_greaterEqualTo,
    Tui_TOKEN_lessEqualTo,
    Tui_TOKEN_VAR_START_INDEX
};

enum {
    TuiSTATEMENT_TYPE_RETURN = 0,
    TuiSTATEMENT_TYPE_RETURN_EXPRESSION,
    TuiSTATEMENT_TYPE_VAR_ASSIGN,
    TuiSTATEMENT_TYPE_FUNCTION_CALL,
    TuiSTATEMENT_TYPE_IF,
    TuiSTATEMENT_TYPE_FOR,
};


struct TuiTokenMap {
    uint32_t tokenIndex = Tui_TOKEN_VAR_START_INDEX;
    std::map<uint32_t, TuiRef*> refsByToken;
    std::map<std::string, uint32_t> tokensByVarNames;
};

struct TuiExpression {
    std::vector<uint32_t> tokens;
};

class TuiStatement {
public: //members
    uint32_t lineNumber;
    uint32_t type;
    TuiString* varName = nullptr;
    uint32_t varToken = 0;
    TuiExpression* expression = nullptr;
    
public://functions
    TuiStatement(uint8_t type_) {type = type_;}
    virtual ~TuiStatement() { if(expression) { delete expression;}}
};

class TuiIfStatement : public TuiStatement {
public://functions
    std::vector<TuiStatement*> statements;
    TuiIfStatement* elseIfStatement = nullptr;
    TuiIfStatement() : TuiStatement(TuiSTATEMENT_TYPE_IF) {}
};


class TuiForStatement : public TuiStatement {
public://functions
    std::vector<TuiStatement*> statements;
    TuiExpression* continueExpression = nullptr;
    TuiStatement* incrementStatement = nullptr;
    
    TuiForStatement() : TuiStatement(TuiSTATEMENT_TYPE_FOR) {}
};

class TuiFunction : public TuiRef {
    
public: //static functions
    static TuiFunction* initWithHumanReadableString( const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo);
    
    static void recursivelySerializeExpression(     const char* str, char** endptr, TuiExpression* expression,  TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, bool runLowOperators);
    static bool serializeFunctionBody(              const char* str, char** endptr,                             TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, std::vector<TuiStatement*>* statements);
    static TuiForStatement* serializeForStatement(  const char* str, char** endptr,                             TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo);
    
    
    static TuiRef* runExpression(TuiExpression* expression, uint32_t* tokenIndex,   TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo);
    static TuiRef* runStatement(                 TuiStatement* statement,           TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo);
    static TuiRef* runStatementArray(std::vector<TuiStatement*>& statements,        TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo);

public: //class members
    std::vector<std::string> argNames;
    std::vector<TuiStatement*> statements;
    std::function<TuiRef*(TuiTable* args, TuiTable* state)> func;
    
    TuiTokenMap tokenMap;
    
    TuiDebugInfo debugInfo;
    uint32_t functionLineNumber; //save a copy so we can change it in debugInfo, which will save a string copy
    
public: //class functions
    TuiFunction(TuiRef* parent_);
    TuiFunction(std::function<TuiRef*(TuiTable* args, TuiTable* state)> func_, TuiRef* parent_);
    virtual ~TuiFunction();
    
    virtual TuiFunction* copy()
    {
        return new TuiFunction(parent);
    }
    
    
    virtual uint8_t type() { return Tui_ref_type_FUNCTION; }
    virtual std::string getTypeName() {return "function";}
    virtual std::string getStringValue() {return "function";}
    
    virtual bool boolValue() {return true;}
    
    TuiRef* call(TuiTable* args, TuiTable* state, TuiRef* existingResult);
    //void call(TuiTable* args, std::function<void(TuiRef*)> callback); //todo async
    
    virtual TuiRef* recursivelyFindVariable(TuiString* variableName, TuiDebugInfo* debugInfo, bool searchParents, int varStartIndex = 0);
    virtual bool recursivelySetVariable(TuiString* variableName, TuiRef* value, TuiDebugInfo* debugInfo, int varStartIndex = 0);
    
private:
};

#endif
