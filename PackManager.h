#ifndef C_BATTERYPACKOPTIMIZER_PACKMANAGER_H
#define C_BATTERYPACKOPTIMIZER_PACKMANAGER_H

#include <string>
#include <vector>
#include "BatteryInventory.h"
#include "Battery.h"
#include "ParallelGroup.h"
#include <limits>
#include <climits>
#include <cstdint>

// Represent whole pack of parallel groups
class PackManager {
public:
    PackManager() = default;

    // sets the size of the pack in series and parallel
    void SetSize(int series, int parallel) {
        seriesCount = series;
        parallelCount = parallel;
        seriesGroups.resize(seriesCount);
    }

    // sets single cell (Parallel group) nominal and maximum voltages
    void SetVoltages(double NominalCellVoltage, double MaxSingleCellVoltage) {
        nominalCellVoltage = NominalCellVoltage;
        maxSingleCellVoltage = MaxSingleCellVoltage;
    }

    // sets the weights for capacity and resistance optimization.
    void SetOptimizationWeights(double capacityWeight, double resistanceWeight) {
        selectedWcapacity = capacityWeight;
        selectedWresistance = resistanceWeight;
    }

    // ---- Exemption stage ----
    // Packs the cells into the pack without any optimization, just filling groups in order.
    // Used in case of all cells being same capacity to reduce complexity and time of packing.
    void PackWithoutOptimization(BatteryInventory &batteryInventory) {
        int k = 0;
        for (int i = 0; i < seriesCount; i++) {
            for (int j = 0; j < parallelCount; j++) {
                seriesGroups[i].AddCell(batteryInventory.GetCell(k));
                k++;
            }
        }
    }

    // ---- 1st packing stage: Greedy packing algorithm. ----
    // Packs by taking top series*parallel sorted cells and assigns them to whichever next parallel group has lowest capacity.
    void PackWithOptimization(BatteryInventory &batteryInventory) {
        std::vector<Battery> bestCells = batteryInventory.GetTopCells(seriesCount * parallelCount, selectedWcapacity,
                                                                      selectedWresistance);

        for (int i = 0; i < seriesCount * parallelCount; i++) {
            Battery battery = bestCells[i];
            int groupIndex = GetLowestCapacityGroupIndex();
            if (groupIndex != -1) {
                seriesGroups[groupIndex].AddCell(battery);
            }
        }
    }

    int GetSeries() const { return seriesCount; }
    int GetParallel() const { return parallelCount; }

    Battery GetCell(int seriesIndex, int parallelIndex) const {
        return seriesGroups[seriesIndex].GetCell(parallelIndex);
    }

    // Single parallel group info getters.
    int GetIndexParallelCapacity(int seriesIndex) const { return seriesGroups[seriesIndex].GetTotalCapacity(); }
    int GetIndexParallelMaxCapacity(int seriesIndex) const { return seriesGroups[seriesIndex].GetMaxCapacity(); }
    int GetIndexParallelMinCapacity(int seriesIndex) const { return seriesGroups[seriesIndex].GetMinCapacity(); }

    double GetIndexParallelAverageCapacity(int seriesIndex) const {
        return seriesGroups[seriesIndex].GetAverageCapacity();
    }

    double GetIndexParallelVariancePercentage(int seriesIndex) const {
        return seriesGroups[seriesIndex].GetCapacityVariancePercentage();
    }

    double GetIndexParallelTotalPackVoltage(int seriesIndex) const {
        return seriesGroups[seriesIndex].GetTotalPackEnergy(nominalCellVoltage);
    }

    double GetIndexParallelTotalResistance(int seriesIndex) const {
        return seriesGroups[seriesIndex].GetTotalResistance();
    }

    double GetIndexParallelMaxResistance(int seriesIndex) const { return seriesGroups[seriesIndex].GetMaxResistance(); }
    double GetIndexParallelMinResistance(int seriesIndex) const { return seriesGroups[seriesIndex].GetMinResistance(); }

    double GetIndexParallelAverageResistance(int seriesIndex) const {
        return seriesGroups[seriesIndex].GetAverageResistance();
    }

    double GetIndexParallelResistanceVariancePercentage(int seriesIndex) const {
        return seriesGroups[seriesIndex].GetResistanceVariancePercentage();
    }

    // Returns maximum parallel group total capacity.
    int MaxCapacity() const {
        if (seriesCount > 0) {
            return MinMaxIndexes().maxCapacity;
        }
        return 0;
    }

    // Returns minimum parallel group total capacity.
    int MinCapacity() const {
        if (seriesCount > 0) {
            return MinMaxIndexes().minCapacity;
        }
        return 0;
    }

    // Returns whole pack capacity (sum of all parallel groups capacities).
    int CalculateTotalCapacity() const {
        int totalCapacity = 0;
        for (int i = 0; i < seriesCount; i++) {
            totalCapacity += seriesGroups[i].GetTotalCapacity();
        }
        return totalCapacity;
    }

    // Returns average parallel group capacity.
    double CalculateAverageCapacity() const {
        int totalCapacity = CalculateTotalCapacity();
        return static_cast<double>(totalCapacity) / seriesCount;
    }

    // Returns percentage variance between the weakest and strongest parallel group, calculated as (max - min) / average * 100.
    double CalculateCapacityVariancePercentage() const {
        double variance = 0;
        double averageCap = CalculateAverageCapacity();
        auto extremes = MinMaxIndexes();
        int biggestDifference = extremes.maxCapacity - extremes.minCapacity;
        variance = static_cast<double>(biggestDifference) / averageCap;
        return variance * 100;
    }

    // Returns actual usable pack capacity, which is the capacity of the weakest parallel group, since it limits the whole pack.
    int GetUsablePackCapacity() const {
        return MinCapacity();
    }

    // Calculates true usable energy in Watt-hours (Wh)
    double CalculateTotalPackEnergy() const {
        double usableAh = static_cast<double>(GetUsablePackCapacity()) / 1000.0;
        double totalPackVoltage = nominalCellVoltage * seriesCount;
        return usableAh * totalPackVoltage;
    }

    // ---- 2nd packing stage: Hill climbing optimization algorithm. ----
    // Attempts to swap cells between parallel groups to minimize the difference in total capacities and internal resistances, based on a weighted score.
    void HillClimbOptimization() {
        bool madeAnyImprovement = true;
        int iterations = 0;
        int maxIterations = 2000; // Limit the number of iterations to prevent infinite loops in case of no further improvements.

        double targetCapacity = CalculateAverageCapacity();
        double totalConductance = 0.0;
        for (int i = 0; i < seriesCount; i++) {
            for (int j = 0; j < parallelCount; j++) {
                double r = seriesGroups[i].GetCell(j).GetResistance();
                if (r > 0) totalConductance += (1.0 / r);
            }
        }
        double targetResistance = seriesCount / totalConductance;

        while (madeAnyImprovement && iterations < maxIterations) {
            madeAnyImprovement = false;
            iterations++;

            for (int i = 0; i < seriesCount; i++) {
                for (int j = i + 1; j < seriesCount; j++) {
                    if (OptimizeParallelGroups(seriesGroups[i], seriesGroups[j], targetCapacity, targetResistance)) {
                        madeAnyImprovement = true;
                    }
                }
            }
        }
    }

    // Returns whole pack maximum voltage.
    double TotalMaxVoltage() const {
        return maxSingleCellVoltage * seriesCount;
    }

    // Returns whole pack total resistance, which is the sum of all parallel groups resistances.
    double GetTotalResistance() const {
        double totalResistance = 0.0;
        for (const auto &group: seriesGroups) {
            totalResistance += group.GetTotalResistance();
        }
        return totalResistance;
    }

    // Returns maximum parallel group total resistance.
    double GetMaxResistance() const {
        if (seriesCount > 0) {
            return MinMaxIndexes().maxInternalResistance;
        }
        return 0.0;
    }

    // Returns minimum parallel group total resistance.
    double GetMinResistance() const {
        if (seriesCount > 0) {
            return MinMaxIndexes().minInternalResistance;
        }
        return 0.0;
    }

    // Returns average parallel group total resistance.
    double GetAverageResistance() const {
        return GetTotalResistance() / seriesCount;
    }

    // Returns percentage variance between the weakest and strongest parallel group resistances.
    double GetResistanceVariancePercentage() const {
        double averageResistance = GetAverageResistance();
        auto extremes = MinMaxIndexes();
        double biggestDifference = extremes.maxInternalResistance - extremes.minInternalResistance;
        if (averageResistance == 0) return 0.0;
        return (biggestDifference / averageResistance) * 100.0;
    }

private:
    std::vector<ParallelGroup> seriesGroups; // Vector holding parallel groups, which in turn hold cells.
    int seriesCount; // Number of parallel groups in series (number of cells in series).
    int parallelCount; // Number of cells in each parallel group.
    double nominalCellVoltage; // Nominal voltage of a single cell.
    double maxSingleCellVoltage; // Maximum voltage of a single cell.

    double selectedWresistance = 0.5;
    double selectedWcapacity = 0.5;

    // Holds indexes of the parallel groups with minimum and maximum total capacities.
    struct PackExtremes {
        int minCapacityIndex = -1;
        int maxCapacityIndex = -1;
        int minCapacity = INT_MAX;
        int maxCapacity = INT_MIN;

        int minInternalResistanceIndex = -1;
        int maxInternalResistanceIndex = -1;
        double minInternalResistance = std::numeric_limits<double>::max();
        double maxInternalResistance = std::numeric_limits<double>::min();
    };


    // Returns index of the parallel group with the lowest total capacity, that isn't already full (doesn't have parallelCount cells).
    int GetLowestCapacityGroupIndex() const {
        int capacity = INT32_MAX;
        int index = -1;
        for (int i = 0; i < seriesCount; i++) {
            if (seriesGroups[i].GetTotalCapacity() < capacity && seriesGroups[i].GetCellCount() < parallelCount) {
                capacity = seriesGroups[i].GetTotalCapacity();
                index = i;
            }
        }
        return index;
    }

    // Finds the indexes of the parallel groups with minimum and maximum total capacities and internal resistances.
    PackExtremes MinMaxIndexes() const {
        if (seriesCount == 0) return {};
        PackExtremes extremes;
        for (int i = 0; i < seriesCount; i++) {
            int totalCap = seriesGroups[i].GetTotalCapacity();
            if (totalCap < extremes.minCapacity) {
                extremes.minCapacityIndex = i;
                extremes.minCapacity = totalCap;
            }
            if (totalCap > extremes.maxCapacity) {
                extremes.maxCapacityIndex = i;
                extremes.maxCapacity = totalCap;
            }

            double internalResistance = seriesGroups[i].GetTotalResistance();
            if (internalResistance < extremes.minInternalResistance) {
                extremes.minInternalResistanceIndex = i;
                extremes.minInternalResistance = internalResistance;
            }
            if (internalResistance > extremes.maxInternalResistance) {
                extremes.maxInternalResistanceIndex = i;
                extremes.maxInternalResistance = internalResistance;
            }
        }
        return extremes;
    }

    // Returns the absolute difference between two integers.
    int IntegerDifference(int cap1, int cap2) const {
        return abs(cap1 - cap2);
    }

    // Returns the absolute difference between two doubles.
    double DoubleDifference(double cap1, double cap2) const {
        return abs(cap1 - cap2);
    }
    
    // Optimizes two parallel groups by attempting to swap cells between them to minimize the difference in total capacities and internal resistances, based on a weighted score.
    // Returns true if a swap was made, false otherwise.
    bool OptimizeParallelGroups(ParallelGroup &parallelGroup1, ParallelGroup &parallelGroup2, double targetCapacity,
                                double targetResistance) {
        int bestI = -1;
        int bestJ = -1;

        double bestScore = CalculateComparisonScore(selectedWcapacity, selectedWresistance,
                                                    parallelGroup1.GetTotalCapacity(),
                                                    parallelGroup2.GetTotalCapacity(),
                                                    parallelGroup1.GetTotalResistance(),
                                                    parallelGroup2.GetTotalResistance(),
                                                    targetCapacity, targetResistance);

        for (int i = 0; i < parallelGroup1.GetCellCount(); i++) {
            int cap1 = parallelGroup1.GetCell(i).GetCapacity();
            for (int j = 0; j < parallelGroup2.GetCellCount(); j++) {
                int cap2 = parallelGroup2.GetCell(j).GetCapacity();
                int hypotheticalCap1 = parallelGroup1.GetTotalCapacity() - cap1 + cap2;
                int hypotheticalCap2 = parallelGroup2.GetTotalCapacity() - cap2 + cap1;

                double res1 = parallelGroup1.GetCell(i).GetResistance();
                double res2 = parallelGroup2.GetCell(j).GetResistance();

                double currentTotalRes1 = parallelGroup1.GetTotalResistance();
                double currentTotalRes2 = parallelGroup2.GetTotalResistance();

                double hypotheticalRes1 = currentTotalRes1;
                double hypotheticalRes2 = currentTotalRes2;

                if (res1 > 0 && res2 > 0 && currentTotalRes1 > 0 && currentTotalRes2 > 0) {
                    double d1 = (1.0 / currentTotalRes1) - (1.0 / res1) + (1.0 / res2);
                    double d2 = (1.0 / currentTotalRes2) - (1.0 / res2) + (1.0 / res1);

                    if (d1 > 0.000001 && d2 > 0.000001) {
                        hypotheticalRes1 = 1.0 / d1;
                        hypotheticalRes2 = 1.0 / d2;
                    } else {
                        continue;
                    }
                }

                double hypotheticalScore = CalculateComparisonScore(selectedWcapacity, selectedWresistance,
                                                                    hypotheticalCap1, hypotheticalCap2,
                                                                    hypotheticalRes1, hypotheticalRes2,
                                                                    targetCapacity,
                                                                    targetResistance);

                if (hypotheticalScore < (bestScore - 0.000001)) {
                    bestScore = hypotheticalScore;
                    bestI = i;
                    bestJ = j;
                }
            }
        }
        if (bestI != -1 && bestJ != -1) {
            Battery cell1 = parallelGroup1.GetCell(bestI);
            Battery cell2 = parallelGroup2.GetCell(bestJ);

            parallelGroup1.SetCell(bestI, cell2);
            parallelGroup2.SetCell(bestJ, cell1);
            return true;
        }
        return false;
    }

    // Calculates a squared weighted comparison score based on the deviations of capacities and resistances from target values.
    double CalculateComparisonScore(const double Wcapacity, const double Wresistance,
                                    const int capacity1, const int capacity2,
                                    const double resistance1, const double resistance2,
                                    const double targetCapacity, const double targetResistance) const {
        double capDev1 = (capacity1 - targetCapacity) / targetCapacity;
        double resDev1 = (resistance1 - targetResistance) / targetResistance;
        double capDev2 = (capacity2 - targetCapacity) / targetCapacity;
        double resDev2 = (resistance2 - targetResistance) / targetResistance;

        double totalCapDeviation = (capDev1 * capDev1) + (capDev2 * capDev2);
        double totalResDeviation = (resDev1 * resDev1) + (resDev2 * resDev2);

        double score = (Wcapacity * totalCapDeviation) + (Wresistance * totalResDeviation);
        return score;
    }
};


#endif
