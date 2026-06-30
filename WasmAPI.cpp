#include <emscripten/bind.h>
#include <string>
#include "BatteryInventory.h"
#include "PackManager.h"
#include "DataHandler.h"

std::string OptimizePack(std::string csvData, int series, int parallel, double wCap, double wRes, double nomV, double maxV) {
    BatteryInventory AllBateries;
    DataHandler dataHandler;

    dataHandler.ReadDataFromString(csvData, AllBateries);

    if (series * parallel > AllBateries.GetCellCount()) {
        return "ERROR|||Not enough batteries. You need " + std::to_string(series * parallel) + ".";
    }

    PackManager allPacks;
    allPacks.SetSize(series, parallel);
    allPacks.SetVoltages(nomV, maxV);
    allPacks.SetOptimizationWeights(wCap / 100.0, wRes / 100.0);
    
    allPacks.PackWithOptimization(AllBateries);
    allPacks.HillClimbOptimization();

    std::string summary = dataHandler.GetSummaryHTML(allPacks);
    std::string compact = dataHandler.CompactCellOutputHTML(allPacks);
    std::string detailed = dataHandler.DetailedCellOutputHTML(allPacks);

    return summary + "|||" + compact + "|||" + detailed;
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("OptimizePack", &OptimizePack);
}