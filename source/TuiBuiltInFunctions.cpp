#include "TuiBuiltInFunctions.h"
#include "TuiTable.h"
#include <algorithm>
#include <random>

auto rd = std::random_device {};
auto rng = std::default_random_engine { rd() };

namespace Tui {

TuiTable* createRootTable()
{
    TuiTable* rootTable = new TuiTable(nullptr);
    
    //random(max) provides a floating point value between 0 and max (default 1.0)
    rootTable->setFunction("random", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
        {
            TuiRef* arg = args->arrayObjects[0];
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                return new TuiNumber(((double)rng() / RAND_MAX) * ((TuiNumber*)(arg))->value);
            }
        }
        return new TuiNumber(((double)rng() / RAND_MAX));
    });
    
    
    //randomInt(max) provides an integer from 0 to (max - 1) with a default of 2.
    rootTable->setFunction("randomInt", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
        {
            TuiRef* arg = args->arrayObjects[0];
            if(arg->type() == Tui_ref_type_NUMBER)
            {
                double flooredValue = floor(((TuiNumber*)(arg))->value);
                return new TuiNumber(min(flooredValue - 1.0, floor(((double)rng() / UINT32_MAX) * flooredValue)));
            }
        }
        return new TuiNumber(min(1.0, floor(((double)rng() / RAND_MAX) * 2)));
    });
    
    
    // print(msg1, msg2, msg3, ...) print values, args are concatenated together
    rootTable->setFunction("print", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
        {
            std::string printString = "";
            for(TuiRef* arg : args->arrayObjects)
            {
                printString += arg->getDebugStringValue();
            }
            TuiLog("%s", printString.c_str());
        }
        return nullptr;
    });
    
    // error(msg1, msg2, msg3, ...) print values, args are concatenated together, calls abort() to exit the program
    rootTable->setFunction("error", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
        {
            std::string printString = "";
            for(TuiRef* arg : args->arrayObjects)
            {
                printString += arg->getDebugStringValue();
            }
            TuiError("%s", printString.c_str());
            abort();
        }
        return nullptr;
    });
    
    //require(path) loads the given tui file
    rootTable->setFunction("require", [rootTable](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
        {
            return TuiRef::load(Tui::getResourcePath(args->arrayObjects[0]->getStringValue()), rootTable);
        }
        return nullptr;
    });
    
    //readValue() reads input from the command line, serializing just the first value, will call functions and load variables
    rootTable->setFunction("readValue",
                           [rootTable](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        std::string stringValue;
        std::getline(std::cin, stringValue);
        
        TuiDebugInfo debugInfo;
        debugInfo.fileName = "input";
        const char* cString = stringValue.c_str();
        char* endPtr;
        
        TuiRef* enclosingRef = nullptr;
        std::string finalKey = "";
        int finalIndex = -1;
        
        TuiRef* result = TuiRef::loadValue(cString,
                                           &endPtr,
                                           nullptr,
                                           rootTable,
                                           callingDebugInfo,
                                           false,
                                           &enclosingRef,
                                           &finalKey,
                                           &finalIndex);
        if(!result && !finalKey.empty())
        {
            result = new TuiString(finalKey);
        }
        
        return result;
    });
    
    //clear() clears the console
    rootTable->setFunction("clear", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
#if defined _WIN32
        system("cls");
    //clrscr(); // including header file : conio.h
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__)
        system("clear");
    //std::cout<< u8"\033[2J\033[1;1H"; //Using ANSI Escape Sequences
#elif (__APPLE__ && (!TARGET_OS_IPHONE))
        system("clear");
#endif
        return nullptr;
    });
    
    //type() returns the type name of the given object, eg. 'table', 'string', 'number', 'vec4', 'bool'
    rootTable->setFunction("type", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0)
        {
            return new TuiString(args->arrayObjects[0]->getTypeName());
        }
        return new TuiString("nil");
    });
    
    addStringTable(rootTable);
    addTableTable(rootTable);
    addMathTable(rootTable);
    addFileTable(rootTable);
    addDebugTable(rootTable);
    
    return rootTable;
}


void addStringTable(TuiTable* rootTable)
{
    //************
    //string
    //************
    TuiTable* stringTable = new TuiTable(rootTable);
    rootTable->set("string", stringTable);
    stringTable->release();
    static const std::set<int> integerChars = {
        'd','i','o','u','x','X','D','O','U','c','C'
    };
    static const std::set<int> floatingPointChars = {
        'e','E','f','F','g','G','a','A',
    };
    
    stringTable->setFunction("format", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            const char* s = ((TuiString*)args->arrayObjects[0])->value.c_str();
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
                                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "string.format expected at least %d args", argIndex + 1);
                                break;
                            }
                            TuiRef* arg = args->arrayObjects[argIndex++];
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
                                result += Tui::string_format(currentString, (arg->type() == Tui_ref_type_USERDATA ? ((void*)((TuiUserData*)arg)->value): (void*)arg));
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
            
            return new TuiString(result);
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "string.format expected string, args");
        return nullptr;
    });
    
    stringTable->setFunction("length", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiNumber((((TuiString*)args->arrayObjects[0])->value).length());
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "string.length expected string");
        return nullptr;
    });
    
    stringTable->setFunction("subString", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            int length = -1;
            if(args->arrayObjects.size() > 2 && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
            {
                length = ((TuiNumber*)args->arrayObjects[2])->value;
            }
            return new TuiString((((TuiString*)args->arrayObjects[0])->value).substr(((TuiNumber*)args->arrayObjects[1])->value, length));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "string.length expected string, start index, optional length");
        return nullptr;
    });
    
    stringTable->setFunction("sha1", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(TuiSHA1::sha1(((TuiString*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "string.sha1 expected string");
        return nullptr;
    });
    
    //returns nil if not found
    stringTable->setFunction("find", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            int startIndex = 0;
            if(args->arrayObjects.size() > 2 && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
            {
                startIndex = ((TuiNumber*)args->arrayObjects[2])->value;
            }
            
            int location = (int)(((TuiString*)args->arrayObjects[0])->value).find(((TuiString*)args->arrayObjects[1])->value, startIndex);
            if(location < 0)
            {
                return TUI_NIL;
            }
            return new TuiNumber(location);
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "string.find expected string");
        return nullptr;
    });
    
}

void addTableTable(TuiTable* rootTable)
{
    //************
    //table
    //************
    
    TuiTable* tableTable = new TuiTable(rootTable);
    rootTable->set("table", tableTable);
    tableTable->release();
    
    //table.insert(table, index, value) to specify the index or table.insert(table,value) to add to the end
    tableTable->setFunction("insert", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 2)
        {
            TuiRef* tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.insert expected table for first argument");
                return nullptr;
            }
            
            if(args->arrayObjects.size() >= 3)
            {
                TuiRef* indexObject = args->arrayObjects[1];
                if(indexObject->type() != Tui_ref_type_NUMBER)
                {
                    TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.insert expected index for second argument. (object to add is third)");
                    return nullptr;
                }
                int addIndex = ((TuiNumber*)indexObject)->value;
                TuiRef* addObject = args->arrayObjects[2];
                
                ((TuiTable*)tableRef)->insert(addIndex, addObject);
                
            }
            else
            {
                TuiRef* addObject = args->arrayObjects[1];
                addObject->retain();
                ((TuiTable*)tableRef)->arrayObjects.push_back(addObject);
            }
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.insert expected 2-3 args.");
        }
        return nullptr;
    });
    
    //table.remove(table, index) removes an object from an array, shuffling the rest down. Will exit with an error if index is beyond the bounds of the array
    tableTable->setFunction("remove", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 2)
        {
            TuiRef* tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.remove expected table for first argument");
                return nullptr;
            }
            
            TuiRef* indexObject = args->arrayObjects[1];
            if(indexObject->type() != Tui_ref_type_NUMBER)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.remove expected index for second argument.");
                return nullptr;
            }
            int removeIndex = ((TuiNumber*)indexObject)->value;
            
            if(!((TuiTable*)tableRef)->remove(removeIndex))
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.remove index beyond bounds. index:%d array object count:%d", removeIndex, (int)((TuiTable*)tableRef)->arrayObjects.size());
            }
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.remove expected table and index.");
        }
        return nullptr;
    });
    
    //table.count(table) count of array objects
    tableTable->setFunction("count", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1)
        {
            TuiRef* tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.count expected table for first argument");
                return nullptr;
            }
            
            return new TuiNumber(((TuiTable*)tableRef)->arrayObjects.size());
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.count expected table argument");
        }
        return nullptr;
    });
    
    //table.shuffle(table) randomize order of array objects
    tableTable->setFunction("shuffle", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1)
        {
            TuiRef* tableRef = args->arrayObjects[0];
            if(tableRef->type() != Tui_ref_type_TABLE)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "table.shuffle expected table for first argument");
                return nullptr;
            }
            auto& arrayObjects = ((TuiTable*)tableRef)->arrayObjects;
            std::shuffle(std::begin(arrayObjects), std::end(arrayObjects), rng);
        }
        return nullptr;
    });
    
    
    
}

void addMathTable(TuiTable* rootTable)
{
    //************
    //math
    //************
    TuiTable* mathTable = new TuiTable(rootTable);
    rootTable->set("math", mathTable);
    mathTable->release();
    
    
    mathTable->setFunction("sin", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(sin(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.sin expected number");
        return nullptr;
    });
    
    mathTable->setFunction("cos", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(cos(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.cos expected number");
        return nullptr;
    });
    
    mathTable->setFunction("tan", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(tan(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.tan expected number");
        return nullptr;
    });
    
    mathTable->setFunction("asin", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(asin(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.asin expected number");
        return nullptr;
    });
    
    mathTable->setFunction("acos", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(acos(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.acos expected number");
        return nullptr;
    });
    
    mathTable->setFunction("atan", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(atan(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.atan expected number");
        return nullptr;
    });
    
    mathTable->setFunction("atan2", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(atan2(((TuiNumber*)args->arrayObjects[0])->value, ((TuiNumber*)args->arrayObjects[1])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.atan2 expected 2 numbers");
        return nullptr;
    });
    
    mathTable->setFunction("sqrt", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(sqrt(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.sqrt expected number");
        return nullptr;
    });
    
    
    mathTable->setFunction("exp", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(exp(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.exp expected number");
        return nullptr;
    });
    
    mathTable->setFunction("log", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(log(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.log expected number");
        return nullptr;
    });
    
    mathTable->setFunction("log10", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(log10(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.log10 expected number");
        return nullptr;
    });
    
    mathTable->setFunction("floor", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(floor(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.floor expected number");
        return nullptr;
    });
    
    mathTable->setFunction("ceil", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(ceil(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.ceil expected number");
        return nullptr;
    });
    
    mathTable->setFunction("abs", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 0 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(abs(((TuiNumber*)args->arrayObjects[0])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.abs expected number");
        return nullptr;
    });
    
    mathTable->setFunction("fmod", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(fmod(((TuiNumber*)args->arrayObjects[0])->value, ((TuiNumber*)args->arrayObjects[1])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.fmod expected 2 numbers");
        return nullptr;
    });
    
    mathTable->setFunction("max", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(max(((TuiNumber*)args->arrayObjects[0])->value, ((TuiNumber*)args->arrayObjects[1])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.max expected 2 numbers");
        return nullptr;
    });
    
    mathTable->setFunction("min", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 1 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(min(((TuiNumber*)args->arrayObjects[0])->value, ((TuiNumber*)args->arrayObjects[1])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.min expected 2 numbers");
        return nullptr;
    });
    
    mathTable->setFunction("clamp", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() > 2 && args->arrayObjects[0]->type() == Tui_ref_type_NUMBER && args->arrayObjects[1]->type() == Tui_ref_type_NUMBER && args->arrayObjects[2]->type() == Tui_ref_type_NUMBER)
        {
            return new TuiNumber(clamp(((TuiNumber*)args->arrayObjects[0])->value, ((TuiNumber*)args->arrayObjects[1])->value, ((TuiNumber*)args->arrayObjects[2])->value));
        }
        TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "math.clamp expected 3 numbers");
        return nullptr;
    });
    
    mathTable->setDouble("pi", M_PI);
}

void addFileTable(TuiTable* rootTable)
{
    
    //************
    //file
    //************
    TuiTable* fileTable = new TuiTable(rootTable);
    rootTable->set("file", fileTable);
    fileTable->release();
    
    //file.directoryContents(path) returns an array of file names
    fileTable->setFunction("directoryContents", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1)
        {
            TuiRef* pathRef = args->arrayObjects[0];
            if(pathRef->type() != Tui_ref_type_STRING)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.directoryContents expected string argument");
                return nullptr;
            }
            
            std::vector<std::string> directoryContents = Tui::getDirectoryContents(((TuiString*)pathRef)->value);
            
            TuiTable* directroyContentsTable = new TuiTable(nullptr);
            
            for(auto& fileName : directoryContents)
            {
                TuiString* fileNameRef = new TuiString(fileName);
                directroyContentsTable->arrayObjects.push_back(fileNameRef);
            }
            
            return directroyContentsTable;
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.directoryContents expected string argument");
        }
    });
    
    // file.load(path) returns a TuiRef object with the contents of a human readable tui or json file
    fileTable->setFunction("load", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TuiRef::load(((TuiString*)args->arrayObjects[0])->value);
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.load expected string argument");
        }
    });
    
    // file.loadBinary(path) returns an object with the contents of a file that has been saved in the proprietry tui binary format
    fileTable->setFunction("loadBinary", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TuiRef::loadBinary(((TuiString*)args->arrayObjects[0])->value);
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.loadBinary expected string argument");
        }
    });
    
    // file.save(path, object) saves the tui object to disk in a human readable format (unless object is a binary string)
    fileTable->setFunction("save", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            ((TuiString*)args->arrayObjects[1])->saveToFile(((TuiString*)args->arrayObjects[0])->value);
            return nullptr;
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.save expected string argument");
        }
    });
    
    // file.saveBinary(path, object) saves the tui object to disk in a proprietry tui binary format
    fileTable->setFunction("saveBinary", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 2 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            ((TuiString*)args->arrayObjects[1])->saveBinary(((TuiString*)args->arrayObjects[0])->value);
            return nullptr;
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.saveBinary expected string argument");
        }
    });
    
    
    // file.loadData(path) returns a string with the contents of file
    fileTable->setFunction("loadData", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Tui::getFileContents(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.loadData expected string argument");
        }
    });
    
    //file.isDirectory(path) returns true if path is a directory
    fileTable->setFunction("isDirectory", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1)
        {
            TuiRef* pathRef = args->arrayObjects[0];
            if(pathRef->type() != Tui_ref_type_STRING)
            {
                TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.isDirectory expected string argument");
                return nullptr;
            }
            
            if(Tui::isDirectoryAtPath(((TuiString*)pathRef)->value))
            {
                return TUI_TRUE;
            }
            
            return TUI_FALSE;
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.isDirectory expected string argument");
        }
    });
    
    fileTable->setFunction("fileName", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Tui::fileNameFromPath(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.fileNameFromPath expected string argument");
        }
    });
    //file.extension(path) returns the extension including the '.' eg. "image.jpg" returns ".jpg"
    fileTable->setFunction("extension", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Tui::fileExtensionFromPath(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.fileExtensionFromPath expected string argument");
        }
    });
    fileTable->setFunction("changeExtension", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING && args->arrayObjects[1]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Tui::changeExtensionForPath(((TuiString*)args->arrayObjects[0])->value, ((TuiString*)args->arrayObjects[1])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.changeExtensionForPath expected string argument");
        }
    });
    fileTable->setFunction("removeExtension", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Tui::removeExtensionForPath(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.removeExtensionForPath expected string argument");
        }
    });
    fileTable->setFunction("removeLastPathComponent", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiString(Tui::pathByRemovingLastPathComponent(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.pathByRemovingLastPathComponent expected string argument");
        }
    });
    
    //file.fileSizeAtPath(path) returns size in bytes
    fileTable->setFunction("fileSize", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return new TuiNumber(Tui::fileSizeAtPath(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.fileSizeAtPath expected string argument");
        }
    });
    
    //file.fileExistsAtPath(path) returns true if file exists, false otherwise
    fileTable->setFunction("fileExists", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TUI_BOOL(Tui::fileExistsAtPath(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.fileExistsAtPath expected string argument");
        }
    });
    
    //file.fileExistsAtPath(path) returns true if file is a symlink, false otherwise
    fileTable->setFunction("isSymLink", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            return TUI_BOOL(Tui::isSymLinkAtPath(((TuiString*)args->arrayObjects[0])->value));
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.isSymLinkAtPath expected string argument");
        }
    });
    
    //file.createDirectoriesIfNeededForDirPath(path) equivalent to mkdir -p
    fileTable->setFunction("createDirectoriesIfNeededForDirPath", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::createDirectoriesIfNeededForDirPath(((TuiString*)args->arrayObjects[0])->value);
            return nullptr;
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.createDirectoriesIfNeededForDirPath expected string argument");
        }
    });
    
    //file.createDirectoriesIfNeededForFilePath(path) equivalent to mkdir -p
    fileTable->setFunction("createDirectoriesIfNeededForDirPath", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        if(args && args->arrayObjects.size() >= 1 && args->arrayObjects[0]->type() == Tui_ref_type_STRING)
        {
            Tui::createDirectoriesIfNeededForFilePath(((TuiString*)args->arrayObjects[0])->value);
            return nullptr;
        }
        else
        {
            TuiParseError(callingDebugInfo->fileName.c_str(), callingDebugInfo->lineNumber, "file.createDirectoriesIfNeededForFilePath expected string argument");
        }
    });
    
    //todo
    /*

     void moveFile(const std::string& fromPath, const std::string& toPath);
     bool removeFile(const std::string& removePath);
     bool removeEmptyDirectory(const std::string& removePath);
     bool removeDirectory(const std::string& removePath);

     bool copyFileOrDir(const std::string& sourcePath, const std::string& destinationPath);

     void openFile(std::string filePath);

     */
    
    
}

void addDebugTable(TuiTable* rootTable)
{
    //************
    //debug
    //************
    TuiTable* debugTable = new TuiTable(rootTable);
    rootTable->set("debug", debugTable);
    debugTable->release();
    
    
    //debug.getFileName() returns the current script file name or debug identifier string
    debugTable->setFunction("getFileName", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiString(callingDebugInfo->fileName);
    });
    
    //debug.getLineNumber() returns the line number in the current script file
    debugTable->setFunction("getLineNumber", [](TuiTable* args, TuiRef* existingResult, TuiDebugInfo* callingDebugInfo) -> TuiRef* {
        return new TuiNumber(callingDebugInfo->lineNumber);
    });
}


}
