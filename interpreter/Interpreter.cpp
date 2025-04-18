
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    for(auto& arg : args)
    {
        //MJLog("loading script:%s", arg.c_str());
        //MJTable* debugTable = MJTable::initWithHumanReadableFilePath(getResourcePath(arg), nullptr);
        //debugTable->debugLog();
        
        MJRef* scriptRunResult = MJTable::runScriptFile(getResourcePath(arg), nullptr);
        scriptRunResult->debugLog();
    }
}
