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
#include "TuiStatement.h"

class TuiTable;
class TuiString;

struct TuiFunctionCallData {
    std::map<uint32_t, TuiRef*> locals; //need to release
    TuiTable* functionStateTable = nullptr;
};


class TuiFunction : public TuiRef {
    
public: //static functions
    static TuiFunction* initWithHumanReadableString(const char* str,
                                                    char** endptr,
                                                    TuiTable* parent,
                                                    TuiDebugInfo* debugInfo);
    
    static bool recursivelySerializeExpression(const char* str,
                                               char** endptr,
                                               TuiExpression* expression,
                                               TuiTable* parent,
                                               TuiTokenMap* tokenMap,
                                               TuiDebugInfo* debugInfo,
                                               int operatorLevel,
                                               std::string* setKey = nullptr,
                                               int* setIndex = nullptr,
                                               uint32_t subExpressionTokenStartPos = UINT32_MAX);
    
    static bool serializeFunctionBody(const char* str,
                                      char** endptr,
                                      TuiTable* parent,
                                      TuiTokenMap* tokenMap,
                                      TuiDebugInfo* debugInfo,
                                      bool sharesParentScope,
                                      std::vector<TuiStatement*>* statements);
    
    static TuiStatement* serializeForStatement(const char* str,
                                                  char** endptr,
                                                  TuiTable* parent,
                                                  TuiTokenMap* tokenMap,
                                                  TuiDebugInfo* debugInfo,
                                               bool sharesParentScope,
                                               bool isWhileLoop);
    
    
    
    static TuiRef* runExpression(TuiExpression* expression,
                                 uint32_t* tokenPos,
                                 TuiRef* result,
                                 TuiTable* parent,
                                 TuiTokenMap* tokenMap,
                                 TuiFunctionCallData* callData,
                                 TuiDebugInfo* debugInfo,
                                 std::string* setKey = nullptr,
                                 int* setIndex = nullptr,
                                 TuiRef** enclosingSetRef = nullptr,
                                 std::string* subTypeAccessKey = nullptr,
                                 TuiRef** subTypeRef = nullptr);
    
    static TuiRef* runStatement(TuiStatement* statement,
                                TuiRef* result,
                                TuiTable* parent,
                                TuiTokenMap* tokenMap,
                                TuiFunctionCallData* callData,
                                TuiDebugInfo* debugInfo,
                                bool* breakFound = nullptr);
    
    static TuiRef* runStatementArray(std::vector<TuiStatement*>& statements,
                                     TuiRef* result,
                                     TuiTable* parent,
                                     TuiTokenMap* tokenMap,
                                     TuiFunctionCallData* callData,
                                     TuiDebugInfo* debugInfo,
                                     bool* breakFound = nullptr);

public: //class members
    TuiTable* parentTable = nullptr;
    std::vector<std::string> argNames;
    std::vector<TuiStatement*> statements;
    std::function<TuiRef*(TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func;
    
    TuiTokenMap tokenMap;
    
    TuiDebugInfo debugInfo;
    uint32_t functionLineNumber; //save a copy so we can change it in debugInfo, which will save a string copy
    
public: //class functions
    TuiFunction(TuiTable* parentTable_);
    TuiFunction(std::function<TuiRef*(TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo)> func_);
    virtual ~TuiFunction();
    
    virtual TuiRef* copy() //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        retain();
        return this;
    }
    
    TuiFunction* trueCopy()
    {
        TuiFunction* copied = new TuiFunction(parentTable);
        copied->argNames = argNames;
        copied->statements = statements;
        for(TuiStatement* statement : copied->statements)
        {
            statement->refCount++;
        }
        copied->func = func;
        copied->tokenMap = tokenMap;
        for(auto& tokenAndRef : copied->tokenMap.refsByToken)
        {
            tokenAndRef.second->retain();
        }
        copied->debugInfo = debugInfo;
        copied->functionLineNumber = functionLineNumber;
        return copied;
    }
    
    virtual uint8_t type() { return Tui_ref_type_FUNCTION; }
    virtual std::string getTypeName() {return "function";}
    virtual std::string getStringValue() {return "function";}
    virtual bool isEqual(TuiRef* other) {return other == this;}
    
    virtual bool boolValue() {return true;}
    
    TuiRef* call(TuiTable* args,
                 TuiRef* existingResult,
                 TuiDebugInfo* callingDebugInfo);
    
    TuiRef* runTableConstruct(TuiTable* state,
                 TuiRef* existingResult,
                 TuiDebugInfo* callingDebugInfo);
    
    TuiRef* call(const std::string& debugName,
                 TuiRef* arg1 = nullptr,
                 TuiRef* arg2 = nullptr,
                 TuiRef* arg3 = nullptr,
                 TuiRef* arg4 = nullptr,
                 TuiRef* arg5 = nullptr,
                 TuiRef* arg6 = nullptr,
                 TuiRef* arg7 = nullptr,
                 TuiRef* arg8 = nullptr);
    
    //void call(TuiTable* args, std::function<void(TuiRef*)> callback); //todo async
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset)
    {
        TuiError("TuiFunction does not support binary serialization");
    }
    
private:
};

#endif
