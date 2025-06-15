
#include "Interpreter.h"

enum {
    TUI_RUN_MODE_RUN_ARGS = 0,
    TUI_RUN_MODE_RUN_ALL_TESTS,
    TUI_RUN_MODE_INTERNAL_TESTING
};

static const int tuiRunMode = TUI_RUN_MODE_RUN_ARGS;

Interpreter::Interpreter(std::vector<std::string>& args)
{
    if(tuiRunMode == TUI_RUN_MODE_RUN_ARGS)
    {
        for(auto& arg : args)
        {
            TuiRef* scriptRunResult = TuiRef::runScriptFile(Tui::getResourcePath(arg));
            if(scriptRunResult)
            {
                scriptRunResult->debugLog();
            }
        }
    }
    else if(tuiRunMode == TUI_RUN_MODE_INTERNAL_TESTING)
    {
        TuiRef* scriptRunResult = TuiRef::runScriptFile("examples/testing.tui");
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
    else if(tuiRunMode == TUI_RUN_MODE_RUN_ALL_TESTS)
    {
        TuiRef* scriptRunResult = TuiRef::runScriptFile("tests/tests.tui");
        if(scriptRunResult)
        {
            scriptRunResult->debugLog();
        }
    }
}
