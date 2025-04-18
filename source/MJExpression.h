#ifndef MJExpression_h
#define MJExpression_h

#include <stdio.h>
#include <string>
#include <set>
#include "glm.hpp"
#include "MJLog.h"

#include "MJRef.h"

class MJTable;
class MJString;


MJRef* loadVariableIfAvailable(MJString* stringValueRef,
                               const char* str,
                               char** endptr,
                               MJTable* parentTable,
                               MJDebugInfo* debugInfo);

MJRef* recursivelyLoadValue(const char* str,
                            char** endptr,
                            MJRef* leftValue,
                            MJTable* parentTable,
                            MJDebugInfo* debugInfo,
                            bool runLowOperators);

class MJExpression : public MJRef { //THIS MJExpression CLASS IS NOT USED YET
public: //members
    std::string expression;

public://functions
    MJExpression(const std::string& expression_) {expression = expression_;} //THIS MJExpression CLASS IS NOT USED YET
    virtual ~MJExpression() {};
    
    virtual MJExpression* copy()
    {
        return new MJExpression(expression);
    }
    
    virtual uint8_t type() { return MJREF_TYPE_EXPRESSION; }
    virtual std::string getTypeName() {return "expression";}
    virtual std::string getStringValue() {return "\"" + expression + "\"";}

private:
    
private:
};

#endif /* MJExpression_h */
