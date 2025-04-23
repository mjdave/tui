# This is mjscript

mjscript is a small, cross platform, open source scripting and serialization library for C++.

Created by a solo game developer to be fast, simple, and easy to use and modify, mjscript combines a key/value storage data format in a human readable format similar to JSON, with a powerful scripting language and interpreter similar to lua.

mjscript is dynamically typed, fast, flexible, and lightweight. Data is shared in memory between mjscript and the host c++ program, allowing high performance read/writes in both enviornments.

You can add the files to your c++ project, run a script, and access the output easily. The only dependency is glm for vector math support.

```c++
#include "MJScript.h"

int main()
{
    MJTable* table = MJTable::initWithHumanReadableFilePath("config.mjh"); // load a JSON-like config file
    std::string playerName = table->getString("playerName"); //get a string
    double playDuration = table->getDouble("playDuration"); //get a number
    table->setDouble("playDuration", playDuration + 1.0); //set a number
    table->saveToFile("config.mjh"); //save in a human readable JSON-like format
    table->release(); //cleanup
    
    MJRef* scriptRunResult = MJTable::runScriptFile("script.mjh"); //run a script file
    scriptRunResult->debugLog(); //print the result
    scriptRunResult->release(); //cleanup
}

```

With no virtual machine, and no bindings required in C++, all of the data and script state is stored in a public std::map or std::vector under the hood. Scripts and tables are parsed together and are treated the same. Each character is simply parsed one by one in a single phase, with data loaded immediately. Functions are quickly serialized and called as required.

This means mjscript can solve two problems. You can use it as a fast and small scripting language, that also happens to have built in serialization support from/to both binary and human readable data formats.

Or you can use it as a data format and serialization library. Where you might have used XML, JSON, plists, or other ways of storing and sharing data, mjscript reads JSON out of the box, while adding a bunch of new features.

# Features & Usage/Examples

## A single object is valid
This is a valid mjscript file:
```lua
42
```
If you loaded it, the MJRef* you got would be an MJNumber with the value of 42. //todo

This is also valid file that will give us an MJTable with array elements:
```lua
42, coconut, vec3(1,2,3), true, nil, {x=10}
```

## Reads JSON
This is the same data in JSON. mjscript can read this too, as it treats ":" and "=" the same, "{" and "[" the same, and doesn't require commas or quotes, but doesn't mind them either.
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
    nil,
    {
        "x":10
    }
]
```

Whitespace is ignored, however newlines are treated like commas, unless quoted or within a bracketed expression
```javascript
array = {
    ThisIsTheFirstObjectWithIndexZero
    "This is the second object with spaces, so it needs quotes"
    "We don't have any commas after each variable, but we can if we want to.
Also, quotes and brackets allow us to use newlines",
    surprise = "We can also mix array values with keyed values like in lua"
}

//this is a comment, anything after '//' will be skipped by the parser until the next line
#this is also a comment
```

## Adds variables, if statements & Expressions
Any previously assigned value can be accessed by the key name. This can be used to define constants to do math for later values.
```javascript
width = 400,
height = 200,
halfWidth = width * 0.5,
doubleWidth = width * 2.0,

//if statements can be put right in there with your data
if(width > 300) // brackets optional, 'if width > 300' is also valid
{
    height = height * 2.0
}
else // 'else if' and 'elseif' are both valid too
{
    height = height * 0.5
}
```
## Functions
Functions are still a work in progress, but we have value assignments, if/else statements, and basic expressions so far.
```javascript
addTariff = function(base)
{
    tariff = 145 / 100
    if(random(2) > 0)
    {
        tariff = 245 / 100
    }
    return (base * (1.0 + tariff))
}

costOfTV = addTariff(500)
costOfPlaystation = addTariff(400)
```

You can supply your own functions in C++ easily
```c++

```

## Vectors
The only dependency of mjscript is glm, which currently exposes vec2, vec3, vec4, and mat3 types, as well as a number of builtin math functions
```javascript
size = vec2(400,200)
color = vec4(0.0,1.0,0.0,1.0)
halfSize = size * 0.5
```

## Scope
Every table and every function has its own scope for variable creation/assignment.

All variable assignments create new local within the enclosing function or table. They are limited in scope to the table/function they are declared in, and readable for all child tables/functions, and also read/writable via the `'.'` syntax to access children and `'..'` to access parents.

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
    thisFuncDoesntExist() // error examples/scope.mjh:27:attempt to call missing function: thisFuncDoesntExist()
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

# What mjscript is not
mjscript is not finished! It should not be used in production environments yet. Binary formats and for statements in particular are not yet implemented.

The interpreter only builds for macos at present, but getting it building on other platforms will happen soon. mjscript is cross platform, tested on windows, macos and linux.

mjscript has 'objects', as tables. However there is no concept of 'self/this', and no direct support for inheritance or classes in general.

mjscript will stay small, and won't add a lot of support for built in system functionality. If this functionality is desired, it can easily be added on the C++ side by registering your own functions.

There are no bindings for languages other than C++ at present, however porting to other languages will not be a difficult job.

# More about the motivations and ideologies behind mjscript
This is a one-man project (to start with), my name is Dave Frampton, I made the games Sapiens (C++/Lua) and The Blockheads (Objective C) using my own custom engines.

mjscript was initially created to serialize and share data in C++ for my games, so it started life as a quick little JSON parser. Very soon though, missing the power of lua, I started adding variables and functions.

During this process, I have taken a performance-centric approach, while trying to keep it simple and clear. As it parses and immediately runs hand written script code in a single pass, in many cases where a script is just read, or a config file loaded, it should perform faster than the alternatives. Where mjscript might not be as fast is when you have a lot of function calls with long variable names, but we'll see how it goes.

I feel this could be useful for a lot of people. I don't desire to keep it only for myself or to profit from it, and I wouldn't mind some help, so I'm making it open source. 

Hopefully it is useful, and if you find a bug or have a feature request please feel free to open an issue, but please do fork this and send it in new directions too!

-- Dave