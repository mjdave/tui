
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
    
    virtual uint8_t type() override { return Tui_ref_type_STRING; }
    virtual std::string getTypeName() override {return "string";}
    virtual std::string getStringValue() override {return value;}
    virtual double getNumberValue() override {return atof(value.c_str());}
    virtual bool boolValue() override {return true;}
    virtual bool isEqual(TuiRef* other) override {return other && other->type() == Tui_ref_type_STRING && ((TuiString*)other)->value == value;}
    
    virtual void printHumanReadableString(std::string& debugString, int indent = 0) {
        debugString += "\"" + getStringValue() + "\"";
    }
    
    TuiString(const std::string& value_) : TuiRef() {value = value_;}
    virtual ~TuiString() {};
    
    virtual TuiRef* copy() override
    {
        return new TuiString(value);
    }
    virtual void assign(TuiRef* other) override {
        value = ((TuiString*)other)->value;
    }
    
    virtual void serializeBinaryToBuffer(std::string& buffer, int* currentOffset) override
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
