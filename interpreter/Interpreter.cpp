
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    for(auto& arg : args)
    {
        TuiRef* scriptRunResult = TuiRef::runScriptFile(Tui::getResourcePath(arg)); //todo this should instead run the first arg, pass the rest to the script
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
}
