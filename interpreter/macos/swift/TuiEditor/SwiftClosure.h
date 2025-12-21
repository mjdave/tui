//
//  SwiftClosure.h
//  TuiEditor
//

#pragma once

#include <functional>
#include <memory>
#include <swift/bridging>

#include "TuiTable.h"

class SwiftClosure final {
public:
    using VoidFunc = void();
    using ArgsFunc = char*(void* _Nonnull args);
    using NoArgsContextFunc = char*(void* _Nonnull);
    
    void* _Nullable context = nullptr;
    
    explicit SwiftClosure(VoidFunc* _Nonnull call) {
        _function = [call]() {
            call();
        };
    }
    
    explicit SwiftClosure(ArgsFunc* _Nonnull call) {
        _argsFunction = [call](void* _Nonnull args) {
            return call(args);
        };
    }
    
    explicit SwiftClosure(void* _Nonnull context, NoArgsContextFunc* _Nonnull call) {
        this->context = context;
        _noArgsContextFunction = [context, call]() {
            // Call the actual Swift closure.
            return call(context);
        };
    }
    
    void setFunction(const std::string& key, TuiTable* _Nonnull rootTable)
    {
        rootTable->setFunction(key, [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
            this->_function();
            return nullptr;
        });
    }
    
    void setFunctionWithReturnValue(const std::string& key, TuiTable* _Nonnull rootTable)
    {
        rootTable->setFunction(key, [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
            char* cStr = this->_noArgsContextFunction();
            if (cStr == NULL) {
                return TUI_NIL;
            }
            TuiString* tuiString = new TuiString(cStr);
            free(cStr);
            return tuiString;
        });
    }
    
    void setFunctionWithArgs(const std::string& key, TuiTable* _Nonnull rootTable)
    {
        rootTable->setFunction(key, [this](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
            char* cStr = this->_argsFunction(args);
            if (cStr == NULL) {
                return TUI_NIL;
            }
            TuiString* tuiString = new TuiString(cStr);
            free(cStr);
            return tuiString;
        });
    }

private:
    std::function<void()> _function;
    std::function<char*()> _noArgsContextFunction;
    std::function<char*(TuiTable* _Nonnull args)> _argsFunction;
} SWIFT_UNSAFE_REFERENCE;
