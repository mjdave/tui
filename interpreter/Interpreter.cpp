
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    //TuiRef* debugRef = TuiRef::load(Tui::getResourcePath("examples/mathsAndExpressions.tui"));
   // debugRef->debugLog();
    
    for(auto& arg : args)
    {
        TuiRef* scriptRunResult = TuiTable::runScriptFile(Tui::getResourcePath(arg));
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
    
    
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/daveTest.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/scope.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/functionExpressions.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/simpleScript.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/functionsAndIfStatements.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/mathsAndExpressions.tui");
    //if(scriptRunResult)
    {
    //    scriptRunResult->debugLog();
    }
}
