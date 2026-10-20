#ifndef TuiNumber_h
#define TuiNumber_h

#include <stdio.h>
#include <string>
#include <cstring>
#include "glm.hpp"
#include "TuiLog.h"
#include "TuiTypes.h"

#include "TuiRef.h"

using namespace glm;

class TuiNumber8;
class TuiNumber16;
class TuiNumber32;
class TuiNumber64;


namespace tui {

    inline tui::number newNumber(const double& value)
    {
        return tui::create<TuiNumber>(value);
    }
    inline tui::number8 newNumber8(uint8_t value)
    {
        return tui::create<TuiNumber8>(value);
    }
    inline tui::number16 newNumber16(uint16_t value)
    {
        return tui::create<TuiNumber16>(value);
    }
    inline tui::number32 newNumber32(uint32_t value)
    {
        return tui::create<TuiNumber32>(value);
    }
    inline tui::number64 newNumber64(uint64_t value)
    {
        return tui::create<TuiNumber64>(value);
    }
    inline tui::boolean newBool(bool value)
    {
        return tui::create<TuiBool>(value);
    }
    inline tui::vec2 newVec2(const dvec2& value)
    {
        return tui::create<TuiVec2>(value);
    }
    inline tui::vec3 newVec3(const dvec3& value)
    {
        return tui::create<TuiVec3>(value);
    }
    inline tui::vec4 newVec4(const dvec4& value)
    {
        return tui::create<TuiVec4>(value);
    }
    inline tui::mat3 newMat3(const dmat3& value)
    {
        return tui::create<TuiMat3>(value);
    }
}


class TuiNumber : public TuiRef {
public: //members
    double value;

public://functions
    TuiNumber(double value_) : TuiRef() {value = value_;}
    virtual ~TuiNumber() {};
    
    virtual tui::ref copy() override
    {
        return tui::create<TuiNumber>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiNumber>(other)->value;
    }
    
    
    virtual uint8_t type() override { return Tui_ref_type_NUMBER; }
    virtual std::string getTypeName() override {return "number";}
    virtual std::string getStringValue() override {
        if(value == floor(value))
        {
            return tui::string_format("%.0f", value);
        }
        return tui::string_format("%s", tui::doubleToString(value).c_str());
    }
    virtual bool boolValue() override {return true;}
    virtual double getNumberValue() override {return value;}
    virtual bool isEqual(const tui::ref& other) override {return other && other->type() == Tui_ref_type_NUMBER && tui::cast<TuiNumber>(other)->value == value;}
    
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 9);
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER;
        memcpy(&buffer[(*currentOffset)], &value, 8);
        *currentOffset += 8;
    }
};


//these integer types are only added for binary serializaton so far, they cannot yet be used from within tui scripts

class TuiNumber8 : public TuiRef {
public: //members
    uint8_t value;

public://functions
    TuiNumber8(uint8_t value_) : TuiRef() {value = value_;}
    virtual ~TuiNumber8() {};
    
    virtual tui::ref copy() override
    {
        return tui::create<TuiNumber8>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiNumber8>(other)->value;
    }
    
    
    virtual uint8_t type() override { return Tui_ref_type_NUMBER_8; }
    virtual std::string getTypeName() override {return "number8";}
    virtual std::string getStringValue() override {
        return tui::string_format("%u", value);
    }
    virtual bool boolValue() override {return true;}
    virtual double getNumberValue() override {return value;}
    virtual bool isEqual(const tui::ref& other) override {return other && (
      (other->type() == Tui_ref_type_NUMBER_8 && tui::cast<TuiNumber8>(other)->value == value))
        ;}
    
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 2);
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_8;
        memcpy(&buffer[(*currentOffset)], &value, 1);
        *currentOffset += 1;
    }
};

class TuiNumber16 : public TuiRef {
public: //members
    uint16_t value;

public://functions
    TuiNumber16(uint16_t value_) : TuiRef() {value = value_;}
    virtual ~TuiNumber16() {};
    
    virtual tui::ref copy() override
    {
        return tui::create<TuiNumber16>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiNumber16>(other)->value;
    }
    
    
    virtual uint8_t type() override { return Tui_ref_type_NUMBER_16; }
    virtual std::string getTypeName() override {return "number16";}
    virtual std::string getStringValue() override {
        return tui::string_format("%lu", value);
    }
    virtual bool boolValue() override {return true;}
    virtual double getNumberValue() override {return value;}
    virtual bool isEqual(const tui::ref& other) override {return other && (
      (other->type() == Tui_ref_type_NUMBER_16 && tui::cast<TuiNumber16>(other)->value == value))
        ;}
    
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 3);
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_16;
        memcpy(&buffer[(*currentOffset)], &value, 2);
        *currentOffset += 2;
    }
};


class TuiNumber32 : public TuiRef {
public: //members
    uint32_t value;

public://functions
    TuiNumber32(uint32_t value_) : TuiRef() {value = value_;}
    virtual ~TuiNumber32() {};
    
    virtual tui::ref copy() override
    {
        return tui::create<TuiNumber32>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiNumber32>(other)->value;
    }
    
    
    virtual uint8_t type() override { return Tui_ref_type_NUMBER_32; }
    virtual std::string getTypeName() override {return "number32";}
    virtual std::string getStringValue() override {
        return tui::string_format("%lu", value);
    }
    virtual bool boolValue() override {return true;}
    virtual double getNumberValue() override {return value;}
    virtual bool isEqual(const tui::ref& other) override {return other && (
      (other->type() == Tui_ref_type_NUMBER_32 && tui::cast<TuiNumber32>(other)->value == value))
        ;}
    
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 5);
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_32;
        memcpy(&buffer[(*currentOffset)], &value, 4);
        *currentOffset += 4;
    }
};


class TuiNumber64 : public TuiRef {
public: //members
    uint64_t value;

public://functions
    TuiNumber64(uint64_t value_) : TuiRef() {value = value_;}
    virtual ~TuiNumber64() {};
    
    virtual tui::ref copy() override
    {
        return tui::create<TuiNumber64>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiNumber64>(other)->value;
    }
    
    virtual uint8_t type() override { return Tui_ref_type_NUMBER_64; }
    virtual std::string getTypeName() override {return "number64";}
    virtual std::string getStringValue() override {
        return tui::string_format("%llu", value);
    }
    virtual bool boolValue() override {return true;}
    virtual double getNumberValue() override {return value;}
    virtual bool isEqual(const tui::ref& other) override {return other && (
      (other->type() == Tui_ref_type_NUMBER_64 && tui::cast<TuiNumber64>(other)->value == value))
        ;}
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 9);
        buffer[(*currentOffset)++] = Tui_binary_type_NUMBER_64;
        memcpy(&buffer[(*currentOffset)], &value, 8);
        *currentOffset += 8;
    }
};

class TuiBool;
extern tui::boolean TUI_TRUE;
extern tui::boolean TUI_FALSE;

#define TUI_BOOL(__boolValue__) ((__boolValue__) ? TUI_TRUE : TUI_FALSE)

class TuiBool : public TuiRef {
public: //members
    bool value;

public://functions
    TuiBool(bool value_) : TuiRef() {value = value_;} //do not use TuiBool directly, use TUI_TRUE and TUI_FALSE
    
    virtual ~TuiBool() {};
    
    virtual tui::ref copy() override
    {
        return createFromThis();
    }
    virtual void assign(const tui::ref& other) override {
        TuiError("assign not supported for bool type");
    }
    
    static tui::boolean initWithHumanReadableString(const char* str, char** endptr, const tui::table& parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 't' && *(s + 1) == 'r' && *(s + 2) == 'u' && *(s + 3) == 'e' && checkSymbolNameComplete(s + 4))
        {
            *endptr = (char*)(s + 4);
            return TUI_TRUE;
        }
        if(*s == 'f' && *(s + 1) == 'a' && *(s + 2) == 'l' && *(s + 3) == 's' && *(s + 4) == 'e' && checkSymbolNameComplete(s + 5))
        {
            *endptr = (char*)(s + 5);
            return TUI_FALSE;
        }
        
        {
            return {};
        }
    }
    
    virtual uint8_t type() override { return Tui_ref_type_BOOL; }
    virtual std::string getTypeName() override {return "bool";}
    virtual std::string getStringValue() override {
        return (value ? "true" : "false");
    }
    virtual bool boolValue() override {return value;}
    virtual double getNumberValue() override {return value;}
    virtual bool isEqual(const tui::ref& other) override
    {
        if(!other)
        {
            return !value;
        }
        return other.get() == this;
    }
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 1);
        if(value)
        {
            buffer[(*currentOffset)++] = Tui_binary_type_BOOL_TRUE;
        }
        else
        {
            buffer[(*currentOffset)++] = Tui_binary_type_BOOL_FALSE;
        }
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
    virtual tui::ref copy() override
    {
        return tui::create<TuiVec2>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiVec2>(other)->value;
    }
    
    static tui::vec2 initWithHumanReadableString(const char* str, char** endptr, const tui::table& parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '2' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[2] = {0.0,0.0};
            for(int i = 0; i < 2; i++)
            {
                tui::ref loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo, "uninitialized or non-number value in vec2 constructor:%c", *s);
                    return nullptr;
                }
                
                values[i] = (tui::cast<TuiNumber>(loadedValue)->value);
                
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
                    TuiParseError(debugInfo, "found bad char when expecting ',' within vec2:%c", *s);
                    return nullptr;
                }
            }
            
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return tui::create<TuiVec2>(dvec2(values[0], values[1]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() override { return Tui_ref_type_VEC2; }
    virtual std::string getTypeName() override {return "vec2";}
    virtual std::string getStringValue() override {
        return tui::string_format("vec2(%s,%s)", tui::doubleToString(value.x).c_str(), tui::doubleToString(value.y).c_str());
    }
    virtual bool boolValue() override {return true;}
    virtual bool isEqual(const tui::ref& other) override {return other && other->type() == Tui_ref_type_VEC2 && tui::cast<TuiVec2>(other)->value == value;}

    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 17);
        buffer[(*currentOffset)++] = Tui_binary_type_VEC2;
        memcpy(&buffer[(*currentOffset)], &value.x, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value.y, 8);
        *currentOffset += 8;
    }
    
private:
    
private:
};


class TuiVec3 : public TuiRef {
public: //members
    dvec3 value;

public://functions
    TuiVec3(dvec3 value_) : TuiRef() {value = value_;}
    virtual ~TuiVec3() {};
    virtual tui::ref copy() override
    {
        return tui::create<TuiVec3>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiVec3>(other)->value;
    }
    
    static tui::vec3 initWithHumanReadableString(const char* str, char** endptr, const tui::table& parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '3' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[3] = {0.0,0.0,0.0};
            for(int i = 0; i < 3; i++)
            {
                tui::ref loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo, "uninitialized or non-number value in vec3 constructor:%c", *s);
                    return nullptr;
                }
                
                values[i] = (tui::cast<TuiNumber>(loadedValue)->value);
                
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
                    TuiParseError(debugInfo, "found bad char when expecting ',' within vec3:%c", *s);
                    return nullptr;
                }
            }
            
            *endptr = (char*)s;
            
            return tui::create<TuiVec3>(dvec3(values[0], values[1], values[2]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() override { return Tui_ref_type_VEC3; }
    virtual std::string getTypeName() override {return "vec3";}
    virtual std::string getStringValue() override {
        return tui::string_format("vec3(%s,%s,%s)", tui::doubleToString(value.x).c_str(), tui::doubleToString(value.y).c_str(), tui::doubleToString(value.z).c_str());
    }
    virtual bool boolValue() override {return true;}
    virtual bool isEqual(const tui::ref& other) override {return other && other->type() == Tui_ref_type_VEC3 && tui::cast<TuiVec3>(other)->value == value;}

    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 25);
        buffer[(*currentOffset)++] = Tui_binary_type_VEC3;
        memcpy(&buffer[(*currentOffset)], &value.x, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value.y, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value.z, 8);
        *currentOffset += 8;
    }
    
private:
    
private:
};


class TuiVec4 : public TuiRef {
public: //members
    dvec4 value;

public://functions
    TuiVec4(dvec4 value_) : TuiRef() {value = value_;}
    virtual ~TuiVec4() {};
    virtual tui::ref copy() override
    {
        return tui::create<TuiVec4>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiVec4>(other)->value;
    }
    
    static tui::vec4 initWithHumanReadableString(const char* str, char** endptr, const tui::table& parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'v' && *(s + 1) == 'e' && *(s + 2) == 'c' && *(s + 3) == '4' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[4] = {0.0,0.0,0.0,0.0};
            for(int i = 0; i < 4; i++)
            {
                tui::ref loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo, "uninitialized or non-number value in vec4 constructor:%c", *s);
                    return nullptr;
                }
                
                values[i] = (tui::cast<TuiNumber>(loadedValue)->value);
                
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
                    TuiParseError(debugInfo, "found bad char when expecting ',' within vec4:%c", *s);
                    return nullptr;
                }
            }
            
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return tui::create<TuiVec4>(dvec4(values[0], values[1], values[2],  values[3]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() override { return Tui_ref_type_VEC4; }
    virtual std::string getTypeName() override {return "vec4";}
    virtual std::string getStringValue() override {
        return tui::string_format("vec4(%s,%s,%s,%s)", tui::doubleToString(value.x).c_str(), tui::doubleToString(value.y).c_str(), tui::doubleToString(value.z).c_str(), tui::doubleToString(value.w).c_str());
    }
    virtual bool boolValue() override {return true;}
    virtual bool isEqual(const tui::ref& other) override {return other && other->type() == Tui_ref_type_VEC4 && tui::cast<TuiVec4>(other)->value == value;}

    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 33);
        buffer[(*currentOffset)++] = Tui_binary_type_VEC4;
        memcpy(&buffer[(*currentOffset)], &value.x, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value.y, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value.z, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value.w, 8);
        *currentOffset += 8;
    }
    
private:
};


class TuiMat3 : public TuiRef {
public: //members
    dmat3 value;

public://functions
    TuiMat3(dmat3 value_) : TuiRef() {value = value_;}
    virtual ~TuiMat3() {};
    virtual tui::ref copy() override
    {
        return tui::create<TuiMat3>(value);
    }
    virtual void assign(const tui::ref& other) override {
        value = tui::cast<TuiMat3>(other)->value;
    }
    
    static tui::mat3 initWithHumanReadableString(const char* str, char** endptr, const tui::table& parent, TuiDebugInfo* debugInfo) {
        const char* s = tuiSkipToNextChar(str, debugInfo);
        
        if(*s == 'm' && *(s + 1) == 'a' && *(s + 2) == 't' && *(s + 3) == '3' && *(s + 4) == '(')
        {
            s+= 5;
            s = tuiSkipToNextChar(s, debugInfo);
            
            double values[9] = {0,0,0,0,0,0,0,0,0};
            for(int i = 0; i < 9; i++)
            {
                tui::ref loadedValue = TuiRef::loadExpression(s, endptr, nullptr, nullptr, parent, debugInfo);
                s = tuiSkipToNextChar(*endptr, debugInfo);
                
                if(!loadedValue || loadedValue->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(debugInfo, "uninitialized or non-number value in mat3 constructor:%c", *s);
                    return nullptr;
                }
                
                values[i] = (tui::cast<TuiNumber>(loadedValue)->value);
                
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
                    TuiParseError(debugInfo, "found bad char when expecting ',' within mat3:%c", *s);
                    return nullptr;
                }
            }
            
            s = tuiSkipToNextChar(s, debugInfo, true);
            *endptr = (char*)s;
            
            return tui::create<TuiMat3>(dmat3(values[0],
                                     values[1],
                                     values[2],
                                     values[3],
                                     values[4],
                                     values[5],
                                     values[6],
                                     values[7],
                                     values[8]));
        }
        
        return nullptr;
    }
    
    virtual uint8_t type() override { return Tui_ref_type_MAT3; }
    virtual std::string getTypeName() override {return "mat3";}
    virtual std::string getStringValue() override {
        return tui::string_format("mat3((%s,%s,%s), (%s,%s,%s), (%s,%s,%s))",
                                  tui::doubleToString(value[0].x).c_str(),
                                  tui::doubleToString(value[0].y).c_str(),
                                  tui::doubleToString(value[0].z).c_str(),
                                  tui::doubleToString(value[1].x).c_str(),
                                  tui::doubleToString(value[1].y).c_str(),
                                  tui::doubleToString(value[1].z).c_str(),
                                  tui::doubleToString(value[2].x).c_str(),
                                  tui::doubleToString(value[2].y).c_str(),
                                  tui::doubleToString(value[2].z).c_str());
    }
    virtual bool boolValue() override {return true;}
    virtual bool isEqual(const tui::ref& other) override {return other && other->type() == Tui_ref_type_MAT3 && tui::cast<TuiMat3>(other)->value == value;}

    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
    {
        resizeBufferIfNeeded(buffer, currentOffset, 73);
        buffer[(*currentOffset)++] = Tui_binary_type_MAT3;
        memcpy(&buffer[(*currentOffset)], &value[0].x, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[0].y, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[0].z, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[1].x, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[1].y, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[1].z, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[2].x, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[2].y, 8);
        *currentOffset += 8;
        memcpy(&buffer[(*currentOffset)], &value[2].z, 8);
        *currentOffset += 8;
    }
    
private:
};

#endif
