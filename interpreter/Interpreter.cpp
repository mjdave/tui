
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    for(auto& arg : args)
    {
        MJLog("loading script:%s", arg.c_str());
        MJTable* debugTable = MJTable::initWithHumanReadableFilePath(getResourcePath(arg), nullptr);
        debugTable->debugLog();
    }
}
