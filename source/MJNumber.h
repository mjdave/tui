#ifndef MJNumber_h
#define MJNumber_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
#include "MJLog.h"

#include "MJRef.h"

using namespace glm;

class MJNumber : public MJRef {
public: //members
    double value;

public://functions
    MJNumber(double value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJNumber() {};
    
    virtual MJNumber* copy()
    {
        return new MJNumber(value, parent);
    }
    
    static MJNumber* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = str;
        
        double value = strtod(s, endptr);
        
        s = skipToNextChar(*endptr, debugInfo, true);
        *endptr = (char*)s;
        
        MJNumber* number = new MJNumber(value, parent);
        return number;
    }
    
    //virtual uint64_t generateHash() {return *((uint64_t*)&value);}
    
    virtual uint8_t type() { return MJREF_TYPE_NUMBER; }
    virtual std::string getTypeName() {return "number";}
    virtual std::string getStringValue() {
        if(value == floor(value))
        {
            return string_format("%.0f", value);
        }
        return string_format("%s", doubleToString(value).c_str());
    }
    virtual bool boolValue() {return value != 0;}

private:
    
private:
};

class MJBool : public MJRef {
public: //members
    bool value;

public://functions
    MJBool(bool value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJBool() {};
    
    virtual MJNumber* copy()
    {
        return new MJNumber(value, parent);
    }
    
    static MJBool* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = skipToNextChar(str, debugInfo);
        
        if(*s == 't' && *(s + 1) == 'r' && *(s + 2) == 'u' && *(s + 3) == 'e' )
        {
            *endptr = (char*)(s + 4);
            MJBool* number = new MJBool(true, parent);
            return number;
        }
        if(*s == 'f' && *(s + 1) == 'a' && *(s + 2) == 'l' && *(s + 3) == 's' && *(s + 4) == 'e' )
        {
            *endptr = (char*)(s + 5);
            MJBool* number = new MJBool(false, parent);
            return number;
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return MJREF_TYPE_BOOL; }
    virtual std::string getTypeName() {return "bool";}
    virtual std::string getStringValue() {
        return (value ? "true" : "false");
    }
    virtual bool boolValue() {return value;}

private:
    
private:
};


class MJVec2 : public MJRef {
public: //members
    dvec2 value;

public://functions
    MJVec2(dvec2 value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJVec2() {};
    virtual MJVec2* copy()
    {
        return new MJVec2(value, parent);
    }
    
    static MJVec2* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = skipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '2' && *(s + 4) == '(')
        {
            s+= 5;
            
            double values[2] = {0.0,0.0};
            for(int i = 0; i < 2; i++)
            {
                values[i] = strtod(s, endptr);
                s = skipToNextChar(*endptr, debugInfo);
                if(*s == ',')
                {
                    s++;
                }
                else if(*s == ')')
                {
                    s++;
                    break;
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "found bad char when expecting ',' within vec2:%c", *s);
                    break;
                }
            }
            
            s = skipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return new MJVec2(dvec2(values[0], values[1]), parent);
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return MJREF_TYPE_VEC2; }
    virtual std::string getTypeName() {return "vec2";}
    virtual std::string getStringValue() {
        return string_format("vec2(%s,%s)", doubleToString(value.x).c_str(), doubleToString(value.y).c_str());
    }
    virtual bool boolValue() {return true;}

private:
    
private:
};


class MJVec3 : public MJRef {
public: //members
    dvec3 value;

public://functions
    MJVec3(dvec3 value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJVec3() {};
    virtual MJVec3* copy()
    {
        return new MJVec3(value, parent);
    }
    
    static MJVec3* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = skipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '3' && *(s + 4) == '(')
        {
            s+= 5;
            
            double values[3] = {0.0,0.0,0.0};
            for(int i = 0; i < 3; i++)
            {
                values[i] = strtod(s, endptr);
                s = skipToNextChar(*endptr, debugInfo);
                if(*s == ',')
                {
                    s++;
                }
                else if(*s == ')')
                {
                    s++;
                    break;
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "found bad char when expecting ',' within vec3:%c", *s);
                    break;
                }
            }
            
            s = skipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return new MJVec3(dvec3(values[0], values[1], values[2]), parent);
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return MJREF_TYPE_VEC3; }
    virtual std::string getTypeName() {return "vec3";}
    virtual std::string getStringValue() {
        return string_format("vec3(%s,%s,%s)", doubleToString(value.x).c_str(), doubleToString(value.y).c_str(), doubleToString(value.z).c_str());
    }
    virtual bool boolValue() {return true;}

private:
    
private:
};


class MJVec4 : public MJRef {
public: //members
    dvec4 value;

public://functions
    MJVec4(dvec4 value_, MJRef* parent_ = nullptr) : MJRef(parent_) {value = value_;}
    virtual ~MJVec4() {};
    virtual MJVec4* copy()
    {
        return new MJVec4(value, parent);
    }
    
    static MJVec4* initWithHumanReadableString(const char* str, char** endptr, MJRef* parent, MJDebugInfo* debugInfo) {
        const char* s = skipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '4' && *(s + 4) == '(')
        {
            s+= 5;
            
            double values[4] = {0.0,0.0,0.0,0.0};
            for(int i = 0; i < 4; i++)
            {
                values[i] = strtod(s, endptr);
                s = skipToNextChar(*endptr, debugInfo);
                if(*s == ',')
                {
                    s++;
                }
                else if(*s == ')')
                {
                    s++;
                    break;
                }
                else
                {
                    MJSError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "found bad char when expecting ',' within vec4:%c", *s);
                    break;
                }
            }
            
            s = skipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return new MJVec4(dvec4(values[0], values[1], values[2],  values[3]), parent);
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return MJREF_TYPE_VEC4; }
    virtual std::string getTypeName() {return "vec4";}
    virtual std::string getStringValue() {
        return string_format("vec4(%s,%s,%s,%s)", doubleToString(value.x).c_str(), doubleToString(value.y).c_str(), doubleToString(value.z).c_str(), doubleToString(value.w).c_str());
    }
    virtual bool boolValue() {return true;}

private:
    
private:
};

#endif
