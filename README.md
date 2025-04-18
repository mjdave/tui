# mjscript

mjscript is a scripting and serialization library for c++.

Designed by a game developer for practicality and performance, mjscript combines a key/value storage data format in a human readable format similar to JSON, with a powerful scripting language and interpreter similar to lua.

With no virtual machine, and no bindings required in C++, all of the data and script state is stored in a std::map or std::vector under the hood. Scripts and tables are parsed in the same way and are treated the same. Each character is simply parsed one by one in a single phase, with data loaded into a data tree immediately, and functions stored and called as required.

This means you can use it as a scripting language, that just happens to also have built in serialization support for both binary and human readable (JSON-like) formats.

Or you can use it as a data format and serialization library. Where you might have used XML, JSON, plists, or other ways of storing and sharing data, this is a new option that reads JSON out of the box, but adds a whole bunch of new features.

For example:

# Variables & Expressions
Any previously assigned value can be accessed by the key name. This can be used to define constants to do math for later values.
```
{
    baseWidth = 400,
    halfWidth = baseWidth * 0.5,
    doubleWidth = baseWidth * 2.0,
}
```
# Vectors
The only dependency of mjscript is glm, which currently exposes vec2, vec3, vec4, and mat3 types, as well as a number of builtin math functions
```
{
    size = vec2(400,200),
    color = vec4(0.0,1.0,0.0,1.0),
    halfSize = size * 0.5,
}
```
# Commas and quotes optional, arrays can use '{'
```
array = {
    ThisIsTheFirstObjectWithIndexZero
    "This is the second object with spaces needs quotes"
    "note we don't have any commas, but we can if we want to",
    surprise = "We can also mix array values with keyed values like in lua"
}
```

# Scope
All variables are limited in scope to the block they are declared in and all child blocks. So variables declared at the top level are effectively globals. You can access children with the '.' sytax.
```
{
    varA = 10

    tableA = {
        varB = varA     # we can access varA directly, so varB is now 10
        varA = 20       # a new local varA is now 20
        varB = varA     # varB is now the value of the local varA:20
    }

    varC = varA         # varC is now 10
    varC = tableA.varA  # varC is now 20
}
```


# Memory 
Memory is handled with reference counting, there is no garbage collection. You can free objects by setting them to nil.
```
{
    baseWidth = 400,
    halfWidth = baseWidth * 0.5,
    doubleWidth = baseWidth * 2.0,
    baseWidth = nil,
}
```