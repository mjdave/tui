#ifndef TuiNumber_h
#define TuiNumber_h

#include <stdio.h>
#include <string>
#include "glm.hpp"
#include "TuiLog.h"

#include "TuiRef.h"

using namespace glm;

class TuiNumber : public TuiRef {
public: //members
    double value;

public://functions
    TuiNumber(double value_) : TuiRef() {value = value_;}
    virtual ~TuiNumber() {};
    
    virtual TuiRef* copy()
    {
        return new TuiNumber(value);
    }
    virtual void assign(TuiRef* other) {
        value = ((TuiNumber*)other)->value;
    };
    
    
    virtual uint8_t type() { return Tui_ref_type_NUMBER; }
    virtual std::string getTypeName() {return "number";}
    virtual std::string getStringValue() {
        if(value == floor(value))
        {
            return Tui::string_format("%.0f", value);
        }
        return Tui::string_format("%s", Tui::doubleToString(value).c_str());
    }
    virtual bool boolValue() {return value != 0;}
    virtual double getNumberValue() {return value;}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_NUMBER && ((TuiNumber*)other)->value == value;}

private:
    
private:
};

class TuiBool;
extern TuiBool* TUI_TRUE;
extern TuiBool* TUI_FALSE;

#define TUI_BOOL(__boolValue__) ((__boolValue__) ? TUI_TRUE : TUI_FALSE)

class TuiBool : public TuiRef {
public: //members
    bool value;

public://functions
    TuiBool(bool value_) : TuiRef() {value = value_;} //do not use TuiBool directly, use TUI_TRUE and TUI_FALSE
    
    virtual ~TuiBool() {};
    
    virtual TuiRef* copy()
    {
        return this;
    }
    virtual void assign(TuiRef* other) {
        TuiError("assign not supported for bool type");
    };
    virtual void release() {}
    virtual TuiRef* retain() { return this;}
    
    static TuiBool* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 't' && *(s + 1) == 'r' && *(s + 2) == 'u' && *(s + 3) == 'e' )
        {
            *endptr = (char*)(s + 4);
            return TUI_TRUE;
        }
        if(*s == 'f' && *(s + 1) == 'a' && *(s + 2) == 'l' && *(s + 3) == 's' && *(s + 4) == 'e' )
        {
            *endptr = (char*)(s + 5);
            return TUI_FALSE;
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return Tui_ref_type_BOOL; }
    virtual std::string getTypeName() {return "bool";}
    virtual std::string getStringValue() {
        return (value ? "true" : "false");
    }
    virtual bool boolValue() {return value;}
    virtual double getNumberValue() {return value;}
    virtual bool isEqual(TuiRef* other)
    {
        if(!other)
        {
            return !value;
        }
        return other == this;
    }

private:
    
private:
};


class TuiVec2 : public TuiRef {
public: //members
    dvec2 value;

public://functions
    TuiVec2(dvec2 value_) : TuiRef() {value = value_;}
    virtual ~TuiVec2() {};
    virtual TuiRef* copy()
    {
        return new TuiVec2(value);
    }
    virtual void assign(TuiRef* other) {
        value = ((TuiVec2*)other)->value;
    };
    
    static TuiVec2* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '2' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[2] = {0.0,0.0};
            for(int i = 0; i < 2; i++)
            {
                TuiRef* loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, (TuiTable*)parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "uninitialized or non-number value in vec2 constructor:%c", *s);
                    if(loadedValue)
                    {
                        loadedValue->release();
                    }
                    return nullptr;
                }
                
                values[i] = ((TuiNumber*)loadedValue)->value;
                loadedValue->release();
                
                if(*s == ',')
                {
                    s++;
                    s = tuiSkipToNextChar(s, debugInfo);
                }
                else if(*s == ')' || *s == '\0')
                {
                    s++;
                    break;
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "found bad char when expecting ',' within vec2:%c", *s);
                    return nullptr;
                }
            }
            
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return new TuiVec2(dvec2(values[0], values[1]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return Tui_ref_type_VEC2; }
    virtual std::string getTypeName() {return "vec2";}
    virtual std::string getStringValue() {
        return Tui::string_format("vec2(%s,%s)", Tui::doubleToString(value.x).c_str(), Tui::doubleToString(value.y).c_str());
    }
    virtual bool boolValue() {return true;}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_VEC2 && ((TuiVec2*)other)->value == value;}

private:
    
private:
};


class TuiVec3 : public TuiRef {
public: //members
    dvec3 value;

public://functions
    TuiVec3(dvec3 value_) : TuiRef() {value = value_;}
    virtual ~TuiVec3() {};
    virtual TuiRef* copy()
    {
        return new TuiVec3(value);
    }
    virtual void assign(TuiRef* other) {
        value = ((TuiVec3*)other)->value;
    };
    
    static TuiVec3* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '3' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[3] = {0.0,0.0,0.0};
            for(int i = 0; i < 3; i++)
            {
                TuiRef* loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, (TuiTable*)parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "uninitialized or non-number value in vec3 constructor:%c", *s);
                    if(loadedValue)
                    {
                        loadedValue->release();
                    }
                    return nullptr;
                }
                
                values[i] = ((TuiNumber*)loadedValue)->value;
                loadedValue->release();
                
                if(*s == ',')
                {
                    s++;
                    s = tuiSkipToNextChar(s, debugInfo);
                }
                else if(*s == ')' || *s == '\0')
                {
                    s++;
                    s = tuiSkipToNextChar(s, debugInfo, true);
                    break;
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "found bad char when expecting ',' within vec3:%c", *s);
                    return nullptr;
                }
            }
            
            *endptr = (char*)s;
            
            return new TuiVec3(dvec3(values[0], values[1], values[2]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return Tui_ref_type_VEC3; }
    virtual std::string getTypeName() {return "vec3";}
    virtual std::string getStringValue() {
        return Tui::string_format("vec3(%s,%s,%s)", Tui::doubleToString(value.x).c_str(), Tui::doubleToString(value.y).c_str(), Tui::doubleToString(value.z).c_str());
    }
    virtual bool boolValue() {return true;}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_VEC3 && ((TuiVec3*)other)->value == value;}

private:
    
private:
};


class TuiVec4 : public TuiRef {
public: //members
    dvec4 value;

public://functions
    TuiVec4(dvec4 value_) : TuiRef() {value = value_;}
    virtual ~TuiVec4() {};
    virtual TuiRef* copy()
    {
        return new TuiVec4(value);
    }
    virtual void assign(TuiRef* other) {
        value = ((TuiVec4*)other)->value;
    };
    
    static TuiVec4* initWithHumanReadableString(const char* str, char** endptr, TuiTable* parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '4' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[4] = {0.0,0.0,0.0,0.0};
            for(int i = 0; i < 4; i++)
            {
                TuiRef* loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, (TuiTable*)parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "uninitialized or non-number value in vec4 constructor:%c", *s);
                    if(loadedValue)
                    {
                        loadedValue->release();
                    }
                    return nullptr;
                }
                
                values[i] = ((TuiNumber*)loadedValue)->value;
                loadedValue->release();
                
                if(*s == ',')
                {
                    s++;
                    s = tuiSkipToNextChar(s, debugInfo);
                }
                else if(*s == ')' || *s == '\0')
                {
                    s++;
                    break;
                }
                else
                {
                    TuiParseError(debugInfo->fileName.c_str(), debugInfo->lineNumber, "found bad char when expecting ',' within vec4:%c", *s);
                    return nullptr;
                }
            }
            
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return new TuiVec4(dvec4(values[0], values[1], values[2],  values[3]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() { return Tui_ref_type_VEC4; }
    virtual std::string getTypeName() {return "vec4";}
    virtual std::string getStringValue() {
        return Tui::string_format("vec4(%s,%s,%s,%s)", Tui::doubleToString(value.x).c_str(), Tui::doubleToString(value.y).c_str(), Tui::doubleToString(value.z).c_str(), Tui::doubleToString(value.w).c_str());
    }
    virtual bool boolValue() {return true;}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_VEC4 && ((TuiVec4*)other)->value == value;}

private:
    
private:
};

#endif
