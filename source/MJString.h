
#ifndef MJString_h
#define MJString_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
#include "MJLog.h"

#include "MJRef.h"

class MJString : public MJRef {
public: //members
    std::string value;
    bool allowAsVariableName = true; // this is set to false if it finds a quoted string when loaded via readable string
    bool isValidFunctionString = false; // optimization
    std::vector<std::string> varNames; // optimization, finds look ups of sub-tables on load

public://functions
    
    virtual uint8_t type() { return MJREF_TYPE_STRING; }
    virtual std::string getTypeName() {return "string";}
    virtual std::string getStringValue() {return value;}
    virtual bool boolValue() {return !value.empty();}
    
    MJString(const std::string& value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJString() {};
    
    virtual MJString* copy()
    {
        return new MJString(value, parent);
    }
    
    static MJString* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = str;
        
        MJString* mjString = new MJString("", parent);
        
        bool singleQuote = false;
        bool doubleQuote = false;
        bool escaped = false;
        
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
                    (isspace(*s) || *s == ',' || *s == '\n' || *s == ')' || MJExpressionOperatorsSet.count(*s) != 0))
            {
                if(*s == '\n')
                {
                    debugInfo->lineNumber++;
                }
                break;
            }
            else
            {
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
