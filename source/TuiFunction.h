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
#include "TuiTypes.h"

#include "TuiRef.h"
#include "TuiStatement.h"

class TuiTable;
class TuiString;
class TuiFunction;

struct TuiFunctionCallData {
    TuiFunctionCallData* parentCallData = nullptr;
    tui::table parentTable = nullptr;
    tui::table thisTable = nullptr;
    std::map<std::string, uint32_t> localTokensByStringKey;
    std::map<uint32_t, tui::ref> locals; //need to release
    std::vector<tui::table> transientLoopTables;
    std::vector<tui::function> capturedFunctions;
};


namespace tui {

    inline tui::function newFunction(std::function<tui::ref(const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> func_)
    {
        return tui::create<TuiFunction>(func_);
    }
}

class TuiFunction : public TuiRef {
    
public: //static functions
    static tui::function initWithHumanReadableString(const char* str,
                                                    char** endptr,
                                                    const tui::table& parent,
                                                    TuiDebugInfo* debugInfo);
    
    static bool recursivelySerializeExpression(const char* str,
                                               char** endptr,
                                               TuiExpression* expression,
                                               const tui::table& parent,
                                               TuiTokenMap* tokenMap,
                                               TuiDebugInfo* debugInfo,
                                               int operatorLevel,
                                               std::string* setKey = nullptr,
                                               int* setIndex = nullptr,
                                               uint32_t subExpressionTokenStartPos = UINT32_MAX);
    
    static bool serializeFunctionBody(const char* str,
                                      char** endptr,
                                      const tui::table& parent,
                                      TuiTokenMap* tokenMap,
                                      TuiDebugInfo* debugInfo,
                                      bool sharesParentScope,
                                      std::vector<TuiStatement*>* statements);
    
    static TuiStatement* serializeForStatement(const char* str,
                                                  char** endptr,
                                                  const tui::table& parent,
                                                  TuiDebugInfo* debugInfo,
                                               bool sharesParentScope,
                                               bool isWhileLoop);
    
    
    
    static tui::ref runExpression(TuiExpression* expression,
                                 uint32_t* tokenPos,
                                 const tui::ref& result,
                                 const tui::table& parent,
                                 TuiTokenMap* tokenMap,
                                 TuiFunctionCallData* callData,
                                 TuiDebugInfo* debugInfo,
                                 std::string* setKey = nullptr,
                                 int* setIndex = nullptr,
                                 tui::table* enclosingSetRef = nullptr,
                                 std::string* subTypeAccessKey = nullptr,
                                 tui::ref* subTypeRef = nullptr);
    
    static tui::ref runStatement(TuiStatement* statement,
                                const tui::ref& result,
                                const tui::table& parent,
                                TuiTokenMap* tokenMap,
                                TuiFunctionCallData* callData,
                                TuiDebugInfo* debugInfo,
                                bool* breakFound = nullptr);
    
    static tui::ref runStatementArray(std::vector<TuiStatement*>& statements,
                                     const tui::ref& result,
                                     const tui::table& parent,
                                     TuiTokenMap* tokenMap,
                                     TuiFunctionCallData* callData,
                                     TuiDebugInfo* debugInfo,
                                     bool* breakFound = nullptr);

public: //class members
    tui::table parentTable = nullptr;
    std::vector<std::string> argNames;
    std::vector<TuiStatement*> statements;
    std::function<tui::ref(const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> func;
    
    TuiTokenMap tokenMap;
    std::vector<tui::table> retainedTransientLoopTables;
    
    TuiDebugInfoLine debugInfoLine;
    
public: //class functions
    TuiFunction(tui::table parentTable_);
    TuiFunction(std::function<tui::ref(const tui::table& args, const tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> func_);
    virtual ~TuiFunction() {};
    
    virtual tui::ref copy() override //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        return createFromThis();
    }
    
    void releaseAndRemoveTransientLoopTables();
    
    tui::function trueCopy() //assumed to only be used for function construction from a pre-serialized prototype
    {
        tui::function copied = tui::create<TuiFunction>(parentTable);
        copied->argNames = argNames;
        copied->statements = statements;
        for(TuiStatement* statement : copied->statements)
        {
            statement->refCount++;
        }
        copied->func = func;
        copied->tokenMap = tokenMap;
        copied->debugInfoLine = debugInfoLine;
        return copied;
    }
    
    virtual uint8_t type() override { return Tui_ref_type_FUNCTION; }
    virtual std::string getTypeName() override {return "function";}
    virtual std::string stringValue() override {return "function";}
    virtual bool isEqual(const tui::ref& other) override {return other.get() == this;}
    
    virtual bool boolValue() override {return true;}
    
    tui::ref call(const tui::table& args,
                 const tui::ref& existingResult,
                 TuiFunctionCallData* incomingCallData,
                 TuiDebugInfo* callingDebugInfo);
    
    tui::ref runTableConstruct(const tui::table& state,
                 const tui::ref& existingResult,
                 TuiDebugInfo* callingDebugInfo);
    
    tui::ref call(const std::string& debugName,
                 const tui::ref& arg1 = nullptr,
                 const tui::ref& arg2 = nullptr,
                 const tui::ref& arg3 = nullptr,
                 const tui::ref& arg4 = nullptr,
                 const tui::ref& arg5 = nullptr,
                 const tui::ref& arg6 = nullptr,
                 const tui::ref& arg7 = nullptr,
                 const tui::ref& arg8 = nullptr);
    
    tui::ref call(TuiFunctionCallData* incomingCallData,
                              TuiDebugInfo* callingDebugInfo,
                              const tui::ref& arg1 = nullptr,
                              const tui::ref& arg2 = nullptr,
                              const tui::ref& arg3 = nullptr,
                              const tui::ref& arg4 = nullptr,
                              const tui::ref& arg5 = nullptr,
                              const tui::ref& arg6 = nullptr,
                              const tui::ref& arg7 = nullptr,
                              const tui::ref& arg8 = nullptr);
    
    //void call(tui::table args, std::function<void(tui::ref)> callback); //todo async
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        TuiError("TuiFunction does not support binary serialization");
    }
    
private:
};

#endif
