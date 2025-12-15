#pragma once
#include "problemdata.h"
#include <vector>
#include <cstdint>

class CostModel {
public:
    explicit CostModel(const ProblemData& data);

    void build();

    uint32_t preferenceCost(int familyIndex, int day) const;
    double accountingCost(const std::vector<int>& occupancy) const;
    double totalCost(const std::vector<int>& assignment,
                     std::vector<int>* outOccupancy = nullptr,
                     double* outPref = nullptr,
                     double* outAcc = nullptr) const;
    double deltaAccounting2(const std::vector<int>& occupancy,
                            int dayA, int deltaA,
                            int dayB, int deltaB) const;

private:
    const ProblemData& m_data;
    std::vector<uint32_t> m_prefCost;

    static uint32_t preferencePenaltyFromRank(int nPeople, int rank);
    static double accountingDayCost(int Nd, int NdNext);
};
