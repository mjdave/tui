#ifndef TuiFunction_h
#define TuiFunction_h

#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <list>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"
#include "TuiString.h"

class TuiTable;

enum {
    Tui_token_nil = 0,
    Tui_token_pad,
    Tui_token_functionCall,
    Tui_token_end,
    Tui_token_add,
    Tui_token_subtract,
    Tui_token_divide,
    Tui_token_multiply,
    
    Tui_token_equalTo, //8
    Tui_token_notEqualTo,
    Tui_token_lessThan,
    Tui_token_greaterThan,
    Tui_token_greaterEqualTo,
    Tui_token_lessEqualTo,
    Tui_token_not,
    Tui_token_increment,
    
    Tui_token_decrement, //16
    Tui_token_addInPlace,
    Tui_token_subtractInPlace,
    Tui_token_multiplyInPlace,
    Tui_token_divideInPlace,
    Tui_token_VAR_START_INDEX
};

enum {
    Tui_statement_type_RETURN = 0,
    Tui_statement_type_RETURN_EXPRESSION,
    Tui_statement_type_VAR_ASSIGN,
    Tui_statement_type_FUNCTION_CALL,
    Tui_statement_type_IF,
    Tui_statement_type_FOR,
};


struct TuiTokenMap {
    uint32_t tokenIndex = Tui_token_VAR_START_INDEX;
    std::map<uint32_t, TuiRef*> refsByToken;
    std::map<std::string, uint32_t> tokensByVarNames;
};

struct TuiExpression {
    std::list<uint32_t> tokens;
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
    TuiIfStatement() : TuiStatement(Tui_statement_type_IF) {}
};


class TuiForStatement : public TuiStatement {
public://functions
    std::vector<TuiStatement*> statements;
    TuiExpression* continueExpression = nullptr;
    TuiStatement* incrementStatement = nullptr;
    
    TuiString* indexOrKeyName = nullptr; //todo leaks
    uint32_t indexOrKeyToken = 0;
    
    
    TuiForStatement() : TuiStatement(Tui_statement_type_FOR) {}
};

class TuiFunction : public TuiRef {
    
public: //static functions
    static TuiFunction* initWithHumanReadableString( const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo);
    
    static bool recursivelySerializeExpression(     const char* str, char** endptr, TuiExpression* expression,  TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, int operatorLevel, std::list<uint32_t>::iterator* subExpressionTokenStartPos = nullptr);
    static bool serializeFunctionBody(              const char* str, char** endptr,                             TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, std::vector<TuiStatement*>* statements);
    static TuiForStatement* serializeForStatement(  const char* str, char** endptr,                             TuiRef* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo);
    
    
    static TuiRef* runExpression(TuiExpression* expression, std::list<uint32_t>::iterator& tokenPos,   TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo);
    static TuiRef* runStatement(                 TuiStatement* statement,           TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo);
    static TuiRef* runStatementArray(std::vector<TuiStatement*>& statements,        TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>& locals, TuiDebugInfo* debugInfo);

public: //class members
    std::vector<std::string> argNames;
    std::vector<TuiStatement*> statements;
    std::function<TuiRef*(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func;
    
    TuiTokenMap tokenMap;
    
    TuiDebugInfo debugInfo;
    uint32_t functionLineNumber; //save a copy so we can change it in debugInfo, which will save a string copy
    
public: //class functions
    TuiFunction(TuiRef* parent_);
    TuiFunction(std::function<TuiRef*(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func_, TuiRef* parent_);
    virtual ~TuiFunction();
    
    virtual TuiFunction* copy()
    {
        return new TuiFunction(parent);
    }
    
    
    virtual uint8_t type() { return Tui_ref_type_FUNCTION; }
    virtual std::string getTypeName() {return "function";}
    virtual std::string getStringValue() {return "function";}
    virtual bool isEqual(TuiRef* other) {return other == this;}
    
    virtual bool boolValue() {return true;}
    
    TuiRef* call(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo);
    //void call(TuiTable* args, std::function<void(TuiRef*)> callback); //todo async
    
    virtual TuiRef* recursivelyFindVariable(TuiString* variableName, TuiDebugInfo* debugInfo, bool searchParents, int varStartIndex = 0);
    virtual bool recursivelySetVariable(TuiString* variableName, TuiRef* value, TuiDebugInfo* debugInfo, int varStartIndex = 0);
    
private:
};

#endif
