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
    Tui::table parentTable = nullptr;
    Tui::table thisTable = nullptr;
    std::map<std::string, uint32_t> localTokensByStringKey;
    std::map<uint32_t, Tui::ref> locals; //need to release
    std::vector<Tui::table> transientLoopTables;
    std::vector<Tui::function> capturedFunctions;
};


class TuiFunction : public TuiRef {
    
public: //static functions
    static Tui::function initWithHumanReadableString(const char* str,
                                                    char** endptr,
                                                    const Tui::table& parent,
                                                    TuiDebugInfo* debugInfo);
    
    static bool recursivelySerializeExpression(const char* str,
                                               char** endptr,
                                               TuiExpression* expression,
                                               const Tui::table& parent,
                                               TuiTokenMap* tokenMap,
                                               TuiDebugInfo* debugInfo,
                                               int operatorLevel,
                                               std::string* setKey = nullptr,
                                               int* setIndex = nullptr,
                                               uint32_t subExpressionTokenStartPos = UINT32_MAX);
    
    static bool serializeFunctionBody(const char* str,
                                      char** endptr,
                                      const Tui::table& parent,
                                      TuiTokenMap* tokenMap,
                                      TuiDebugInfo* debugInfo,
                                      bool sharesParentScope,
                                      std::vector<TuiStatement*>* statements);
    
    static TuiStatement* serializeForStatement(const char* str,
                                                  char** endptr,
                                                  const Tui::table& parent,
                                                  TuiDebugInfo* debugInfo,
                                               bool sharesParentScope,
                                               bool isWhileLoop);
    
    
    
    static Tui::ref runExpression(TuiExpression* expression,
                                 uint32_t* tokenPos,
                                 const Tui::ref& result,
                                 const Tui::table& parent,
                                 TuiTokenMap* tokenMap,
                                 TuiFunctionCallData* callData,
                                 TuiDebugInfo* debugInfo,
                                 std::string* setKey = nullptr,
                                 int* setIndex = nullptr,
                                 Tui::ref* enclosingSetRef = nullptr,
                                 std::string* subTypeAccessKey = nullptr,
                                 Tui::ref* subTypeRef = nullptr);
    
    static Tui::ref runStatement(TuiStatement* statement,
                                const Tui::ref& result,
                                const Tui::table& parent,
                                TuiTokenMap* tokenMap,
                                TuiFunctionCallData* callData,
                                TuiDebugInfo* debugInfo,
                                bool* breakFound = nullptr);
    
    static Tui::ref runStatementArray(std::vector<TuiStatement*>& statements,
                                     const Tui::ref& result,
                                     const Tui::table& parent,
                                     TuiTokenMap* tokenMap,
                                     TuiFunctionCallData* callData,
                                     TuiDebugInfo* debugInfo,
                                     bool* breakFound = nullptr);

public: //class members
    Tui::table parentTable = nullptr;
    std::vector<std::string> argNames;
    std::vector<TuiStatement*> statements;
    std::function<Tui::ref(const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> func;
    
    TuiTokenMap tokenMap;
    std::vector<Tui::table> retainedTransientLoopTables;
    
    TuiDebugInfoLine debugInfoLine;
    
public: //class functions
    TuiFunction(Tui::table parentTable_);
    TuiFunction(std::function<Tui::ref(const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo)> func_);
    virtual ~TuiFunction() {};
    
    virtual Tui::ref copy() override //NOTE! This is not a true copy, copy is called internally when assigning vars, but tables, function, and userdata are treated like pointers
    {
        return createPointerFromThis();
    }
    
    virtual void release() override;
    void releaseAndRemoveTransientLoopTables();
    
    Tui::function trueCopy() //assumed to only be used for function construction from a pre-serialized prototype
    {
        Tui::function copied = Tui::createPointer<TuiFunction>(parentTable);
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
    virtual std::string getStringValue() override {return "function";}
    virtual bool isEqual(const Tui::ref& other) override {return other.get() == this;}
    
    virtual bool boolValue() override {return true;}
    
    Tui::ref call(const Tui::table& args,
                 const Tui::ref& existingResult,
                 TuiFunctionCallData* incomingCallData,
                 TuiDebugInfo* callingDebugInfo);
    
    Tui::ref runTableConstruct(const Tui::table& state,
                 const Tui::ref& existingResult,
                 TuiDebugInfo* callingDebugInfo);
    
    Tui::ref call(const std::string& debugName,
                 const Tui::ref& arg1 = nullptr,
                 const Tui::ref& arg2 = nullptr,
                 const Tui::ref& arg3 = nullptr,
                 const Tui::ref& arg4 = nullptr,
                 const Tui::ref& arg5 = nullptr,
                 const Tui::ref& arg6 = nullptr,
                 const Tui::ref& arg7 = nullptr,
                 const Tui::ref& arg8 = nullptr);
    
    Tui::ref call(TuiFunctionCallData* incomingCallData,
                              TuiDebugInfo* callingDebugInfo,
                              const Tui::ref& arg1 = nullptr,
                              const Tui::ref& arg2 = nullptr,
                              const Tui::ref& arg3 = nullptr,
                              const Tui::ref& arg4 = nullptr,
                              const Tui::ref& arg5 = nullptr,
                              const Tui::ref& arg6 = nullptr,
                              const Tui::ref& arg7 = nullptr,
                              const Tui::ref& arg8 = nullptr);
    
    //void call(Tui::table args, std::function<void(Tui::ref)> callback); //todo async
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        TuiError("TuiFunction does not support binary serialization");
    }
    
private:
};

#endif
