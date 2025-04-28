
#include <iostream>
#include "Interpreter.h"
#include "TuiLog.h"

int main(int argc, const char * argv[]) {
    
    std::vector<std::string> args;
    for(int i = 1; i < argc; i++)
    {
        args.push_back(argv[i]);
    }
    
    if(args.empty())
    {
        TuiLog("Please supply a script to run, eg. ./tui examples/example.tui");
        return 0;
    }
    
    Interpreter* interpreter = new Interpreter(args);
    delete interpreter;
    
    return 0;
}