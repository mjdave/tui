# This is tui

tui is a small, cross platform, open source embeddable scripting language and serialization library for C++.

***NOTE (July 2025): tui is still in the early stages of development, it is not fully optimized and will still contain bugs. It does work pretty well now though, and is actively being used and improved. ***

Created by a solo game developer to be fast, small, and easy to integrate and use, tui combines a key/value storage data format in a human readable format similar to JSON, with a powerful scripting language and interpreter similar to lua.

tui is dynamically typed, flexible, and lightweight. Data is shared in memory between tui and the host c++ program, allowing high performance data read/writes in both environments.

Compared to a JSON serializer, tui adds a whole scripting language on top. It's also super fast, provides your data in thinly wrapped stl containers (eg std::map, std::vector), and populates the data for the host program to read immediately when parsed.

Compared to lua, tui is generally slower (up to 10x slower in tight loops), but easier to integrate and bind, and (potentially) faster when sharing data between C++ and the scripting environment. It has a smaller footprint, but less language features, and has good, fast built-in table serialization.

There is a [VSCode extension](https://github.com/PythooonUser/vscode-tui-language-support) that adds language support for tui too!

# Features & Usage/Examples

## A single object is valid
This is a valid tui file:
```lua
42
```
If you loaded it, the TuiRef* you got would be an TuiNumber with the value of 42.

This is also valid file that will give us an TuiTable with array elements:
```lua
42, "coconut", vec3(1,2,3), true, nil, {x=10}
```

## Reads JSON
This is the same data in JSON. tui can read this too, as it treats ":" and "=" the same, "{" and "[" the same, and doesn't require commas or quotes, but doesn't mind them either.
```json
[
    42,
    "coconut",
    {
        "x":1,
        "y":2,
        "z":3
    },
    true,
    null,
    {
        "x":10
    }
]
```

Whitespace is ignored, however newlines are treated like commas, unless quoted or within a bracketed expression
```java
array = {
    "This is the first object with index zero"
    "We don't have any commas after each variable, but we can if we want to",
    surprise = "We can also mix array values with keyed values like in lua"
}

//this is a comment, anything after '//' will be skipped by the parser until the next line
#this is also a comment
/*
and this is a
block comment
*/
```

## Adds variables, if statements & Expressions
Any previously assigned value can be accessed by the key name. This can be used to define constants to do math for later values.
```javascript
width = 400,
height = 200,
halfWidth = width * 0.5,
doubleWidth = width * 2.0,

//if statements can be put right in there with your data
if(width > 300 or height < 150) // brackets optional, 'if width > 300' is also valid. 'or' and 'and' are supported, as well as !,<,>,>=,<=,==,!=
{
    height *= 2.0
}
else // 'else if' and 'elseif' are both valid too
{
    height *= 0.5
}
```
## Functions
Functions support value assignments, if/else statements, and for loops.
```javascript
addTariff = function(base)
{
    tariff = 145 / 100
    if(randomInt(2) > 0)
    {
        tariff = 245 / 100
    }
    return (base * (1.0 + tariff))
}

costOfTV = addTariff(500)
costOfPlaystation = addTariff(400)
sadness = 10

for (i = 0, i < 5, i++)
{
    print("sigh")
    sadness = sadness + 1
}
```
The randomInt() function is built in, it's loaded in to the root table by default. 

### List of built in functions (so far):
```c++
random(max)             // provides a floating point value between 0 and max (default 1.0)
randomInt(max)          // provides an integer from 0 to (max - 1) with a default of 2.

print(msg1, msg2, ...)  // print values, args are concatenated together
error(msg1, msg2, ...)  // print values, args are concatenated together, calls abort() to exit the program
readValue()             // reads input from the command line, serializing just the first value, doesn't (shouldn't!) call functions or load variables
clear()                 // clears the console when run from a command line

require(path)   // loads the given tui file, path is relative to the tui binary (for now)
type()          // returns the type name of the given object, eg. 'table', 'string', 'number', 'vec4', 'bool'

debug.getFileName()             // returns the current script file name or debug identifier string
debug.getLineNumber()           // returns the line number in the current script file

table.count(table)                  // count of array objects
table.insert(table, index, value)   // insert into an array, specifying the index. Will be filled with nil objects < index. Objects >= index are shifted
table.insert(table,value)           // add to the end of an array
table.remove(table, index)          // removes an object from an array, shuffling the rest down. Will exit with an error if index is beyond the bounds of the array

string.length(string)           // returns the number of characters in string
string.format(string, arg1, arg2, ...)    // works like printf, eg string.format("float:%.2f int:%d hex:%x", 1.2345, 5.78, 127) produces "float:1.23 int:5 hex:7f"
string.subString(string, pos)               // returns a substring from the chracter at index 'pos' to the end of the string
string.subString(string, pos, length)       // returns a substring from the chracter at index 'pos' to pos + length or the end of the string, whichever comes first

math.pi //pi constant

math.sqrt(x)
math.exp(x)
math.log(x)
math.log10(x)

math.floor(x)
math.ceil(x)
math.fmod(x)
math.abs(x)
math.max(x, y)
math.min(x, y)
math.clamp(x, min, max)

math.sin(x)
math.cos(x)
math.tan(x)
math.asin(x)
math.acos(x)
math.atan(x)
math.atan2(y,x)

```

### Supplying custom functions in C++

You can supply your own functions in C++ easily by providing a std::function that takes tables for args and any parent state, and gives you the result, eg. here is the code that adds the print function:
```c++

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

```

## Vectors
The only dependency of tui is glm, which currently exposes vec2, vec3, vec4, and mat3(WIP) types, as well as a number of builtin math functions (not yet implemented)
```javascript
size = vec2(400,200)
color = vec4(0.0,1.0,0.0,1.0)
halfSize = size * 0.5
```

## Scope
Every table and every function has its own scope for variable creation/assignment.

If you assign to a variable eg: `a = 42` then that will assign to any existing variable named 'a' within the current scope only. If the parent table or function has a variable named 'a', it remains unaffected, and a new local 'a' is created.

Variables are readable for all child tables/functions, and also read/writable via the `'.'` syntax to access children and `'..'` to access parents.

```javascript
value = 10
table = {
    subValue = 20
    subTable = {
        subValue = 30 // creates a new local table.subTable.subValue with the value 30
        
        ..subValue = value // this assigns 10 to the parent subValue (previously 20)
        
        enclosingTable = .. // we can store the parent table '..' in a local variable
        enclosingTable.subValue = value // achieves the same as '..subValue = value'.
        enclosingTable = nil // otherwise we create a circular loop and will hang if we try to log or iterate this table!
        
        ...value = 20 // we can go up multiple levels by adding dots, this modifies the variable created at the top level on the first line
        
        testValue = 1
        
        testFunction = function(valueToSet) { // the same rules apply for functions
            ..testValue = value + 1 // 21, remember we can always *read* the higher level variables directly
            ....value = valueToSet // 4 dots this time to modify
        }

        testFunction(100)
    }
    
    outsideFunc = subTable.testFunction // we can grab a reference to that subTable's function and call it directly here
    outsideFunc(200) // ..value is now 200. '....value' still works, because we find variables relative to where the function was defined, not where it is called
    thisFuncDoesntExist() // error examples/scope.tui:27:attempt to call missing function: thisFuncDoesntExist()
}
```

if/else and for statements do not create new scopes, but share the parent scope. Inside if/else/for blocks, you may freely access parent values, and all assigned variables belong to the enclosing table/function.

```javascript
a = 10

if(a == 10)
{
    a = 5 //this is assigning to the a variable created above
}

b = a // b is now 5
```

## Memory 
Memory is handled with reference counting, there is no garbage collection. You can free objects that you don't need anymore by setting them to nil.
```javascript
baseWidth = 400
halfWidth = baseWidth * 0.5
doubleWidth = baseWidth * 2.0
baseWidth = nil
```

## Performance

As tui parses and immediately runs hand written script code in a single pass, when reading data files or in cases with few loops or functions, it should perform just as well as, or better than the alternatives. 

Where tui might be noticeably slower than lua, is when writing high performance loops and functions. To make up for that it is much easier to call out to your own C++ functions where needed.

# Using tui in C++

You can add the files to your c++ project, run a script, and access the output easily. The only dependency is glm for vector math support.

```c++
#include "TuiScript.h"

int main()
{
    TuiTable* table = (TuiTable*)TuiRef::load("config.tui"); // load a JSON-like config file
    std::string playerName = table->getString("playerName"); // get a string
    double playDuration = table->getDouble("playDuration"); // get a number
    table->setDouble("playDuration", playDuration + 1.0); // set a number
    table->saveToFile("config.tui"); // save in a human readable JSON-like format
    table->release(); // cleanup

    TuiRef* scriptRunResult = TuiRef::runScriptFile("script.tui"); // run a script file
    scriptRunResult->debugLog(); // print the result
    scriptRunResult->release(); // cleanup
}

```

With no virtual machine, and no bindings required to access data in C++, all of the data and script state is stored in a public std::map or std::vector under the hood. Scripts and tables are parsed together and are treated the same. Each character is simply parsed one by one in a single phase, with data loaded immediately. Functions and for loops are serialized and run as required.

This means tui can solve two problems. You can use it as a scripting language, that also happens to have built in serialization support from/to both binary (todo) and human readable data formats.

Or you can use it as a data format and serialization library. Where you might have used XML, JSON, plists, or other formats for storing and sharing data, tui reads JSON out of the box, with all the power of the scripting language.

# onSet notifications for tables in C++

Instead of implementing get/set bindings strictly as functions, with tui you can use the "onSet" notification to update any derived data on the C++ side when tui state changes. This encorages you to keep as much state as possible in tui objects, in many cases making it faster and easier to serialize and otherwise work with that data.

```c++

void View::init()
{
    stateTable = new TuiTable(nullptr);
    stateTable->onSet = [this](TuiRef* table, const std::string& key, TuiRef* value) {
        tableKeyChanged(key, value);
    };
    stateTable->setVec2("size", size); // this will call tableKeyChanged
}

void View::tableKeyChanged(const std::string& key, TuiRef* valueRef)
{
    if(key == "size")
    {
        switch (value->type()) {
            case Tui_ref_type_VEC2:
                setSize(((TuiVec2*)valueRef)->value); // value is a dvec2
                break;
            default:
                error("Expected vec2");
                break;
        }
    }
}


```

# What tui is not
tui is not finished!

It should not be used in production environments yet. Binary formats are not yet implemented, there is a lot of optimization work yet to do, error reporting has a few issues, and there is still missing functionality.

tui has 'objects', as tables. However there is no concept of 'self/this', and no direct support for inheritance or classes in general. Further OOP support is not planned.

tui will stay small, and won't add a lot of support for built in system functionality. If this functionality is desired, it can easily be added on the C++ side by registering your own functions.

There are no bindings for languages other than C++ at present.

# More about the motivations and ideologies behind tui

Most scripting languages run a virtual machine, as this is the best approach for the highest performance running that script code.

However, in many cases we don't really need such high performance from a scripting language. For low complexity operations, the speed that the script loads, and how it handles data is often more important. And at the other end, even with the fastest scripting language, if we are working with the behaviors of hundreds or thousands of complex objects every frame, we are still often best to use C++.

High performance script code is of course very disirable, but the VM can create problems when managing state between the host and the scripting enviornment. Transferring data between the two can be slow, and for the programmer, bindings can often be quite difficult to implement.

Tui is fast enough for all the high level code, all the structure, the stuff you want to be able to change easily. And then when you need the performance, tui makes it easy to pull anything out and run it with C++ instead. Your C++ functions have all the data right there, and you're just a function call away from it being serialized and ready to save or send over a network.

This is a one-man project, though I'm keen for help! My name is Dave Frampton, I made the games Sapiens (C++/Lua) and The Blockheads (Objective C) using my own custom engines.

tui was initially created to serialize and share data in C++ for my games, so it started life as a quick little JSON parser. Very soon though, missing the power of lua, I started adding variables and functions.

I feel tui could be useful for a lot of people for many different purposes. I don't desire to keep it only for myself or to profit from it, so I'm making it open source. 

Hopefully it is useful, and if you find a bug or have a feature request please feel free to open an issue, but please do fork this and send it in new directions too!

-- Dave