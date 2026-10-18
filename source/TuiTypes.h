
#ifndef TuiTypes_h

#include "TuiPointer.h"

// Forward declare the Tui types that are lifetime managed by TuiPointer
class TuiFunction;
class TuiNumber;
class TuiNumber8;
class TuiNumber16;
class TuiNumber32;
class TuiNumber64;
class TuiBool;
class TuiVec2;
class TuiVec3;
class TuiVec4;
class TuiMat3;
class TuiRef;
class TuiNil;
class TuiUserData;
class TuiStatement;
class TuiIfStatement;
class TuiForExpressionsStatement;
class TuiForContainerLoopStatement;
class TuiString;
class TuiTable;

// Create simply type definitions for TuiPointer managed types
namespace Tui
{
    using function = TuiPointer<TuiFunction>;
    using number = TuiPointer<TuiNumber>;
    using number8 = TuiPointer<TuiNumber8>;
    using number16 = TuiPointer<TuiNumber16>;
    using number32 = TuiPointer<TuiNumber32>;
    using number64 = TuiPointer<TuiNumber64>;
    using boolean = TuiPointer<TuiBool>;
    using vec2 = TuiPointer<TuiVec2>;
    using vec3 = TuiPointer<TuiVec3>;
    using vec4 = TuiPointer<TuiVec4>;
    using mat3 = TuiPointer<TuiMat3>;
    using ref = TuiPointer<TuiRef>;
    using nil = TuiPointer<TuiNil>;
    using userData = TuiPointer<TuiUserData>;
    using statement = TuiPointer<TuiStatement>;
    using ifStatement = TuiPointer<TuiIfStatement>;
    using forExpressionsStatement = TuiPointer<TuiForExpressionsStatement>;
    using forContainerLoopStatement = TuiPointer<TuiForContainerLoopStatement>;
    using string = TuiPointer<TuiString>;
    using table = TuiPointer<TuiTable>;
}

#endif // TuiTypes_h
