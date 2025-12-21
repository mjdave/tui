//
//  SwiftClosure.cpp
//  TuiEditor
//

#include <string>

#include "TuiTable.h"

#include <vector>
#include <sstream>

void setOtherFunctions(TuiTable* _Nonnull rootTable) {
    
    // replaceString(input, stringToReplace, replacementString, location)
    // location is optional and if omitted all occurances will be replaced
    rootTable->setFunction("replaceString", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        auto str = args->getArray(0)->getStringValue();
        auto oldStr = args->getArray(1)->getStringValue();
        auto newStr = args->getArray(2)->getStringValue();
        
        std::string::size_type pos = 0u;
        
        if (args->arrayObjects.size() == 4) {
            auto location = args->getArray(3)->getNumberValue();
            if ((pos = str.find(oldStr, location)) != std::string::npos) {
                str.replace(pos, oldStr.length(), newStr);
            }
        } else {
            while((pos = str.find(oldStr, pos)) != std::string::npos){
                str.replace(pos, oldStr.length(), newStr);
                pos += newStr.length();
            }
        }
        
        TuiString* tuiString = new TuiString(str);
        return tuiString;
    });
    
    rootTable->setFunction("appendString", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        auto firstString = args->getArray(0)->getStringValue();
        auto secondString = args->getArray(1)->getStringValue();
        auto returnValue = firstString.append(secondString);
        
        TuiString* tuiString = new TuiString(returnValue);
        return tuiString;
    });
    
    rootTable->setFunction("splitString", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        auto stringToSplit = args->getArray(0)->getStringValue();
        auto delimiter = args->getArray(1)->getStringValue();
        if (delimiter.length() == 0) {
            TuiError("Invalid delimiter");
            return nullptr;
        }
        
        auto charDelimiter = delimiter[0];
        
        std::stringstream stream(stringToSplit);
        std::string segment;
        std::vector<TuiRef*> seglist;

        while(std::getline(stream, segment, charDelimiter))
        {
            TuiString* tuiString = new TuiString(segment);
            seglist.push_back(tuiString);
        }
        
        TuiTable* table = new TuiTable(nullptr);
        table->arrayObjects = seglist;
        return table;
    });
}
