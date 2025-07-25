
#ifndef __TuiBuiltInFunctions__
#define __TuiBuiltInFunctions__

class TuiTable;

namespace Tui {

TuiTable* createRootTable();
void addStringTable(TuiTable* rootTable);
void addTableTable(TuiTable* rootTable);
void addMathTable(TuiTable* rootTable);
void addFileTable(TuiTable* rootTable);
void addDebugTable(TuiTable* rootTable);

}

#endif
