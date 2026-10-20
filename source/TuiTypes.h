
#ifndef TuiTypes_h
#define TuiTypes_h

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
namespace tui
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
    using userData = TuiPointer<TuiUserData>;
    using statement = TuiPointer<TuiStatement>;
    using ifStatement = TuiPointer<TuiIfStatement>;
    using forExpressionsStatement = TuiPointer<TuiForExpressionsStatement>;
    using forContainerLoopStatement = TuiPointer<TuiForContainerLoopStatement>;
    using string = TuiPointer<TuiString>;
    using table = TuiPointer<TuiTable>;

    inline tui::function toFunction(const tui::ref& tuiRef)
    {
        return tui::cast<TuiFunction>(tuiRef);
    }

    inline tui::number toNumber(const tui::ref& tuiRef)
    {
        return tui::cast<TuiNumber>(tuiRef);
    }

    inline tui::number8 toNumber8(const tui::ref& tuiRef)
    {
        return tui::cast<TuiNumber8>(tuiRef);
    }

    inline tui::number16 toNumber16(const tui::ref& tuiRef)
    {
        return tui::cast<TuiNumber16>(tuiRef);
    }

    inline tui::number32 toNumber32(const tui::ref& tuiRef)
    {
        return tui::cast<TuiNumber32>(tuiRef);
    }

    inline tui::number64 toNumber64(const tui::ref& tuiRef)
    {
        return tui::cast<TuiNumber64>(tuiRef);
    }

    inline tui::boolean toBool(const tui::ref& tuiRef)
    {
        return tui::cast<TuiBool>(tuiRef);
    }

    inline tui::vec2 toVec2(const tui::ref& tuiRef)
    {
        return tui::cast<TuiVec2>(tuiRef);
    }

    inline tui::vec3 toVec3(const tui::ref& tuiRef)
    {
        return tui::cast<TuiVec3>(tuiRef);
    }

    inline tui::vec4 toVec4(const tui::ref& tuiRef)
    {
        return tui::cast<TuiVec4>(tuiRef);
    }

    inline tui::mat3 toMat3(const tui::ref& tuiRef)
    {
        return tui::cast<TuiMat3>(tuiRef);
    }

    inline tui::string toString(const tui::ref& tuiRef)
    {
        return tui::cast<TuiString>(tuiRef);
    }

    inline tui::table toTable(const tui::ref& tuiRef)
    {
        return tui::cast<TuiTable>(tuiRef);
    }
}

#endif // TuiTypes_h
