#include "TuiBuiltInFunctions.h"
#include "TuiTable.h"
#include <algorithm>
#include <random>
#include <thread>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include "gtx/transform.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

thread_local std::default_random_engine rng = std::default_random_engine { std::random_device{}() };
thread_local std::default_random_engine seedRng;
std::uniform_real_distribution<double> randDistribution(0.0, 1.0);

namespace Tui {

double random(uint32_t seed)
{
    seedRng.seed(seed);
    return randDistribution(seedRng);
}

double random(const std::string& seed)
{
    std::string sha1 = TuiSHA1::sha1(seed);
    uint32_t randValue;
    memcpy(&randValue, &sha1[0], sizeof(randValue));
    seedRng.seed(randValue);
    return randDistribution(seedRng);
}

double random()
{
    return randDistribution(rng);
}

static std::function tui_system = [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {

#if TARGET_OS_IPHONE
    TuiError("system() is not supported on iOS");
#else
    if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
    {
        int result = system((Tui::castPointer<TuiString>(args->arrayObjects[0])->value.c_str()));
        return Tui::createPointer<TuiNumber>(result);
    }
#endif
    return TUI_NIL;
};

static std::function tui_exec = [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
#if TARGET_OS_IPHONE
    TuiError("exec() is not supported on iOS");
#else
    if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
    {
        std::array<char, 128> buffer;
        std::string result;
#if defined _WIN32
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(Tui::castPointer<TuiString>(args->arrayObjects[0])->value.c_str(), "r"), _pclose);
#else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(Tui::castPointer<TuiString>(args->arrayObjects[0])->value.c_str(), "r"), pclose);
#endif
        if (!pipe) {
            TuiError("popen() failed!");
            return TUI_NIL;
        }
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        return Tui::createPointer<TuiString>(result);
    }
#endif
    return TUI_NIL;
};

static std::function tui_print = [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
    if(args && args->arrayObjects.size() > 0)
    {
        std::string printString = "";
        for(Tui::ref arg : args->arrayObjects)
        {
            printString += arg->getDebugStringValue();
        }
        TuiLog("%s", printString.c_str());
    }
    return TUI_NIL;
};


Tui::table initSafeRootTable(const Tui::function& permissionCallbackFunction, const std::string& sandBoxDir)
{
    Tui::table rootTable = Tui::createPointer<TuiTable>();
    
    addBaseFunctions(rootTable, permissionCallbackFunction);
    addStringTable(rootTable);
    addTimeTable(rootTable);
    addTableTable(rootTable);
    addMathTable(rootTable);
    addFileTable(rootTable, sandBoxDir);
    addDebugTable(rootTable);
    
    return rootTable;
}

Tui::table initRootTable()
{
    Tui::table rootTable = Tui::createPointer<TuiTable>();
    
    addBaseFunctions(rootTable);
    addStringTable(rootTable);
    addTimeTable(rootTable);
    addTableTable(rootTable);
    addMathTable(rootTable);
    addFileTable(rootTable);
    addDebugTable(rootTable);
    
    return rootTable;
}

//todo permissionCallbackFunction for error, exit, sleep, require, and sandBoxDir for require
void addBaseFunctions(const Tui::table& rootTable, const Tui::function& permissionCallbackFunction)
{
    //system(string) calls out to a system function eg. system("ls -la")
    if(permissionCallbackFunction)
    {
        rootTable->setFunction("system", [permissionCallbackFunction](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
            Tui::function resultCallbackFunction = nullptr;
            if(args)
            {
                if(args->arrayObjects.size() > 1 && args->arrayObjects[args->arrayObjects.size() - 1]->type() == Tui_ref_type_FUNCTION)
                {
                    resultCallbackFunction = Tui::castPointer<TuiFunction>(args->arrayObjects[args->arrayObjects.size() - 1]);
                }
            }
            
            
            Tui::function gotPermissionResultFunction = Tui::createPointer<TuiFunction>([resultCallbackFunction, args](Tui::table permissionResultArgs, Tui::ref existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
                if(permissionResultArgs && permissionResultArgs->arrayObjects.size() > 0 && permissionResultArgs->arrayObjects[0]->boolValue())
                {
                    Tui::ref callResult = tui_system(args, existingResult, incomingCallData, callingDebugInfo);
                    if(callResult && resultCallbackFunction)
                    {
                        resultCallbackFunction->call("system result callback", callResult);
                    }
                }
                return TUI_NIL;
            });
            
            if(permissionCallbackFunction)
            {
                Tui::ref functionNameRef = Tui::createPointer<TuiString>("system");
                permissionCallbackFunction->call("permissionCallbackFunction", functionNameRef, args, gotPermissionResultFunction);
            }
            else
            {
                TuiWarn("disallowing unpermitted function call to system()");
            }
            return TUI_NIL;
        });
    }
    else
    {
        rootTable->setFunction("system", tui_system);

        rootTable->setFunction("exec", tui_exec);
    }
    
    // print(msg1, msg2, msg3, ...) print values, args are concatenated together
    rootTable->setFunction("print", tui_print);
    
    // error(msg1, msg2, msg3, ...) print values, args are concatenated together, prints a backtrace, calls abort() to exit the program
    rootTable->setFunction("error", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            std::string printString = "";
            for(Tui::ref arg : args->arrayObjects)
            {
                printString += arg->getDebugStringValue();
            }
            TuiParseError(callingDebugInfo, "%s", printString.c_str());
        }
        return TUI_NIL;
    });
    
    // exit(code)
    rootTable->setFunction("exit", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        int code = 0;
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            code = (Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value);
        }
        exit(code);
    });
    
    
    //require(path) loads the given tui file. Keeps a cache in "tui_requireCache"
    //you can also provide your own file.getResourcePath function in the root table
    rootTable->setFunction("require", [rootTable](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            std::string argPath = args->arrayObjects[0]->getStringValue();
            
            Tui::table tui_requireCache = rootTable->getTable("tui_requireCache");
            if(!tui_requireCache)
            {
                tui_requireCache = Tui::createPointer<TuiTable>();
                rootTable->setTable("tui_requireCache", tui_requireCache);
            }
            
            if(tui_requireCache->hasKey(argPath))
            {
                Tui::ref result = tui_requireCache->get(argPath);
                return result;
            }
            TuiDebugInfo debugInfo;
            TuiDebugInfoCopy(callingDebugInfo, &debugInfo);
            
            Tui::ref getResourcePathFunc = (Tui::castPointer<TuiTable>(rootTable->get("file"))->get("getResourcePath"));
            if(getResourcePathFunc)
            {
                Tui::ref pathResult = Tui::castPointer<TuiFunction>(getResourcePathFunc)->call("getResourcePathFunc", args->arrayObjects[0]);
                if(pathResult)
                {
                    Tui::ref loadedRef = TuiRef::runScriptFile(pathResult->getStringValue(), rootTable, &debugInfo);
                    if(loadedRef && loadedRef != TUI_NIL)
                    {
                        tui_requireCache->set(argPath, loadedRef);
                    }
                    return loadedRef;
                }
                return TUI_NIL;
            }
            Tui::ref loadedRef = TuiRef::runScriptFile(Tui::getResourcePath(argPath, callingDebugInfo->currentLine->fileName), rootTable, &debugInfo);
            if(loadedRef && loadedRef != TUI_NIL)
            {
                tui_requireCache->set(argPath, loadedRef);
            }
            return loadedRef;
            
        }
        return TUI_NIL;
    });
    
    //load(string) loads the given tui string
    rootTable->setFunction("load", [rootTable](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::table parentTable = nullptr;
            if(args->arrayObjects.size() > 1 && args->arrayObjects[1]->type() == Tui_ref_type_TABLE)
            {
                parentTable = Tui::castPointer<TuiTable>(args->arrayObjects[1]);
            }
            TuiDebugInfo debugInfo;
            TuiDebugInfoCopy(callingDebugInfo, &debugInfo);
            Tui::ref loadedRef = TuiRef::loadString(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, parentTable, callingDebugInfo);
            return loadedRef;
        }
        TuiParseError(callingDebugInfo, "load expected string");
        return TUI_NIL;
    });
    
    //readValue() reads input from the command line, serializing just the first value, will call functions and load variables
    rootTable->setFunction("readValue",
                           [rootTable](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        std::string stringValue;
        std::getline(std::cin, stringValue);
        
        const char* cString = stringValue.c_str();
        char* endPtr;
        
        Tui::ref enclosingRef = nullptr;
        std::string finalKey = "";
        int finalIndex = -1;
        
        Tui::ref result = TuiRef::loadValue(cString,
                                           &endPtr,
                                           nullptr,
                                           rootTable,
                                           callingDebugInfo,
                                           &enclosingRef,
                                           &finalKey,
                                           &finalIndex);
        if(!result && !finalKey.empty())
        {
            result = Tui::createPointer<TuiString>(finalKey);
        }
        
        return result;
    });
    
    //clear() clears the console
    rootTable->setFunction("clear", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
#if defined _WIN32
        system("cls");
    //clrscr(); // including header file : conio.h
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
        system("clear");
    //std::cout<< u8"\033[2J\033[1;1H"; //Using ANSI Escape Sequences
#elif (__APPLE__ && (!TARGET_OS_IPHONE))
        system("clear");
#endif
        return TUI_NIL;
    });
    
    //type() returns the type name of the given object, eg. 'table', 'string', 'number', 'vec4', 'bool'
    rootTable->setFunction("type", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            return Tui::createPointer<TuiString>(args->arrayObjects[0]->getTypeName());
        }
        return Tui::createPointer<TuiString>("nil");
    });
    
    //sleep(seconds) puts the current thread to sleep for the duration given in seconds
    rootTable->setFunction("sleep", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        return TUI_NIL;
    });
    
    //platform() returns a string representing the current running platform, currently one of: ios, macos, windows, linux
    rootTable->setFunction("platform", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
#if defined _WIN32
        return Tui::createPointer<TuiString>("windows");
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
        return Tui::createPointer<TuiString>("linux");
#elif TARGET_OS_IPHONE
        return Tui::createPointer<TuiString>("ios");
#elif (__APPLE__)
        return Tui::createPointer<TuiString>("macos");
#endif
    });
}

void addStringTable(const Tui::table& rootTable)
{
    //************
    //string
    //************
    Tui::table stringTable = Tui::createPointer<TuiTable>(rootTable);
    rootTable->set("string", stringTable);
    static const std::set<int> integerChars = {
        'd','i','o','u','x','X','D','O','U','c','C'
    };
    static const std::set<int> floatingPointChars = {
        'e','E','f','F','g','G','a','A',
    };
    
    stringTable->setFunction("format", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const char* s = (Tui::castPointer<TuiString>(args->arrayObjects[0])->value.c_str());
            //char* endPtr;
            
            std::string result = "";
            int argIndex = 1;
            
            std::string currentString = "";
            bool percentFound = false;
            bool typeFound = false;
            
            bool interpretAsInteger = false;
            bool interpretAsFloatingPoint = false;
            bool interpretAsPointer = false;
            
            for(;; s++)
            {
                if(*s == '\0' || *s == '%')
                {
                    if(*s == '%' && *(s + 1) == '%')
                    {
                        currentString += *s;
                        s++;
                    }
                    else
                    {
                        if(percentFound)
                        {
                            if(argIndex >= args->arrayObjects.size())
                            {
                                TuiParseError(callingDebugInfo, "string.format expected at least %d args", argIndex + 1);
                                break;
                            }
                            Tui::ref arg = args->arrayObjects[argIndex++];
                            if(interpretAsInteger)
                            {
                                result += Tui::string_format(currentString, (int)arg->getNumberValue());
                            }
                            else if(interpretAsFloatingPoint)
                            {
                                result += Tui::string_format(currentString, arg->getNumberValue());
                            }
                            else if(interpretAsPointer)
                            {
                                result += Tui::string_format(currentString, (arg->type() == Tui_ref_type_USERDATA ? ((void*)(Tui::castPointer<TuiUserData>(arg)->value)) : (void*)arg.get()));
                            }
                            else
                            {
                                result += Tui::string_format(currentString, arg->getStringValue().c_str());
                            }
                            currentString = "";
                        }
                        
                        if(*s == '%')
                        {
                            percentFound = true;
                            typeFound = false;
                            interpretAsInteger = false;
                            interpretAsFloatingPoint = false;
                            interpretAsPointer = false;
                            currentString += *s;
                        }
                        else
                        {
                            result += currentString;
                            break;
                        }
                    }
                }
                else if(*s == '\0')
                {
                    break;
                }
                else
                {
                    if(percentFound && !typeFound)
                    {
                        if(integerChars.count(*s) != 0)
                        {
                            interpretAsInteger = true;
                            typeFound = true;
                        }
                        else if(floatingPointChars.count(*s) != 0)
                        {
                            interpretAsFloatingPoint = true;
                            typeFound = true;
                        }
                        else if(*s == 'p')
                        {
                            interpretAsPointer = true;
                            typeFound = true;
                        }
                        else if(*s == 'S' || *s == 's')
                        {
                            typeFound = true;
                        }
                    }
                    currentString += *s;
                }
            }
            
            return Tui::createPointer<TuiString>(result);
        }
        TuiParseError(callingDebugInfo, "string.format expected string, args");
        return TUI_NIL;
    });
    
    stringTable->setFunction("length", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiNumber>(((Tui::castPointer<TuiString>(args->arrayObjects[0])->value).length()));
        }
        TuiParseError(callingDebugInfo, "string.length expected string");
        return TUI_NIL;
    });
    
    stringTable->setFunction("subString", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            int length = -1;
            if(args->arrayObjects.size() > 2 && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
            {
                length = (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value);
            }
            int32_t pos = (Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value);
            Tui::string tuiString = Tui::castPointer<TuiString>(args->arrayObjects[0]);
            if(pos < 0 || pos >= tuiString->value.length())
            {
                TuiParseError(callingDebugInfo, "string.subString pos:%d invalid for string length:%d", pos, (int)(tuiString->value.length()));
                return TUI_NIL;
            }
            return Tui::createPointer<TuiString>(tuiString->value.substr(pos, length));
        }
        TuiParseError(callingDebugInfo, "string.subString expected string, start index, optional length");
        return TUI_NIL;
    });
    
    stringTable->setFunction("sha1", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(TuiSHA1::sha1((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "string.sha1 expected string");
        return TUI_NIL;
    });
    
    //returns nil if not found
    stringTable->setFunction("find", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            int startIndex = 0;
            if(args->arrayObjects.size() > 2 && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
            {
                startIndex = (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value);
            }
            
            int location = (int)(Tui::castPointer<TuiString>(args->arrayObjects[0])->value).find(Tui::castPointer<TuiString>(args->arrayObjects[1])->value, startIndex);
            if(location == std::string::npos)
            {
                return TUI_NIL;
            }
            return Tui::createPointer<TuiNumber>(location);
        }
        TuiParseError(callingDebugInfo, "string.find expected string");
        return TUI_NIL;
    });
    
    // returns an array of substrings split by the given splitChar. eg. string.split("path/file.txt", "/") -> {"path", "file.txt"}
    stringTable->setFunction("split", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            std::string foundString;
            std::istringstream inputStringStream((Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
            
            std::string& delimString = (Tui::castPointer<TuiString>(args->arrayObjects[1])->value);
            if(delimString.length() != 1)
            {
                TuiParseError(callingDebugInfo, "string.split: single split character expected, but got string of length:%d", (int)delimString.length());
                return TUI_NIL;
            }
            char delim = delimString[0];
            
            Tui::table result = Tui::createPointer<TuiTable>();
            
            while (std::getline(inputStringStream, foundString, delim)) {
                result->arrayObjects.push_back(Tui::createPointer<TuiString>(foundString));
            }
            
            return result;
        }
        TuiParseError(callingDebugInfo, "string.split expected string and split character");
        return TUI_NIL;
    });
    
    // string.replace(string, searchString, replacementString) replaces all occurrences of searchSting within string with replacementString
    stringTable->setFunction("replace", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 2 &&
        args->arrayObjects[0]->type() == Tui_ref_type_STRING &&
        args->arrayObjects[1]->type() == Tui_ref_type_STRING &&
        args->arrayObjects[2]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::stringByReplacingString(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, Tui::castPointer<TuiString>(args->arrayObjects[1])->value, (Tui::castPointer<TuiString>(args->arrayObjects[2])->value)));
        }
        
        TuiParseError(callingDebugInfo, "string.replace expected string, search string, and replace string");
        return TUI_NIL;
    });
    
    
    // string.lower(string) returns the lower case transformation of string
    stringTable->setFunction("lower", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::string result = Tui::castPointer<TuiString>(args->arrayObjects[0]->copy());
            std::transform(result->value.begin(), result->value.end(), result->value.begin(),
                [](unsigned char c){ return std::tolower(c); });
            return result;
        }
        TuiParseError(callingDebugInfo, "string.lower expected string");
        return TUI_NIL;
    });
    
    
    // string.upper(string) returns the upper case transformation of string
    stringTable->setFunction("upper", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::string result = Tui::castPointer<TuiString>(args->arrayObjects[0]->copy());
            std::transform(result->value.begin(), result->value.end(), result->value.begin(),
                [](unsigned char c){ return std::toupper(c); });
            return result;
        }
        TuiParseError(callingDebugInfo, "string.upper expected string");
        return TUI_NIL;
    });
    
    
    // string.numberToHexString(number) returns a string containing the hexadecimal representation of the number interpreted as an 8 bit unsigned integer (0-255) eg. 0 - > "00", 15 - > "0f", 255 - > "ff"
    stringTable->setFunction("numberToHexString", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->isNumber())
        {
            uint8_t byteValue = clamp(args->arrayObjects[0]->getNumberValue(), 0.0, 255.0);
            
            return Tui::createPointer<TuiString>(Tui::string_format("%x%x", (byteValue >> 4), (byteValue & 0x0F)));
        }
        TuiParseError(callingDebugInfo, "string.numberToHexString expected number");
        return TUI_NIL;
    });
    
    
    // string.hexStringToNumber(hexString) returns a standard number containing a 32 bit unsigned integer value by interpreting the given string eg. "FF" - > 255, "ffff" -> 65535
    stringTable->setFunction("hexStringToNumber", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            try
            {
                uint32_t value = stoul((Tui::castPointer<TuiString>(args->arrayObjects[0]))->value, 0, 16);
                return Tui::createPointer<TuiNumber>(value);
            }
            catch(const std::exception& ex)
            {
                return TUI_NIL;
            }
        }
        TuiParseError(callingDebugInfo, "string.hexStringToNumber expected string");
        return TUI_NIL;
    });
    
    // string.eachChar(string, charFunction) loops over each character, calling charFunction(charString, charIndex) for each
    stringTable->setFunction("eachChar", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_FUNCTION)
        {
            Tui::string inputString = Tui::castPointer<TuiString>(args->arrayObjects[0]);
            Tui::function charFunction = Tui::castPointer<TuiFunction>(args->arrayObjects[1]);
            
            Tui::string charString = Tui::createPointer<TuiString>("");
            Tui::number indexNumber = Tui::createPointer<TuiNumber>(0);
            for(indexNumber->value = 0; indexNumber->value < inputString->value.length(); indexNumber->value++)
            {
                charString->value = inputString->value[(int)indexNumber->value];
                Tui::ref result = charFunction->call(incomingCallData, callingDebugInfo, charString, indexNumber);
                if(result && result->boolValue())
                {
                    break;
                }
            }
            
            return TUI_NIL;
        }
        TuiParseError(callingDebugInfo, "string.eachChar expected string, charFunction");
        return TUI_NIL;
    });
    
    // string.eachLine(string, lineFunction) loops over each line, calling lineFunction(lineString, lineIndex) for each
    stringTable->setFunction("eachLine", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_FUNCTION)
        {
            Tui::function lineFunction = Tui::castPointer<TuiFunction>(args->arrayObjects[1]);
            
            Tui::string lineString = Tui::createPointer<TuiString>("");
            Tui::number indexNumber = Tui::createPointer<TuiNumber>(0);
            
            std::istringstream inputStringStream((Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
            
            while (std::getline(inputStringStream, lineString->value)) {
                Tui::ref result = lineFunction->call(incomingCallData, callingDebugInfo, lineString, indexNumber);
                if(result && result->boolValue())
                {
                    break;
                }
                indexNumber->value++;
            }
            
            return TUI_NIL;
        }
        TuiParseError(callingDebugInfo, "string.eachLine expected string, lineFunction");
        return TUI_NIL;
    });
    
}

void addTimeTable(const Tui::table& rootTable)
{
    //************
    //time
    //************
    
    Tui::table timeTable = Tui::createPointer<TuiTable>(rootTable);
    rootTable->set("time", timeTable);
    
    //time.now() current time in seconds since epoch
    timeTable->setFunction("now", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        return Tui::createPointer<TuiNumber>(Tui::nowTime());
    });
}

void addTableTable(const Tui::table& rootTable)
{
    //************
    //table
    //************
    
    Tui::table tableTable = Tui::createPointer<TuiTable>(rootTable);
    rootTable->set("table", tableTable);
    
    //table.insert(table, index, value) to specify the index or table.insert(table,value) to add to the end
    tableTable->setFunction("insert", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.insert expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            if(args->arrayObjects.size() >= 3)
            {
                Tui::ref indexObject = args->arrayObjects[1];
                if(indexObject->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(callingDebugInfo, "table.insert expected index for second argument. (object to add is third)");
                    return TUI_NIL;
                }
                int addIndex = (Tui::castPointer<TuiNumber>(indexObject)->value);
                Tui::ref addObject = args->arrayObjects[2];
                
                Tui::castPointer<TuiTable>(tableRef)->insert(addIndex, addObject);
                
            }
            else
            {
                Tui::ref addObject = args->arrayObjects[1];
                (Tui::castPointer<TuiTable>(tableRef)->arrayObjects.push_back(addObject->copy()));
            }
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.insert expected 2-3 args.");
        }
        return TUI_NIL;
    });
    
    //table.remove(table, index) removes an object from an array, shuffling the rest down. Will exit with an error if index is beyond the bounds of the array
    tableTable->setFunction("remove", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.remove expected table for first argument");
                return TUI_NIL;
            }
            
            Tui::ref indexObject = args->arrayObjects[1];
            if(indexObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.remove expected index for second argument.");
                return TUI_NIL;
            }
            int removeIndex = (Tui::castPointer<TuiNumber>(indexObject)->value);
            
            if(!Tui::castPointer<TuiTable>(tableRef)->remove(removeIndex))
            {
                TuiParseError(callingDebugInfo, "table.remove index beyond bounds. index:%d array object count:%d", removeIndex, (int)(Tui::castPointer<TuiTable>(tableRef)->arrayObjects.size()));
            }
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.remove expected table and index.");
        }
        return TUI_NIL;
    });
    
    tableTable->setFunction("set8Add", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set8Add expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set8Add expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set8.insert(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set8Add expected 2 args.");
        }
        return TUI_NIL;
    });
    
    tableTable->setFunction("set8Remove", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set8Remove expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set8Remove expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set8.erase(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set8Remove expected 2 args.");
        }
        return TUI_NIL;
    });
    
    tableTable->setFunction("set16Add", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set16Add expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set16Add expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set16.insert(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set16Add expected 2 args.");
        }
        return TUI_NIL;
    });
    
    tableTable->setFunction("set16Remove", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set16Remove expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set16Remove expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set16.erase(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set16Remove expected 2 args.");
        }
        return TUI_NIL;
    });
    
    
    tableTable->setFunction("set32Add", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set32Add expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set32Add expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set32.insert(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set32Add expected 2 args.");
        }
        return TUI_NIL;
    });
    
    tableTable->setFunction("set32Remove", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set32Remove expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set32Remove expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set32.erase(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set32Remove expected 2 args.");
        }
        return TUI_NIL;
    });
    
    
    //todo the number is interpreted as a TuiNumber/double, there is no way to specify a 64 bit integer constant in tui yet
    tableTable->setFunction("set64Add", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set64Add expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set64Add expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set64.insert(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set64Add expected 2 args.");
        }
        return TUI_NIL;
    });
    
    tableTable->setFunction("set64Remove", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.set64Remove expected table for first argument. got:%s", tableRef->getTypeName().c_str());
                return TUI_NIL;
            }
            
            Tui::ref addObject = args->arrayObjects[1];
            if(addObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo, "table.set64Remove expected number for second argument. got:%s", tableRef->getTypeName().c_str());
            }
            (Tui::castPointer<TuiTable>(tableRef)->set64.erase(Tui::castPointer<TuiNumber>(addObject)->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.set64Remove expected 2 args.");
        }
        return TUI_NIL;
    });
    
    //table.count(table) count of array objects
    tableTable->setFunction("count", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.count expected table for first argument");
                return TUI_NIL;
            }
            
            return Tui::createPointer<TuiNumber>((Tui::castPointer<TuiTable>(tableRef)->arrayObjects.size()));
        }
        else
        {
            TuiParseError(callingDebugInfo, "table.count expected table argument");
        }
        return TUI_NIL;
    });
    
    //table.shuffle(table) randomize order of array objects. Shuffles the table in-place.
    tableTable->setFunction("shuffle", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.shuffle expected table for first argument");
                return TUI_NIL;
            }
            auto& arrayObjects = (Tui::castPointer<TuiTable>(tableRef)->arrayObjects);
            std::shuffle(std::begin(arrayObjects), std::end(arrayObjects), rng);
        }
        return TUI_NIL;
    });
    
    //table.clone(table) does a shallow copy of the table, returning a new table with the same contents.
    tableTable->setFunction("clone", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.clone expected table for first argument");
                return TUI_NIL;
            }
            return (Tui::castPointer<TuiTable>(tableRef)->trueCopy());
        }
        return TUI_NIL;
    });
    
    //table.sort(table, compareFunctionOrNil) sorts table in place, using optional compareFunction to compare objects. default compareFunction is function(a,b) { return a < b }
    tableTable->setFunction("sort", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1)
        {
            Tui::ref tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo, "table.sort expected table for first argument");
                return TUI_NIL;
            }
            auto& arrayObjects = (Tui::castPointer<TuiTable>(tableRef)->arrayObjects);
            if(!arrayObjects.empty())
            {
                
                if(args->arrayObjects.size() >= 2 && args->arrayObjects[1]->type() == Tui_ref_type_FUNCTION)
                {
                    Tui::function compareFunction = Tui::castPointer<TuiFunction>(args->arrayObjects[1]);
                    
                    std::sort(arrayObjects.begin(), arrayObjects.end(), [compareFunction](Tui::ref a, Tui::ref b) {
                          return compareFunction->call("compare", a, b)->boolValue();
                    });
                }
                else
                {
                    std::sort(arrayObjects.begin(), arrayObjects.end(), [](Tui::ref a, Tui::ref b) {
                        if(a->type() != b->type())
                        {
                            return false;
                        }
                        switch (a->type()) {
                            case Tui_ref_type_NUMBER:
                                return Tui::castPointer<TuiNumber>(a)->value < (Tui::castPointer<TuiNumber>(b)->value);
                                break;
                            case Tui_ref_type_STRING:
                                return Tui::castPointer<TuiString>(a)->value < (Tui::castPointer<TuiString>(b)->value);
                                break;
                                
                            default:
                                return false;
                                break;
                        }
                    });
                }
            }
        }
        return TUI_NIL;
    });
    
    
}

void addMathTable(const Tui::table& rootTable)
{
    //************
    //math
    //************
    Tui::table mathTable = Tui::createPointer<TuiTable>(rootTable);
    rootTable->set("math", mathTable);
    
    //math.random(max, seedOrNil) provides a floating point value between 0 and max (default 1.0). Uses a random seed unless seedOrNil is provided
    mathTable->setFunction("random", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            Tui::ref arg = args->arrayObjects[0];
            double result = 0.0;
            
            if(args->arrayObjects.size() > 1)
            {
                Tui::ref arg2 = args->arrayObjects[1];
                if(arg2->type() == Tui_ref_type_NUMBER)
                {
                    result = Tui::random((Tui::castPointer<TuiNumber>(arg2))->value);
                }
                else if(arg2->type() == Tui_ref_type_STRING)
                {
                    result = Tui::random((Tui::castPointer<TuiString>(arg2))->value);
                }
                else
                {
                    TuiParseError(callingDebugInfo, "math.random(max, optionalSeed) expected number or string for seed argument");
                }
            }
            else
            {
                result = Tui::random();
            }
            
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                return Tui::createPointer<TuiNumber>(result * (Tui::castPointer<TuiNumber>((arg))->value));
            }
            
            return Tui::createPointer<TuiNumber>(result);
        }
        
        return  Tui::createPointer<TuiNumber>(Tui::random());
    });
    
    
    //math.randomInt(max, seedOrNil) provides an integer from 0 to (max - 1) with a default of 2. Uses a random seed unless seedOrNil is provided
    mathTable->setFunction("randomInt", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            Tui::ref arg = args->arrayObjects[0];
            double result = 0.0;
            
            if(args->arrayObjects.size() > 1)
            {
                Tui::ref arg2 = args->arrayObjects[1];
                if(arg2->type() == Tui_ref_type_NUMBER)
                {
                    result = Tui::random((Tui::castPointer<TuiNumber>(arg2))->value);
                }
                else if(arg2->type() == Tui_ref_type_STRING)
                {
                    result = Tui::random((Tui::castPointer<TuiString>(arg2))->value);
                }
                else
                {
                    TuiParseError(callingDebugInfo, "math.random(max, optionalSeed) expected number or string for seed argument");
                }
            }
            else
            {
                result = Tui::random();
            }
            
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                double flooredValue = floor((Tui::castPointer<TuiNumber>((arg))->value));
                return Tui::createPointer<TuiNumber>(min(flooredValue - 1.0, floor(result * flooredValue)));
            }
            
            return Tui::createPointer<TuiNumber>(floor(result));
        }
        return Tui::createPointer<TuiNumber>(min(1.0, floor(Tui::random() * 2)));
    });
    
    mathTable->setFunction("pow", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(pow(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value)));
        }
        TuiParseError(callingDebugInfo, "math.pow expected 2 numbers");
        return TUI_NIL;
    });
    
    mathTable->setFunction("sin", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(sin((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.sin expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("cos", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(cos((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.cos expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("tan", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(tan((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.tan expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("asin", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(asin((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.asin expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("acos", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(acos((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.acos expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("atan", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(atan((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.atan expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("atan2", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(atan2(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value)));
        }
        TuiParseError(callingDebugInfo, "math.atan2 expected 2 numbers");
        return TUI_NIL;
    });
    
    mathTable->setFunction("sqrt", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(sqrt((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.sqrt expected number");
        return TUI_NIL;
    });
    
    
    mathTable->setFunction("exp", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(exp((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.exp expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("log", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(log((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.log expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("log10", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(log10((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.log10 expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("floor", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(floor((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.floor expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("ceil", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(ceil((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.ceil expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("abs", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(abs((Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "math.abs expected number");
        return TUI_NIL;
    });
    
    mathTable->setFunction("fmod", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(fmod(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value)));
        }
        TuiParseError(callingDebugInfo, "math.fmod expected 2 numbers");
        return TUI_NIL;
    });
    
    mathTable->setFunction("max", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(max(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value)));
        }
        TuiParseError(callingDebugInfo, "math.max expected 2 numbers");
        return TUI_NIL;
    });
    
    mathTable->setFunction("min", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(min(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value)));
        }
        TuiParseError(callingDebugInfo, "math.min expected 2 numbers");
        return TUI_NIL;
    });
    
    mathTable->setFunction("clamp", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 2 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
        {
            return Tui::createPointer<TuiNumber>(clamp(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value)));
        }
        TuiParseError(callingDebugInfo, "math.clamp expected 3 numbers");
        return TUI_NIL;
    });
    
    mathTable->setFunction("mix", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 2 && args->arrayObjects[0]->type() == args->arrayObjects[1]->type() && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
            {
                return Tui::createPointer<TuiNumber>(mix(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC2)
            {
                return Tui::createPointer<TuiVec2>(mix(Tui::castPointer<TuiVec2>(args->arrayObjects[0])->value, Tui::castPointer<TuiVec2>(args->arrayObjects[1])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiVec3>(mix(Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value, Tui::castPointer<TuiVec3>(args->arrayObjects[1])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC4)
            {
                return Tui::createPointer<TuiVec4>(mix(Tui::castPointer<TuiVec4>(args->arrayObjects[0])->value, Tui::castPointer<TuiVec4>(args->arrayObjects[1])->value, (Tui::castPointer<TuiNumber>(args->arrayObjects[2])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.mix expected 2 numbers or vectors and a number");
        return TUI_NIL;
    });
    
    mathTable->setDouble("pi", M_PI);
    
    mathTable->setFunction("normalize", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_VEC2)
            {
                return Tui::createPointer<TuiVec2>(normalize((Tui::castPointer<TuiVec2>(args->arrayObjects[0])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiVec3>(normalize((Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.normalize expected vec2 or vec3");
        return TUI_NIL;
    });
    
    mathTable->setFunction("length", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_VEC2)
            {
                return Tui::createPointer<TuiNumber>(length((Tui::castPointer<TuiVec2>(args->arrayObjects[0])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiNumber>(length((Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.length expected vec2 or vec3");
        return TUI_NIL;
    });
    
    mathTable->setFunction("length2", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_VEC2)
            {
                return Tui::createPointer<TuiNumber>(dot(Tui::castPointer<TuiVec2>(args->arrayObjects[0])->value, (Tui::castPointer<TuiVec2>(args->arrayObjects[0])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiNumber>(dot(Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value, (Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.length2 expected vec2 or vec3");
        return TUI_NIL;
    });
    
    mathTable->setFunction("dot", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_VEC2 && args->arrayObjects[1]->type() == Tui_ref_type_VEC2)
            {
                return Tui::createPointer<TuiNumber>(dot(Tui::castPointer<TuiVec2>(args->arrayObjects[0])->value, (Tui::castPointer<TuiVec2>(args->arrayObjects[1])->value)));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_VEC3 && args->arrayObjects[1]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiNumber>(dot(Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value, (Tui::castPointer<TuiVec3>(args->arrayObjects[1])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.dot expected two vec2 or vec3s");
        return TUI_NIL;
    });
    
    mathTable->setFunction("cross", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_VEC3 && args->arrayObjects[1]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiVec3>(cross(Tui::castPointer<TuiVec3>(args->arrayObjects[0])->value, (Tui::castPointer<TuiVec3>(args->arrayObjects[1])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.cross expected two vec3s");
        return TUI_NIL;
    });
    
    
    //math.rotate(angleDegrees, axisVec3) returns a mat3 rotation matrix
    mathTable->setFunction("rotate", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 1)
        {
            if(args->arrayObjects[0]->type() == Tui_ref_type_MAT3 && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER && args->arrayObjects[2]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiMat3>(Tui::castPointer<TuiMat3>(args->arrayObjects[0])->value * dmat3(rotate(Tui::castPointer<TuiNumber>(args->arrayObjects[1])->value, (Tui::castPointer<TuiVec3>(args->arrayObjects[2])->value))));
            }
            else if(args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_VEC3)
            {
                return Tui::createPointer<TuiMat3>(rotate(Tui::castPointer<TuiNumber>(args->arrayObjects[0])->value, (Tui::castPointer<TuiVec3>(args->arrayObjects[1])->value)));
            }
        }
        TuiParseError(callingDebugInfo, "math.rotate expected angleDegrees, axisVec3");
        return TUI_NIL;
    });
}

void addFileTable(const Tui::table& rootTable, const std::string& sandBoxDir) //TODO! sandBoxDir ignored
{
    
    //************
    //file
    //************
    Tui::table fileTable = Tui::createPointer<TuiTable>(rootTable);
    rootTable->set("file", fileTable);
    
    //file.directoryContents(path) returns an array of file names
    fileTable->setFunction("directoryContents", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1)
        {
            Tui::ref pathRef = args->arrayObjects[0];
            if(pathRef->type() != Tui_ref_type_STRING)
            {
                TuiParseError(callingDebugInfo, "file.directoryContents expected string argument");
                return TUI_NIL;
            }
            
            std::vector<std::string> directoryContents = Tui::getDirectoryContents((Tui::castPointer<TuiString>(pathRef)->value));
            
            Tui::table directroyContentsTable = Tui::createPointer<TuiTable>();
            
            for(auto& fileName : directoryContents)
            {
                Tui::string fileNameRef = Tui::createPointer<TuiString>(fileName);
                directroyContentsTable->arrayObjects.push_back(fileNameRef);
            }
            
            return directroyContentsTable;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.directoryContents expected string argument");
        }
    });
    
    // file.sha1(path) returns an sha1 hash of the contents of the file at the path provided
    fileTable->setFunction("sha1", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(TuiSHA1::from_file((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        TuiParseError(callingDebugInfo, "file.sha1 expected string");
        return TUI_NIL;
    });
    
    // file.load(path) returns a TuiRef object with the contents of a human readable tui or json file
    fileTable->setFunction("load", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TuiRef::runScriptFile(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, nullptr, callingDebugInfo);
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.load expected string argument");
        }
    });
    
    // file.loadBinary(path) returns an object with the contents of a file that has been saved in the proprietry tui binary format
    fileTable->setFunction("loadBinary", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TuiRef::loadBinary((Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.loadBinary expected string argument");
        }
    });
    
    // file.save(path, object) saves the tui object to disk in a human readable format (unless object is a binary string)
    fileTable->setFunction("save", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            (Tui::castPointer<TuiString>(args->arrayObjects[1])->saveToFile(Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
            return TUI_NIL;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.save expected string argument");
        }
    });
    
    // file.saveBinary(path, object) saves the tui object to disk in a proprietry tui binary format
    fileTable->setFunction("saveBinary", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            (Tui::castPointer<TuiString>(args->arrayObjects[1])->saveBinary(Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
            return TUI_NIL;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.saveBinary expected string argument");
        }
    });
    
    
    // file.loadData(path) returns a string with the contents of file
    fileTable->setFunction("loadData", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::string result = Tui::createPointer<TuiString>("");
            bool success = Tui::getFileContents(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, &(result->value));
            if(!success)
            {
                TuiWarn("file not found in file.loadData. path:%s", (Tui::castPointer<TuiString>(args->arrayObjects[0])->value.c_str()));
                return TUI_NIL;
            }
            return result;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.loadData expected string argument");
        }
    });
    
    // file.saveData(path, string) saves the string to disk directly
    fileTable->setFunction("saveData", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING  && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            Tui::writeToFile(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, ((Tui::castPointer<TuiString>(args->arrayObjects[1])->value)));
            return TUI_NIL;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.saveData expected 2 string arguments");
        }
    });
    
    //file.isDirectory(path) returns true if path is a directory
    fileTable->setFunction("isDirectory", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1)
        {
            Tui::ref pathRef = args->arrayObjects[0];
            if(pathRef->type() != Tui_ref_type_STRING)
            {
                TuiParseError(callingDebugInfo, "file.isDirectory expected string argument");
                return TUI_NIL;
            }
            
            if(Tui::isDirectoryAtPath(Tui::castPointer<TuiString>(pathRef)->value))
            {
                return TUI_TRUE;
            }
            
            return TUI_FALSE;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.isDirectory expected string argument");
        }
    });
    
    fileTable->setFunction("fileName", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::fileNameFromPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.fileNameFromPath expected string argument");
        }
    });
    //file.extension(path) returns the extension including the '.' eg. "image.jpg" returns ".jpg"
    fileTable->setFunction("extension", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::fileExtensionFromPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.fileExtensionFromPath expected string argument");
        }
    });
    fileTable->setFunction("changeExtension", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::changeExtensionForPath(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, (Tui::castPointer<TuiString>(args->arrayObjects[1])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.changeExtensionForPath expected string argument");
        }
    });
    fileTable->setFunction("removeExtension", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::removeExtensionForPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.removeExtensionForPath expected string argument");
        }
    });
    fileTable->setFunction("removeLastPathComponent", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::pathByRemovingLastPathComponent((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.pathByRemovingLastPathComponent expected string argument");
        }
    });
    
    //file.fileSizeAtPath(path) returns size in bytes
    fileTable->setFunction("fileSize", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiNumber>(Tui::fileSizeAtPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.fileSizeAtPath expected string argument");
        }
        return TUI_NIL;
    });
    
    //file.fileExists(path) returns true if file exists, false otherwise
    fileTable->setFunction("fileExists", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TUI_BOOL(Tui::fileExistsAtPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.fileExistsAtPath expected string argument");
        }
    });
    
    //file.isSymLink(path) returns true if file is a symlink, false otherwise
    fileTable->setFunction("isSymLink", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TUI_BOOL(Tui::isSymLinkAtPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.isSymLinkAtPath expected string argument");
        }
    });
    
    //file.createDirectoriesIfNeededForDirPath(path) equivalent to mkdir -p
    fileTable->setFunction("createDirectoriesIfNeededForDirPath", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::createDirectoriesIfNeededForDirPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
            return TUI_NIL;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.createDirectoriesIfNeededForDirPath expected string argument");
        }
    });
    
    //file.createDirectoriesIfNeededForFilePath(path) equivalent to mkdir -p
    fileTable->setFunction("createDirectoriesIfNeededForFilePath", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::createDirectoriesIfNeededForFilePath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value));
            return TUI_NIL;
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.createDirectoriesIfNeededForFilePath expected string argument");
        }
    });
    
    //file.getAbsolutePath(path) returns the full path for a given relative path
    fileTable->setFunction("getAbsolutePath", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return Tui::createPointer<TuiString>(Tui::getAbsolutePath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.getAbsolutePath expected string argument");
        }
    });
    
    //file.isSubPath(path, basePath) returns true if path is a subPath of (is contained within) basePath, false otherwise. basePath is optional, defaults to current working directory
    fileTable->setFunction("isSubPath", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            if(args->arrayObjects.size() >= 2 && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
            {
                return TUI_BOOL(Tui::isSubPath(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, (Tui::castPointer<TuiString>(args->arrayObjects[1])->value)));
            }
            return TUI_BOOL(Tui::isSubPath((Tui::castPointer<TuiString>(args->arrayObjects[0])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.isSubPath expected string argument");
        }
    });
    
    //file.move(fromPath, toPath) // overwrites if toPath already exists
    fileTable->setFunction("move", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            return TUI_BOOL(Tui::moveFile(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, (Tui::castPointer<TuiString>(args->arrayObjects[1])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.move expected 2 string arguments");
        }
        return TUI_FALSE;
    });
    
    //file.remove(path)
    fileTable->setFunction("remove", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const std::string path = (Tui::castPointer<TuiString>(args->arrayObjects[0])->value);
            if(isDirectoryAtPath(path) && !isSymLinkAtPath(path))
            {
                return TUI_BOOL(removeDirectory(path));
            }
            else
            {
                return TUI_BOOL(removeFile(path));
            }
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.remove expected string argument");
        }
        return TUI_FALSE;
    });
    
    
    //file.copy(sourcePath, destinationPath) // overwritoverwrites if toPath already exists
    fileTable->setFunction("copy", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            return TUI_BOOL(Tui::copyFileOrDir(Tui::castPointer<TuiString>(args->arrayObjects[0])->value, (Tui::castPointer<TuiString>(args->arrayObjects[1])->value)));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.copy expected 2 string arguments");
        }
        return TUI_FALSE;
    });
    
    //file.mkdir(path) //makes all enclosing/intermediate directories too, equivalent to mkdir -p
    fileTable->setFunction("mkdir", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const std::string path = (Tui::castPointer<TuiString>(args->arrayObjects[0])->value);
            return TUI_BOOL(Tui::createDirectoriesIfNeededForDirPath(path));
        }
        else
        {
            TuiParseError(callingDebugInfo, "file.mkdir expected string argument");
        }
        return TUI_FALSE;
    });
    
    
}

void addDebugTable(const Tui::table& rootTable)
{
    //************
    //debug
    //************
    Tui::table debugTable = Tui::createPointer<TuiTable>(rootTable);
    rootTable->set("debug", debugTable);
    
    
    //debug.getFileName() returns the current script file name or debug identifier string
    debugTable->setFunction("getFileName", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        return Tui::createPointer<TuiString>(callingDebugInfo->currentLine->fileName);
    });
    
    //debug.getLineNumber() returns the line number in the current script file
    debugTable->setFunction("getLineNumber", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        return Tui::createPointer<TuiNumber>(callingDebugInfo->currentLine->lineNumber);
    });
    
    
    //debug.break() breaks, but only if you set a breakpoint in this function :)
    debugTable->setFunction("break", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        return TUI_NIL;
    });
    
    //debug.backtrace() prints a backtrace
    debugTable->setFunction("backtrace", [](const Tui::table& args, const Tui::ref& existingResult, TuiFunctionCallData* incomingCallData, TuiDebugInfo* callingDebugInfo) -> Tui::ref {
        TuiLog("debug.backtrace:");
        TuiPrintDebugBacktrace(callingDebugInfo);
        return TUI_NIL;
    });
    
}


}
