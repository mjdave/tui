
#ifndef TuiString_h
#define TuiString_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"

class TuiString : public TuiRef {
public: //members
    std::string value;
    bool allowAsVariableName = true; // this is set to false if it finds a quoted string when loaded via readable string
    bool isValidFunctionString = false; // optimization
    std::vector<std::string> varNames; // optimization, finds look ups of sub-tables on load

public://functions
    
    virtual uint8_t type() { return TuiREF_TYPE_STRING; }
    virtual std::string getTypeName() {return "string";}
    virtual std::string getStringValue() {return value;}
    virtual bool boolValue() {return !value.empty();}
    
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
        varNames = other->varNames;
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
                    mjString->varNames.push_back(".");
                    mjString->value += "..";
                    while(*(s+1) == '.')
                    {
                        s++;
                        mjString->varNames.push_back(".");
                        mjString->value += ".";
                    }
                }
                else
                {
                    mjString->varNames.push_back(currentVarName);
                    currentVarName = "";
                    mjString->value += *s;
                }
            }
            else if(*s == '(')
            {
                if(mjString->allowAsVariableName && !escaped && !singleQuote && !doubleQuote)
                {
                    mjString->isValidFunctionString = true;
                    break;
                }
            }
            else if(!escaped && !singleQuote && !doubleQuote &&
                    (isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || TuiExpressionOperatorsSet.count(*s) != 0))
            {
                if(*s == '\n')
                {
                    debugInfo->lineNumber++;
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
                mjString->varNames.push_back(currentVarName);
            }
        }
        
        
        s = skipToNextChar(s, debugInfo, true);
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
