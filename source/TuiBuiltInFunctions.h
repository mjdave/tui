
#ifndef __TuiBuiltInFunctions__
#define __TuiBuiltInFunctions__

#include <string>
#include <chrono>

#include "TuiTypes.h"
#include "TuiPointer.h"

class TuiTable;
class TuiFunction;

namespace Tui {


//TODO WARNING! This is not fully implemented, not to be trusted yet
Tui::table initSafeRootTable(const Tui::function& permissionCallbackFunction = nullptr, const std::string& sandBoxDir = ""); //pass permissionCallbackFunction to selectively give permission for some sensitive functions. Pass sandbox dir to restrict all file operations to within that directory.
// eg. in tui: permissionCallbackFunction = function(functionName, args, hasPermissionCallback) {
//      if(functionName == "system")
//      {
//          hasPermissionCallback(false)
//      }
//      else
//      {
//          hasPermissionCallback(true)
//      }
//  }

Tui::table initRootTable();

inline double nowTime()
{
    auto nowTime = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(nowTime.time_since_epoch()).count() / 1000000.0;
}


static inline const Tui::table& getRootTable()
{
    thread_local Tui::table rootTable = Tui::initRootTable();
    return rootTable;
}


void addBaseFunctions(const Tui::table& rootTable, const Tui::function& permissionCallbackFunction = nullptr);
void addStringTable(const Tui::table& rootTable);
void addTimeTable(const Tui::table& rootTable);
void addTableTable(const Tui::table& rootTable);
void addMathTable(const Tui::table& rootTable);
void addFileTable(const Tui::table& rootTable, const std::string& sandBoxDir = "");
void addDebugTable(const Tui::table& rootTable);

//utility functions

double random();
double random(uint32_t seed);
double random(const std::string& seed);

}

#endif
