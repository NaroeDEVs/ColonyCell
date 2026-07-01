#ifndef C_BATTERYPACKOPTIMIZER_BATTERY_H
#define C_BATTERYPACKOPTIMIZER_BATTERY_H

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

// Represents a single battery cell.
class Battery {
public:
    // Empty constructor with default parameters
    Battery() {
        id = 0;
        capacity = 0;
        manufacturer = "N/A";
        condition = "N/A";
        internalResistance = 0;
    }

    // Constructor with parameters
    Battery(int id, int cap, double ir = 0, const std::string &man = "N/A", const std::string &cond = "N/A") {
        this->id = id;
        this->capacity = cap;
        this->manufacturer = man;
        this->condition = cond;
        this->internalResistance = ir;
    }

    std::string PrintStatus() const { return Status(); }
    int GetId() const { return id; }
    int GetCapacity() const { return capacity; }
    std::string GetManufacturer() const { return manufacturer; }
    std::string GetCondition() const { return condition; }
    double GetResistance() const { return internalResistance; }

    // Compares 2 cells by capacity.
    friend bool operator<(const Battery &lhs, const Battery &rhs) {
        return lhs.capacity < rhs.capacity;
    }

    // Compares 2 cells by capacity.
    friend bool operator>(const Battery &lhs, const Battery &rhs) {
        return lhs.capacity > rhs.capacity;
    }

private:
    int id; // Battery individual identifier
    int capacity; // Capacity in mAh
    std::string manufacturer; // Manufacturer name
    std::string condition; // Battery condition
    double internalResistance; // Internal resistance in milliohms (mΩ).

    // Returns formatted string with battery info.
    // Use ostream formatting (std::ostringstream) instead of std::format to
    // avoid requiring C++20 <format> support in all toolchains / language servers.
    std::string Status() const {
        std::ostringstream oss;
        oss << std::left << std::setw(17) << id << " | ";
        oss << std::left << std::setw(17) << capacity << " | ";
        oss << std::left << std::setw(24) << std::fixed << std::setprecision(2) << internalResistance << " | ";
        oss << std::left << std::setw(17) << manufacturer << " | ";
        oss << condition;
        return oss.str();
    }
};

#endif
