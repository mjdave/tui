
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    for(auto& arg : args)
    {
        tui::ref scriptRunResult = nullptr;
        TuiRef::runScriptFile(tui::getResourcePath(arg), tui::getRootTable(), nullptr, scriptRunResult); //todo this should instead run the first arg, pass the rest to the script
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
}
