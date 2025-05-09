
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    //TuiRef* debugRef = TuiRef::load(Tui::getResourcePath("examples/mathsAndExpressions.tui"));
   // TuiRef* debugRef = TuiRef::runScriptFile("examples/scope.tui");
   // debugRef->debugLog();
    
    for(auto& arg : args)
    {
        TuiRef* scriptRunResult = TuiRef::runScriptFile(Tui::getResourcePath(arg));
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
    
    //bool debugLogging = false;
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("tests/tests.tui", debugLogging);
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/scope.tui");
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/functionExpressions.tui");
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/simpleScript.tui");
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/functionsAndIfStatements.tui");
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/mathsAndExpressions.tui");
    //TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/testing.tui");
    //if(scriptRunResult)
    //{
    //    scriptRunResult->debugLog();
    //}
}
