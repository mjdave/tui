
#include "Interpreter.h"

Interpreter::Interpreter(std::vector<std::string>& args)
{
    //MJTable* debugTable = MJTable::initWithHumanReadableFilePath(getResourcePath("examples/mathsAndExpressions.mjh"), nullptr);
   // debugTable->debugLog();
    
    for(auto& arg : args)
    {
        MJRef* scriptRunResult = MJTable::runScriptFile(getResourcePath(arg), nullptr);
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
}


//#include "MJScript.h"
//
//int main()
//{
//    // load a JSON-like config file, grab a string and a double, make a modification, save it
//    MJTable* table = MJTable::initWithHumanReadableFilePath("config.mjh");
//    std::string playerName = table->getString("playerName");
//    double playDuration = table->getDouble("playDuration");
//    table->setDouble("playDuration", playDuration + 1.0);
//    table->saveToFile("config.mjh");
//
//    //run a script and print the result
//    MJRef* scriptRunResult = MJTable::runScriptFile("script.mjh");
//    scriptRunResult->debugLog();
//}
