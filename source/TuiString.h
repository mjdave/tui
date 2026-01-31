
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

public://functions
    
    virtual uint8_t type() { return Tui_ref_type_STRING; }
    virtual std::string getTypeName() {return "string";}
    virtual std::string getStringValue() {return value;}
    virtual double getNumberValue() {return atof(value.c_str());}
    virtual bool boolValue() {return !value.empty();}
    virtual bool isEqual(TuiRef* other) {return other && other->type() == Tui_ref_type_STRING && ((TuiString*)other)->value == value;}
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += "\"" + getStringValue() + "\"";
    }
    
    TuiString(const std::string& value_) : TuiRef() {value = value_;}
    virtual ~TuiString() {};
    
    virtual TuiRef* copy()
    {
        return new TuiString(value);
    }
    virtual void assign(TuiRef* other) {
        value = ((TuiString*)other)->value;
    };
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset)
    {
        resizeBufferIfNeeded(buffer, currentOffset, 5 + (int)value.size());
        buffer[(*currentOffset)++] = Tui_binary_type_STRING;
        uint32_t stringLength = (uint32_t)value.size();
        memcpy(&buffer[(*currentOffset)], &stringLength, 4);
        (*currentOffset)+=4;
        memcpy(&buffer[(*currentOffset)], value.c_str(), value.size());
        *currentOffset += (int)value.size();
    }

private:
    
private:
};

#endif
