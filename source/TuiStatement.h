#ifndef TuiStatement_h
#define TuiStatement_h

#include <vector>
#include <string>

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
    Tui_token_modulo,
    
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
    Tui_token_or,
    Tui_token_and,
    Tui_token_tableConstruct,
    
    Tui_token_varChain, //24
    Tui_token_childByString,
    Tui_token_childByArrayIndex,
    Tui_token_varName,
    Tui_token_true,
    Tui_token_false,
    Tui_token_forCollectionLoopKeyValue,
    Tui_token_forCollectionLoopValues,
    
    Tui_token_functionDeclaration, //32
    Tui_token_vec2,
    Tui_token_vec3,
    Tui_token_vec4,
    Tui_token_negate,
    
    Tui_token_VAR_START_INDEX
};

enum {
    Tui_statement_type_return = 0,
    Tui_statement_type_returnExpression,
    Tui_statement_type_varAssign, // x = expression assignment
    Tui_statement_type_value, // expression result will be added to arrayObjects
    Tui_statement_type_varModify, // += ++ etc
    Tui_statement_type_functionCall,
    Tui_statement_type_if,
    Tui_statement_type_forExpressions, // for(i = 0, i < 5, i++)
    Tui_statement_type_forKeyedValues, // for(indexOrKey, object in table)
    Tui_statement_type_forValues, // for(object in table)
};

struct TuiExpression {
    std::vector<uint32_t> tokens;
};

struct TuiTokenMap {
    uint32_t tokenIndex = Tui_token_VAR_START_INDEX;
    std::map<uint32_t, TuiRef*> refsByToken; //var names and constants. Captures are not stored here, they are to be loaded when the function is called
    std::map<std::string, uint32_t> capturedTokensByVarName;
    std::map<std::string, uint32_t> localTokensByVarName;
    std::map<int, uint32_t> capturedParentTokensByDepthCount;
};

struct TuiDebugInfo {
    std::string fileName;
    int lineNumber = 1;
};

class TuiStatement {
public: //members
    TuiDebugInfo debugInfo;
    uint32_t type;
    std::string varName; //only stored for var assign statements
    TuiExpression* expression = nullptr;
    
public://functions
    TuiStatement(uint8_t type_) {type = type_;}
    virtual ~TuiStatement() { if(expression) { delete expression;}}
};

class TuiIfStatement : public TuiStatement {
public://functions
    std::vector<TuiStatement*> statements;
    TuiIfStatement* elseIfStatement = nullptr;
    TuiIfStatement() : TuiStatement(Tui_statement_type_if) {}
};


class TuiForExpressionsStatement : public TuiStatement { // for(i = 0, i < 5, i++)
public://functions
    std::vector<TuiStatement*> statements;
    TuiStatement* initialStatement = nullptr; //todo cleanup
    TuiExpression* continueExpression = nullptr; //todo cleanup
    TuiStatement* incrementStatement = nullptr; //todo cleanup
    
    TuiForExpressionsStatement() : TuiStatement(Tui_statement_type_forExpressions) {}
};


class TuiForContainerLoopStatement : public TuiStatement { // for(object in table)
public://functions
    std::vector<TuiStatement*> statements;
    TuiForContainerLoopStatement(uint32_t type_) : TuiStatement(type_) {}
};


#endif
