
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    TuiRef* debugRef = TuiRef::load(getResourcePath("examples/daveTest.mjh"));
    debugRef->debugLog();
    
    /*for(auto& arg : args)
    {
        TuiRef* scriptRunResult = TuiTable::runScriptFile(getResourcePath(arg));
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }*/
    
    
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/daveTest.mjh");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/scope.mjh");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/simpleScript.mjh");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/functionsAndIfStatements.mjh");
    //TuiRef* scriptRunResult = TuiTable::runScriptFile("examples/mathsAndExpressions.mjh");
    //if(scriptRunResult)
    {
        //scriptRunResult->debugLog();
    }
}


//#include "TuiScript.h"
//
//int main()
//{
//    // load a JSON-like config file, grab a string and a double, make a modification, save it
//    TuiTable* table = TuiTable::initWithHumanReadableFilePath("config.mjh");
//    std::string playerName = table->getString("playerName");
//    double playDuration = table->getDouble("playDuration");
//    table->setDouble("playDuration", playDuration + 1.0);
//    table->saveToFile("config.mjh");
//
//    //run a script and print the result
//    TuiRef* scriptRunResult = TuiTable::runScriptFile("script.mjh");
//    scriptRunResult->debugLog();
//}
