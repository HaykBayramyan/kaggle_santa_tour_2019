#include "costmodel.h"
#include <cmath>
#include <algorithm>

CostModel::CostModel(const ProblemData& data) : m_data(data) {}

uint32_t CostModel::preferencePenaltyFromRank(int nPeople, int rank)
{
    switch (rank) {
    case 0:  return 0;
    case 1:  return 50;
    case 2:  return 50  + 9u  * (uint32_t)nPeople;
    case 3:  return 100 + 9u  * (uint32_t)nPeople;
    case 4:  return 200 + 9u  * (uint32_t)nPeople;
    case 5:  return 200 + 18u * (uint32_t)nPeople;
    case 6:  return 300 + 18u * (uint32_t)nPeople;
    case 7:  return 300 + 36u * (uint32_t)nPeople;
    case 8:  return 400 + 36u * (uint32_t)nPeople;
    case 9:  return 500 + 36u * (uint32_t)nPeople + 199u * (uint32_t)nPeople;
    default: return 500 + 36u * (uint32_t)nPeople + 398u * (uint32_t)nPeople;
    }
}

void CostModel::build()
{
    const int F = m_data.familyCount();
    m_prefCost.assign((size_t)F * 100u, 0u);
    for (int i = 0; i < F; ++i) {
        const Family& fam = m_data.families()[i];
        for (int day = 1; day <= 100; ++day) {
            int rank = 10;
            for (int r = 0; r < 10; ++r) {
                if (fam.choices[r] == day) { rank = r; break; }
            }
            m_prefCost[(size_t)i * 100u + (size_t)(day - 1)] =
                preferencePenaltyFromRank(fam.nPeople, rank);
        }
    }
}

uint32_t CostModel::preferenceCost(int familyIndex, int day) const
{
    return m_prefCost[(size_t)familyIndex * 100u + (size_t)(day - 1)];
}

double CostModel::accountingDayCost(int Nd, int NdNext)
{
    const double n = (double)Nd;
    const double diff = std::abs((double)Nd - (double)NdNext);
    const double expo = 0.5 + diff / 50.0;
    const double raw = (n - 125.0) / 400.0 * std::pow(n, expo);
    return std::max(0.0, raw);
}

double CostModel::accountingCost(const std::vector<int>& occ) const
{
    double sum = 0.0;
    for (int day = 1; day <= 100; ++day) {
        const int Nd = occ[day];
        const int NdNext = (day == 100) ? occ[100] : occ[day + 1];
        sum += accountingDayCost(Nd, NdNext);
    }
    return sum;
}

double CostModel::totalCost(const std::vector<int>& assignment,
                            std::vector<int>* outOccupancy,
                            double* outPref,
                            double* outAcc) const
{
    const int F = m_data.familyCount();
    std::vector<int> occ(101, 0);

    double pref = 0.0;
    for (int i = 0; i < F; ++i) {
        const int day = assignment[i];
        occ[day] += m_data.families()[i].nPeople;
        pref += (double)preferenceCost(i, day);
    }

    const double acc = accountingCost(occ);

    if (outOccupancy) *outOccupancy = occ;
    if (outPref) *outPref = pref;
    if (outAcc) *outAcc = acc;

    return pref + acc;
}

double CostModel::deltaAccounting2(const std::vector<int>& occ,
                                   int dayA, int deltaA,
                                   int dayB, int deltaB) const
{
    auto deltaFor = [&](int d) -> int {
        int v = 0;
        if (d == dayA) v += deltaA;
        if (d == dayB) v += deltaB;
        return v;
    };

    auto occNew = [&](int d) -> int {
        if (d < 1) return 0;
        if (d > 100) d = 100;
        return occ[d] + deltaFor(d);
    };

    int candidates[4] = { dayA - 1, dayA, dayB - 1, dayB };
    int affected[6];
    int k = 0;
    for (int i = 0; i < 4; ++i) {
        int d = candidates[i];
        if (d < 1 || d > 100) continue;
        bool seen = false;
        for (int j = 0; j < k; ++j) if (affected[j] == d) { seen = true; break; }
        if (!seen) affected[k++] = d;
    }

    double oldSum = 0.0, newSum = 0.0;
    for (int i = 0; i < k; ++i) {
        const int d = affected[i];
        const int oldNd = occ[d];
        const int oldNext = (d == 100) ? occ[100] : occ[d + 1];
        oldSum += accountingDayCost(oldNd, oldNext);

        const int newNd = occNew(d);
        const int newNext = (d == 100) ? occNew(100) : occNew(d + 1);
        newSum += accountingDayCost(newNd, newNext);
    }

    return newSum - oldSum;
}
