
#ifndef __TuiBuiltInFunctions__
#define __TuiBuiltInFunctions__

#include <string>
#include <chrono>

class TuiTable;
class TuiFunction;

namespace Tui {


//TODO WARNING! This is not fully implemented, not to be trusted yet
TuiTable* initSafeRootTable(TuiFunction* permissionCallbackFunction = nullptr, const std::string& sandBoxDir = ""); //pass permissionCallbackFunction to selectively give permission for some sensitive functions. Pass sandbox dir to restrict all file operations to within that directory.
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

TuiTable* initRootTable();

inline double nowTime()
{
    auto nowTime = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(nowTime.time_since_epoch()).count() / 1000000.0;
}


static inline TuiTable* getRootTable()
{
    thread_local TuiTable* rootTable = Tui::initRootTable();
    return rootTable;
}


void addBaseFunctions(TuiTable* rootTable, TuiFunction* permissionCallbackFunction = nullptr);
void addStringTable(TuiTable* rootTable);
void addTimeTable(TuiTable* rootTable);
void addTableTable(TuiTable* rootTable);
void addMathTable(TuiTable* rootTable);
void addFileTable(TuiTable* rootTable, const std::string& sandBoxDir = "");
void addDebugTable(TuiTable* rootTable);

}

#endif
