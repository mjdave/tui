
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    for(auto& arg : args)
    {
        TuiRef* scriptRunResult = nullptr;
        TuiRef::runScriptFile(Tui::getResourcePath(arg), Tui::getRootTable(), nullptr, scriptRunResult); //todo this should instead run the first arg, pass the rest to the script
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
            scriptRunResult->release();
        }
    }
}
