
#ifndef TuiString_h
#define TuiString_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"

enum {
    Tui_var_token_type_undefined = 0,
    Tui_var_token_type_string,
    Tui_var_token_type_parent,
    Tui_var_token_type_arrayIndex,
    Tui_var_token_type_setIndex
};

struct TuiVarToken {
    uint32_t type = Tui_var_token_type_undefined;
    uint32_t arrayOrSetIndex;
    std::string varName;
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
    
    TuiString(const std::string& value_, TuiRef* parent_ = nullptr) : TuiRef(parent_) {value = value_;}
    virtual ~TuiString() {};
    
    virtual TuiString* copy()
    {
        return new TuiString(value, parent);
    }
    virtual void assign(TuiString* other) {
        value = other->value;
        allowAsVariableName = other->allowAsVariableName;
        isValidFunctionString = other->isValidFunctionString;
        vars = other->vars;
    };
    
    static TuiString* initWithHumanReadableString(const char* str, char** endptr, TuiRef* parent, TuiDebugInfo* debugInfo) {
        const char* s = str;
        
        TuiString* mjString = new TuiString("", parent);
        
        bool singleQuote = false;
        bool doubleQuote = false;
        bool escaped = false;
        bool foundSpace = false;
        
        std::string currentVarName = "";
        
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
                mjString->value += *s;
                s++;
                s = tuiSkipToNextChar(s, debugInfo);
                if(isdigit(*s))
                {
                    uint32_t arrayIndex = (uint32_t)strtoul(s, endptr, 10);
                    s = tuiSkipToNextChar(*endptr, debugInfo, true);
                    if(*s != ']')
                    {
                        TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected ']''");
                        return nullptr;
                    }
                    
                    mjString->value += Tui::string_format("%d]",arrayIndex);
                    
                    if(!currentVarName.empty())
                    {
                        mjString->vars.resize(mjString->vars.size() + 1);
                        mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_string;
                        mjString->vars[mjString->vars.size() - 1].varName = currentVarName;
                        currentVarName = "";
                    }
                    
                    mjString->vars.resize(mjString->vars.size() + 1);
                    mjString->vars[mjString->vars.size() - 1].type = Tui_var_token_type_arrayIndex;
                    mjString->vars[mjString->vars.size() - 1].arrayOrSetIndex = arrayIndex;
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "Expected number after '['");
                }

                /*mjString->varNames.resize(mjString->varNames.size() + 1);
                mjString->varNames[mjString->varNames.size() - 1].varName = currentVarName;
                currentVarName = "";
                mjString->value += *s;*/
            }
            else if(*s == '(')
            {
                if(!escaped && !singleQuote && !doubleQuote)
                {
                    if(mjString->allowAsVariableName)
                    {
                        mjString->isValidFunctionString = true;
                        break;
                    }
                }
                else
                {
                    mjString->value += *s;
                }
            }
            else if(!escaped && !singleQuote && !doubleQuote &&
                    (isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || TuiExpressionOperatorsSet.count(*s) != 0))
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
                    ((*s == 'o' && *(s + 1) == 'r' && checkSymbolNameComplete(s + 2)) ||  (*s == 'a' && *(s + 1) == 'n' && *(s + 2) == 'd' && checkSymbolNameComplete(s + 3))))
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
    }
    
    /*virtual uint64_t generateHash() {
        uint32_t hash;
        MurmurHash3_x86_32(value.data(), value.size(), 123, &hash);
        return hash;
    }*/

private:
    
private:
};

#endif
