
#ifndef MJString_h
#define MJString_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
//#include "MurmurHash3.h"
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
    virtual std::string getStringValue() {return "\"" + value + "\"";}
    virtual bool allowExpressions() {return false;}
    
    MJString(const std::string& value_) {value = value_;}
    virtual ~MJString() {};
    
    virtual MJString* copy()
    {
        return new MJString(value);
    }
    
    static MJString* initWithHumanReadableString(const char* str, char** endptr, MJDebugInfo* debugInfo) {
        const char* s = str;
        
        MJString* mjString = new MJString("");
        
        bool singleQuote = false;
        bool doubleQuote = false;
        bool escaped = false;
        
        std::string currentVarName = "";
        
        for(;; s++)
        {
            if(*s == '\'')
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
                            continue;
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
                            continue;
                        }
                    }
                }
            }
            else if(*s == '\\')
            {
                if(!escaped)
                {
                    escaped = true;
                    continue;
                }
            }
            else if(*s == '.')
            {
                if(mjString->allowAsVariableName && !escaped && !singleQuote && !doubleQuote)
                {
                    mjString->varNames.push_back(currentVarName);
                    currentVarName = "";
                    mjString->value += *s;
                    continue;
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
            
            mjString->value += *s;
            if(mjString->allowAsVariableName)
            {
                currentVarName += *s;
            }
            escaped = false;
        }
        
        if(mjString->allowAsVariableName)
        {
            if(mjString->value.empty())
            {
                mjString->allowAsVariableName = false;
            }
            else
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

#endif /* MJString_h */
