#ifndef C_BATTERYPACKOPTIMIZER_BATTERY_INVENTORY_H
#define C_BATTERYPACKOPTIMIZER_BATTERY_INVENTORY_H

#include <algorithm>
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include "Battery.h"
#include "Battery.h"

// Represents a battery inventory.
class BatteryInventory {
public:
    BatteryInventory() {} // Empty constructor

    // Function to push Battery cell object to cells vector, updates uniqueCells bool variable.
    void AddCell(const Battery &battery) {
        cells.push_back(battery);
        if (!uniqueByCapacity && cells.size() > 0 && cells[0].GetCapacity() != battery.GetCapacity()) {
            uniqueByCapacity = true;
        }
        if (!uniqueByResistance && cells.size() > 0 && cells[0].GetResistance() != battery.GetResistance()) {
            uniqueByResistance = true;
        }
    }

    int GetCellCount() const { return cells.size(); }

    // Sorts in descending order by capacity.
    void Sort() { std::sort(cells.begin(), cells.end(), std::greater<Battery>()); }

    // Returns true if atleast one cell is different from the rest, false if all are the same.
    bool CheckUniqueByCapacity() const { return uniqueByCapacity; }
    bool CheckUniqueByResistance() const { return uniqueByResistance; }

    // Returns specific index cell.
    Battery GetCell(int index) const {
        if (index < 0 || index >= cells.size()) {
            throw std::out_of_range("index out of range");
        }
        return cells[index];
    }

    // Returns a vector of the top numBatteries cells based on a weighted score calculated from capacity and resistance.
    std::vector<Battery> GetTopCells(int numBatteries, double wCap, double wRes) {
        if (cells.empty()) return {};

        double maxCap = 1.0;
        double maxRes = 1.0;

        for (int i = 0; i < cells.size(); i++) {
            if (cells[i].GetCapacity() > maxCap) {
                maxCap = cells[i].GetCapacity();
            }
            if (cells[i].GetResistance() > maxRes) {
                maxRes = cells[i].GetResistance();
            }
        }

        std::sort(cells.begin(), cells.end(), [&](const Battery& a, const Battery& b) {
            double scoreA = (wCap * (a.GetCapacity() / maxCap)) - (wRes * (a.GetResistance() / maxRes));
            double scoreB = (wCap * (b.GetCapacity() / maxCap)) - (wRes * (b.GetResistance() / maxRes));

            return scoreA > scoreB;
        });

        int minCount = std::min(numBatteries, (int)cells.size());
        return std::vector<Battery>(cells.begin(), cells.begin() + minCount);
    }

    // Removes and returns all cells with resistance greater than maxResistance from the inventory.
    std::vector<Battery> GetAndRemoveBadResistanceCells(double maxResistance) {
        std::vector<Battery> badCells;
        auto it = cells.begin();
        while (it != cells.end()) {
            if (it->GetResistance() > maxResistance) {
                badCells.push_back(*it);
                it = cells.erase(it);
            } else {
                ++it;
            }
        }
        return badCells;
    }

private:
    std::vector<Battery> cells; // Battery vector for storing cells.
    bool uniqueByCapacity = false; // True if at least one cell has a different capacity than the rest, false if all are the same.
    bool uniqueByResistance = false; // True if at least one cell has a different internal resistance than the rest, false if all are the same.
};

#endif
