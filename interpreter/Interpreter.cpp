
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    //TuiRef* debugRef = TuiRef::load(getResourcePath("examples/daveTest.tui"));
   // debugRef->debugLog();
    
    /*for(auto& arg : args)
    {
        TuiRef* scriptRunResult = TuiTable::runScriptFile(getResourcePath(arg));
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }*/
    
    
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/daveTest.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/scope.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/simpleScript.tui");
    TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/functionsAndIfStatements.tui");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/mathsAndExpressions.tui");
    if(scriptRunResult)
    {
        scriptRunResult->debugLog();
    }
}


//#include "TuiScript.h"
//
//int main()
//{
//    // load a JSON-like config file, grab a string and a double, make a modification, save it
//    TuiTable* table = TuiTable::initWithHumanReadableFilePath("config.tui");
//    std::string playerName = table->getString("playerName");
//    double playDuration = table->getDouble("playDuration");
//    table->setDouble("playDuration", playDuration + 1.0);
//    table->saveToFile("config.tui");
//
//    //run a script and print the result
//    TuiRef* scriptRunResult = TuiTable::runScriptFile("script.tui");
//    scriptRunResult->debugLog();
//}
