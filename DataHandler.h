#ifndef C_BATTERYPACKOPTIMIZER_DATAHANDLER_H
#define C_BATTERYPACKOPTIMIZER_DATAHANDLER_H

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include "Battery.h"
#include "BatteryInventory.h"
#include "PackManager.h"

class DataHandler {
public:
    DataHandler() {}

    void ReadDataFromString(const std::string& csvContent, BatteryInventory &batteryPack) {
        std::istringstream reader(csvContent);
        std::string line;
        std::getline(reader, line); 

        while (std::getline(reader, line)) {
            if(line.empty()) continue;
            if(!line.empty() && line.back() == '\r') line.pop_back();

            std::istringstream ss(line);
            std::string idString, capacityString, internalResistanceString, manufacturerString, conditionString;

            if (!std::getline(ss, idString, ';')) continue;
            if (!std::getline(ss, capacityString, ';')) continue;
            if (!std::getline(ss, internalResistanceString, ';')) continue;
            if (!std::getline(ss, manufacturerString, ';')) continue;
            std::getline(ss, conditionString, ';');
            
            if(!conditionString.empty() && conditionString.back() == '\r') conditionString.pop_back();

            int id = std::stoi(idString);
            int capacity = std::stoi(capacityString);
            double internalResistance = internalResistanceString.empty() ? 0.0 : std::stod(internalResistanceString);

            Battery battery(id, capacity, internalResistance, manufacturerString, conditionString.empty() ? "N/A" : conditionString);
            batteryPack.AddCell(battery);
        }
    }

    std::string FormatDouble(double val) const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << val;
        return ss.str();
    }

    std::string GetSummaryHTML(const PackManager &packManager) {
        std::ostringstream out;
        double variance = packManager.CalculateCapacityVariancePercentage();
        double score = 100.0 - variance;
        if (score < 0.0) score = 0.0;
        if (score > 100.0) score = 100.0;

        out << "<div class='meter-box'><span>Optimization Score</span><strong class='highlight'>" << FormatDouble(score) << "%</strong></div>\n";
        out << "<div class='meter-box'><span>Expected Energy</span><strong>" << FormatDouble(packManager.CalculateTotalPackEnergy()) << " Wh</strong></div>\n";
        out << "<div class='meter-box'><span>Usable Capacity</span><strong>" << packManager.MinCapacity() << " mAh</strong></div>\n";
        out << "<div class='meter-box'><span>Max Capacity Limit</span><strong>" << packManager.MaxCapacity() << " mAh</strong></div>\n";
        out << "<div class='meter-box'><span>Capacity Variance</span><strong>" << FormatDouble(variance) << "%</strong></div>\n";
        out << "<div class='meter-box'><span>Total Resistance</span><strong>" << FormatDouble(packManager.GetTotalResistance()) << " m&Omega;</strong></div>\n";
        out << "<div class='meter-box'><span>Resistance Variance</span><strong>" << FormatDouble(packManager.GetResistanceVariancePercentage()) << "%</strong></div>\n";
        return out.str();
    }

    std::string CompactCellOutputHTML(const PackManager &packManager) {
        std::ostringstream out;
        int series = packManager.GetSeries();
        int parallel = packManager.GetParallel();
        out << "<table class='styled-table'>\n<thead>\n<tr><th>P \\ S</th>";
        for (int i = 0; i < series; i++) out << "<th>S" << (i + 1) << "</th>";
        out << "</tr>\n</thead>\n<tbody>\n";
        for (int p = 0; p < parallel; p++) {
            out << "<tr><td><strong>Cell " << (p + 1) << "</strong></td>";
            for (int s = 0; s < series; s++) {
                Battery b = packManager.GetCell(s, p);
                out << "<td>" << b.GetCapacity() << " mAh<br>" << FormatDouble(b.GetResistance()) << " m&Omega;</td>";
            }
            out << "</tr>\n";
        }
        out << "<tr class='total-row'><td><strong>Total</strong></td>";
        for (int s = 0; s < series; s++) {
            out << "<td>" << packManager.GetIndexParallelCapacity(s) << " mAh<br>" << FormatDouble(packManager.GetIndexParallelTotalResistance(s)) << " m&Omega;</td>";
        }
        out << "</tr>\n</tbody>\n</table>\n";
        return out.str();
    }

    std::string DetailedCellOutputHTML(const PackManager &packManager) {
        std::ostringstream out;
        int series = packManager.GetSeries();
        int parallel = packManager.GetParallel();
        out << "<div class='detailed-grid'>";
        for (int i = 0; i < series; i++) {
            out << "<div class='detailed-block'>\n<h4>Parallel Group " << (i + 1) << "</h4>\n";
            out << "<table class='styled-table'>\n<thead>\n<tr><th>ID</th><th>Capacity</th><th>Resistance</th><th>Manufacturer</th><th>Condition</th></tr>\n</thead>\n<tbody>\n";
            for (int j = 0; j < parallel; j++) {
                Battery b = packManager.GetCell(i, j);
                out << "<tr><td>" << b.GetId() << "</td><td>" << b.GetCapacity() << " mAh</td><td>" << FormatDouble(b.GetResistance()) << " m&Omega;</td><td>" << b.GetManufacturer() << "</td><td>" << b.GetCondition() << "</td></tr>\n";
            }
            out << "</tbody>\n</table>\n";
            out << "<div class='block-stats'>Total Capacity: <b>" << packManager.GetIndexParallelCapacity(i) << " mAh</b> | Total Resistance: <b>" << FormatDouble(packManager.GetIndexParallelTotalResistance(i)) << " m&Omega;</b></div>\n";
            out << "</div>\n";
        }
        out << "</div>";
        return out.str();
    }
};

#endif
