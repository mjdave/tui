#ifndef TuiFunction_h
#define TuiFunction_h

#include <stdio.h>
#include <string>
#include <set>
#include <functional>
#include <map>
#include <vector>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"
//#include "TuiString.h"
#include "TuiStatement.h"

class TuiTable;
class TuiString;


class TuiFunction : public TuiRef {
    
public: //static functions
    static TuiFunction* initWithHumanReadableString( const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo);
    
    static bool recursivelySerializeExpression(     const char* str, char** endptr, TuiExpression* expression,  TuiTable* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, int operatorLevel, uint32_t subExpressionTokenStartPos = UINT32_MAX);
    static bool serializeFunctionBody(              const char* str, char** endptr,                             TuiTable* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo, std::vector<TuiStatement*>* statements);
    static TuiForStatement* serializeForStatement(  const char* str, char** endptr,                             TuiTable* parent, TuiTokenMap* tokenMap, TuiDebugInfo* debugInfo);
    
    
    static TuiRef* runExpression(TuiExpression* expression, uint32_t* tokenPos,   TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo);
    static TuiRef* runStatement(                 TuiStatement* statement,           TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo);
    static TuiRef* runStatementArray(std::vector<TuiStatement*>& statements,        TuiRef* result,  TuiTable* functionState, TuiTable* parent, TuiTokenMap* tokenMap, std::map<uint32_t, TuiRef*>* locals, TuiDebugInfo* debugInfo);

public: //class members
    std::vector<std::string> argNames;
    std::vector<TuiStatement*> statements;
    std::function<TuiRef*(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func;
    
    TuiTokenMap tokenMap;
    
    TuiDebugInfo debugInfo;
    uint32_t functionLineNumber; //save a copy so we can change it in debugInfo, which will save a string copy
    
public: //class functions
    TuiFunction(TuiTable* parent_);
    TuiFunction(std::function<TuiRef*(TuiTable* args, TuiTable* state, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func_, TuiTable* parent_);
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
    
    TuiRef* call(TuiTable* args,
                 TuiTable* state,
                 TuiRef* existingResult,
                 std::map<uint32_t, TuiRef*>* locals,
                 TuiDebugInfo* callingDebugInfo,
                 TuiRef** createdStateTable = nullptr);
    //void call(TuiTable* args, std::function<void(TuiRef*)> callback); //todo async
    
    
private:
};

#endif
