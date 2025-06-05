
#ifndef TuiString_h
#define TuiString_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"
#include "TuiNumber.h"
#include "TuiStatement.h"
#include "TuiFunction.h"

enum {
    Tui_var_token_type_undefined = 0,
    Tui_var_token_type_string,
    Tui_var_token_type_parent,
    Tui_var_token_type_arrayIndex,
    Tui_var_token_type_setIndex,
    Tui_var_token_type_expression
};


enum {
    Tui_variable_load_type_ignore = 0, //variables will not be loaded into the vars member, treated as a simple string
    Tui_variable_load_type_serializeExpressions, //variable tokens are stored, will serialize an expression if hitting variable[getIndex()] Must pass a tokenMap
    Tui_variable_load_type_runExpressions //variable tokens are stored, will immediately run an expression if hitting variable[getIndex()]
};

struct TuiVarToken {
    uint32_t type = Tui_var_token_type_undefined;
    uint32_t arrayOrSetIndex;
    std::string varName;
    TuiExpression* expression = nullptr;
};

class TuiString : public TuiRef {
public: //members
    std::string value;
    bool allowAsVariableName = true; // this is set to false if it finds a quoted string when loaded via readable string
    bool isValidFunctionString = false; // optimization
    std::vector<TuiVarToken> vars; // optimization, finds look ups of sub-tables on load

public://functions
    
    virtual uint8_t type() { return Tui_ref_type_STRING; }
    virtual std::string getTypeName() {return "string";}
    virtual std::string getStringValue() {return value;}
    virtual bool boolValue() {return !value.empty();}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_STRING && ((TuiString*)other)->value == value;}
    
    TuiString(const std::string& value_, TuiTable* parent_ = nullptr) : TuiRef(parent_) {value = value_;}
    virtual ~TuiString() {
        if(allowAsVariableName)
        {
            for(TuiVarToken& token : vars)
            {
                if(token.expression)
                {
                    delete token.expression; //todo will crash if assign() has been called on a string that has expressions
                }
            }
        }
    };
    
    virtual TuiString* copy()
    {
        return new TuiString(value, parent);
    }
    virtual void assign(TuiRef* other) { //todo will crash if assign() has been called on a string that has expressions
        value = ((TuiString*)other)->value;
        allowAsVariableName = ((TuiString*)other)->allowAsVariableName;
        isValidFunctionString = ((TuiString*)other)->isValidFunctionString;
        vars = ((TuiString*)other)->vars;
    };
    
    
    /*static TuiString* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo, uint32_t variableLoadType, TuiTokenMap* tokenMap = nullptr) {
        const char* s = str;
        
        TuiString* mjString = new TuiString("", parent);
        
        bool singleQuote = false;
        bool doubleQuote = false;
        bool escaped = false;
        bool foundSpace = false;
        
        if(variableLoadType == Tui_variable_load_type_ignore)
        {
            mjString->allowAsVariableName = false;
        }
        
        std::string currentVarName = "";
        const char* currentVarNameS = s;
        
        for(;; s++)
        {
            if(*s == '\0')
            {
                break;
            }
            else if(*s == '\'')
            {
                if(!escaped && !doubleQuote)
                {
                    if(singleQuote)
                    {
                        singleQuote = false;
                        s++;
                        break;
                    }
                    else
                    {
                        if(mjString->value.empty())
                        {
                            singleQuote = true;
                            mjString->allowAsVariableName = false;
                        }
                    }
                }
                else
                {
                    mjString->value += *s;
                }
            }
            else if(*s == '"')
            {
                if(!escaped && !singleQuote)
                {
                    if(doubleQuote)
                    {
                        doubleQuote = false;
                        s++;
                        break;
                    }
                    else
                    {
                        if(mjString->value.empty())
                        {
                            doubleQuote = true;
                            mjString->allowAsVariableName = false;
                        }
                    }
                }
                else
                {
                    mjString->value += *s;
                }
            }
            else if(*s == '\\')
            {
                if(!escaped)
                {
                    escaped = true;
                }
                else
                {
                    escaped = false;
                    mjString->value += *s;
                }
            }
            else if(*s == '.' && mjString->allowAsVariableName && !escaped)
            {
                if(*(s+1) == '.')
                {
                    s++;
                    mjString->vars.resize(mjString->vars.size() + 1);
                    mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_parent;
                    mjString->value += "..";
                    while(*(s+1) == '.')
                    {
                        s++;
                        mjString->vars.resize(mjString->vars.size() + 1);
                        mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_parent;
                        mjString->value += ".";
                    }
                }
                else
                {
                    mjString->vars.resize(mjString->vars.size() + 1);
                    mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                    mjString->vars[mjString->vars.size() - 1].varName = currentVarName;
                    currentVarName = "";
                    mjString->value += *s;
                }
            }
            else if(*s == '[' && mjString->allowAsVariableName && !escaped)
            {
                mjString->vars.resize(mjString->vars.size() + 1);
                mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                mjString->vars[mjString->vars.size() - 1].varName = currentVarName;
                currentVarName = "";
                
                mjString->value += *s;
                s++;
                s = tuiSkipToNextChar(s, debugInfo);
                
                
                if(variableLoadType == Tui_variable_load_type_runExpressions)
                {
                    std::map<uint32_t, TuiRef*> locals;
                    TuiRef* expressionResult = TuiRef::recursivelyLoadValue(s,
                                                                    endptr,
                                                                    nullptr,
                                                                    nullptr,
                                                                    (TuiTable*)parent, tokenMap, &locals,
                                                                    debugInfo,
                                                                    Tui_operator_level_default,
                                                                    true);
                    s = tuiSkipToNextChar(*endptr, debugInfo);
                    s++; //']'
                    
                    if(expressionResult->type() == Tui_ref_type_NUMBER)
                    {
                        mjString->vars.resize(mjString->vars.size() + 1);
                        mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_arrayIndex;
                        mjString->vars[mjString->vars.size() - 1].arrayOrSetIndex = ((TuiNumber*)expressionResult)->value;
                        mjString->value += Tui::string_format("%d]",(int)(((TuiNumber*)expressionResult)->value));
                    }
                    else if(expressionResult->type() == Tui_ref_type_STRING)
                    {
                        mjString->vars.resize(mjString->vars.size() + 1);
                        mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                        mjString->vars[mjString->vars.size() - 1].varName = ((TuiString*)expressionResult)->value;
                        mjString->value += (((TuiString*)expressionResult)->value + "]");
                    }
                    expressionResult->release();
                    currentVarName = "";
                    currentVarNameS = s;
                    
                }
                else if(tokenMap)
                {
                    TuiExpression* expression = new TuiExpression();
                    
                    mjString->vars.resize(mjString->vars.size() + 1);
                    mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_expression;
                    mjString->vars[mjString->vars.size() - 1].expression = expression;
                
                    TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    if(*s == ']')
                    {
                        s++;
                        s = tuiSkipToNextChar(s, debugInfo, true);
                    }
                }
                
            }
            else if(*s == '(')
            {
                if(variableLoadType == Tui_variable_load_type_runExpressions)
                {
                    std::map<uint32_t, TuiRef*> locals;
                    TuiRef* expressionResult = TuiRef::recursivelyLoadValue(currentVarNameS,
                                                                            endptr,
                                                                            nullptr,
                                                                            nullptr,
                                                                            parent, tokenMap, &locals,
                                                                            debugInfo,
                                                                            Tui_operator_level_default,
                                                                            true);
                    if(expressionResult->type() == Tui_ref_type_NUMBER)
                    {
                        mjString->vars.resize(mjString->vars.size() + 1);
                        mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_arrayIndex;
                        mjString->vars[mjString->vars.size() - 1].arrayOrSetIndex = ((TuiNumber*)expressionResult)->value;
                        mjString->value += Tui::string_format("%d]",(int)(((TuiNumber*)expressionResult)->value));
                    }
                    else if(expressionResult->type() == Tui_ref_type_STRING)
                    {
                        mjString->vars.resize(mjString->vars.size() + 1);
                        mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                        mjString->vars[mjString->vars.size() - 1].varName = ((TuiString*)expressionResult)->value;
                        mjString->value += (((TuiString*)expressionResult)->value + "]");
                    }
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    
                    currentVarName = "";
                    currentVarNameS = s;
                    
                    **mjString->value += *s;
                    while(1)
                    {
                        s++;
                        s = tuiSkipToNextChar(s, debugInfo);
                        if(*s == ')')
                        {
                            break;
                        }
                        
                        std::map<uint32_t, TuiRef*> locals;
                        TuiRef* expressionResult = TuiRef::recursivelyLoadValue(s,
                                                                                endptr,
                                                                                nullptr,
                                                                                nullptr,
                                                                                parent, tokenMap, &locals,
                                                                                debugInfo,
                                                                                Tui_operator_level_default,
                                                                                true);
                        s = tuiSkipToNextChar(*endptr, debugInfo);
                        if(*s == ')')
                        {
                            break;
                        }
                        s++; //']'
                        
                        if(expressionResult->type() == Tui_ref_type_NUMBER)
                        {
                            mjString->vars.resize(mjString->vars.size() + 1);
                            mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_arrayIndex;
                            mjString->vars[mjString->vars.size() - 1].arrayOrSetIndex = ((TuiNumber*)expressionResult)->value;
                            mjString->value += Tui::string_format("%d]",(int)(((TuiNumber*)expressionResult)->value));
                        }
                        else if(expressionResult->type() == Tui_ref_type_STRING)
                        {
                            mjString->vars.resize(mjString->vars.size() + 1);
                            mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                            mjString->vars[mjString->vars.size() - 1].varName = ((TuiString*)expressionResult)->value;
                            mjString->value += (((TuiString*)expressionResult)->value + "]");
                        }
                        expressionResult->release();
                        currentVarName = "";
                    }**
                    
                }
                else
                {
                    if(!escaped && mjString->allowAsVariableName)
                    {
                        mjString->isValidFunctionString = true;
                        break;
                    }
                    else
                    {
                        mjString->value += *s;
                    }
                }
                
                **else if(tokenMap)
                {
                    TuiExpression* expression = new TuiExpression();
                    
                    mjString->vars.resize(mjString->vars.size() + 1);
                    mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_expression;
                    mjString->vars[mjString->vars.size() - 1].expression = expression;
                
                    TuiFunction::recursivelySerializeExpression(s, endptr, expression, parent, tokenMap, debugInfo, Tui_operator_level_default);
                    
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    if(*s == ']')
                    {
                        s++;
                        s = tuiSkipToNextChar(s, debugInfo, true);
                    }
                }**
                **if(variableLoadType == Tui_variable_load_type_runExpressions)
                {
                    
                   // TuiRef* newKeyRef = TuiRef::loadVariableIfAvailable(originalString, nullptr, s, endptr, parentTable, tokenMap, locals, debugInfo);
                }
                else
                {
                    if(!escaped && mjString->allowAsVariableName)
                    {
                        mjString->isValidFunctionString = true;
                        break;
                    }
                    else
                    {
                        mjString->value += *s;
                    }
                }**
            }
            else if(!escaped && !singleQuote && !doubleQuote &&
                    (isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || *s == ']' || TuiExpressionOperatorsSet.count(*s) != 0))
            {
                if(*s == '\n')
                {
                    break;
                }
                if(!isspace(*s))
                {
                    break;
                }
                else
                {
                    foundSpace = true;
                }
            }
            else if(!escaped && !singleQuote && !doubleQuote &&
                    s == str && //start of string
                    ((*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2)) ||
                     (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3))))
            {
                break;
            }
            else
            {
                if(foundSpace)
                {
                    break;
                }
                mjString->value += *s;
                if(mjString->allowAsVariableName)
                {
                    currentVarName += *s;
                }
                escaped = false;
            }
            
        }
        
        if(mjString->allowAsVariableName)
        {
            if(mjString->value.empty())
            {
                mjString->allowAsVariableName = false;
            }
            else if(!currentVarName.empty())
            {
                mjString->vars.resize(mjString->vars.size() + 1);
                mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                mjString->vars[mjString->vars.size() - 1].varName = currentVarName;
            }
        }
        
        
        s = tuiSkipToNextChar(s, debugInfo, true);
        *endptr = (char*)s;
        
        
        
        return mjString;
    }*/
    
    /*virtual uint64_t generateHash() {
        uint32_t hash;
        MurmurHash3_x86_32(value.data(), value.size(), 123, &hash);
        return hash;
    }*/

private:
    
private:
};

#endif
