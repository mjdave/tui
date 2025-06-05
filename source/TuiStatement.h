#ifndef TuiStatement_h
#define TuiStatement_h

/*#include <stdio.h>
#include <string>
#include <set>
#include <functional>
#include <map>
#include <vector>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"
#include "TuiString.h"*/

//class TuiTable;

class TuiString;
class TuiRef;

enum {
    Tui_token_nil = 0,
    Tui_token_functionCall,
    Tui_token_end,
    Tui_token_add,
    Tui_token_subtract,
    Tui_token_divide,
    Tui_token_multiply,
    Tui_token_equalTo,
    
    Tui_token_notEqualTo, //8
    Tui_token_lessThan,
    Tui_token_greaterThan,
    Tui_token_greaterEqualTo,
    Tui_token_lessEqualTo,
    Tui_token_not,
    Tui_token_increment,
    Tui_token_decrement,
    
    Tui_token_addInPlace, //16
    Tui_token_subtractInPlace,
    Tui_token_multiplyInPlace,
    Tui_token_divideInPlace,
    Tui_token_or,
    Tui_token_and,
    Tui_token_tableConstruct,
    Tui_token_VAR_START_INDEX
};

enum {
    Tui_statement_type_RETURN = 0,
    Tui_statement_type_RETURN_EXPRESSION,
    Tui_statement_type_VAR_ASSIGN,
    Tui_statement_type_FUNCTION_CALL,
    Tui_statement_type_IF,
    Tui_statement_type_FOR
};


struct TuiTokenMap {
    uint32_t tokenIndex = Tui_token_VAR_START_INDEX;
    std::map<uint32_t, TuiRef*> refsByToken;
    std::map<std::string, uint32_t> readWriteTokensByVarNames;
    std::map<std::string, uint32_t> readOnlyTokensByVarNames;
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


#endif
