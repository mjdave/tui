
#ifndef __TuiBuiltInFunctions__
#define __TuiBuiltInFunctions__

#include <string>
#include <chrono>

#include "TuiTypes.h"
#include "TuiPointer.h"

class TuiTable;
class TuiFunction;

namespace tui {


//TODO WARNING! This is not fully implemented, not to be trusted yet
tui::table initSafeRootTable(const tui::function& permissionCallbackFunction = nullptr, const std::string& sandBoxDir = ""); //pass permissionCallbackFunction to selectively give permission for some sensitive functions. Pass sandbox dir to restrict all file operations to within that directory.
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

tui::table initRootTable();

inline double nowTime()
{
    auto nowTime = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(nowTime.time_since_epoch()).count() / 1000000.0;
}


static inline const tui::table& getRootTable()
{
    thread_local tui::table rootTable = tui::initRootTable();
    return rootTable;
}


void addBaseFunctions(const tui::table& rootTable, const tui::function& permissionCallbackFunction = nullptr);
void addStringTable(const tui::table& rootTable);
void addTimeTable(const tui::table& rootTable);
void addTableTable(const tui::table& rootTable);
void addMathTable(const tui::table& rootTable);
void addFileTable(const tui::table& rootTable, const std::string& sandBoxDir = "");
void addDebugTable(const tui::table& rootTable);

//utility functions

double random();
double random(uint32_t seed);
double random(const std::string& seed);

}

#endif
