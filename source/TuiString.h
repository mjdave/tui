
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
    virtual bool boolValue() {return !value.empty();}
    virtual bool isEqual(TuiRef* other) {return other->type() == Tui_ref_type_STRING && ((TuiString*)other)->value == value;}
    
    TuiString(const std::string& value_, TuiTable* parent_ = nullptr) : TuiRef(parent_) {value = value_;}
    virtual ~TuiString() {};
    
    virtual TuiString* copy()
    {
        return new TuiString(value, parent);
    }
    virtual void assign(TuiRef* other) {
        value = ((TuiString*)other)->value;
    };

private:
    
private:
};

#endif
