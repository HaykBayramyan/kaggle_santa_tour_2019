#include "solver.h"
#include <cmath>
#include <algorithm>

SolverWorker::SolverWorker(const ProblemData* data,
                           const CostModel* cost,
                           const QVector<int>& initialAssignment,
                           const SolverParams& params)
    : m_data(data), m_cost(cost), m_initial(initialAssignment), m_params(params)
{}

void SolverWorker::stop()
{
    m_stop.store(true);
}

std::vector<int> SolverWorker::makeFeasibleInitial(std::mt19937& rng) const
{
    const int F = m_data->familyCount();
    std::vector<int> assign(F, 1);
    std::vector<int> occ(101, 0);
    std::vector<int> order(F);
    for (int i = 0; i < F; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b){
        return m_data->families()[a].nPeople > m_data->families()[b].nPeople;
    });

    for (int idx : order) {
        const auto& fam = m_data->families()[idx];
        bool placed = false;

        for (int r = 0; r < 10; ++r) {
            int d = fam.choices[r];
            if (occ[d] + fam.nPeople <= 300) {
                assign[idx] = d;
                occ[d] += fam.nPeople;
                placed = true;
                break;
            }
        }
        if (!placed) {
            int bestDay = 1;
            int bestOcc = 1e9;
            for (int d = 1; d <= 100; ++d) {
                if (occ[d] + fam.nPeople <= 300 && occ[d] < bestOcc) {
                    bestOcc = occ[d];
                    bestDay = d;
                }
            }
            assign[idx] = bestDay;
            occ[bestDay] += fam.nPeople;
        }
    }

    std::vector<std::vector<int>> dayToFamilies(101);
    std::vector<int> posInDay(F, 0);
    for (int i = 0; i < F; ++i) {
        int d = assign[i];
        posInDay[i] = (int)dayToFamilies[d].size();
        dayToFamilies[d].push_back(i);
    }

    auto removeFromDay = [&](int fam, int day){
        auto& v = dayToFamilies[day];
        int p = posInDay[fam];
        int last = v.back();
        v[p] = last;
        posInDay[last] = p;
        v.pop_back();
    };
    auto addToDay = [&](int fam, int day){
        posInDay[fam] = (int)dayToFamilies[day].size();
        dayToFamilies[day].push_back(fam);
    };
    for (int pass = 0; pass < 20000; ++pass) {
        int worstDay = -1;
        int minOcc = 1e9;
        for (int d = 1; d <= 100; ++d) {
            if (occ[d] < 125 && occ[d] < minOcc) {
                minOcc = occ[d];
                worstDay = d;
            }
        }
        if (worstDay == -1) break;
        int donorDay = -1;
        int maxOcc = -1;
        for (int d = 1; d <= 100; ++d) {
            if (occ[d] > 125 && occ[d] > maxOcc) {
                maxOcc = occ[d];
                donorDay = d;
            }
        }
        if (donorDay == -1) break;
        auto& donorList = dayToFamilies[donorDay];
        if (donorList.empty()) continue;

        std::uniform_int_distribution<int> pick(0, (int)donorList.size() - 1);

        double bestDelta = 1e100;
        int bestFam = -1;

        for (int t = 0; t < 60; ++t) {
            int famIdx = donorList[pick(rng)];
            const int n = m_data->families()[famIdx].nPeople;

            if (occ[donorDay] - n < 125) continue;
            if (occ[worstDay] + n > 300) continue;

            const double dPref =
                (double)m_cost->preferenceCost(famIdx, worstDay) -
                (double)m_cost->preferenceCost(famIdx, donorDay);

            const double dAcc =
                m_cost->deltaAccounting2(occ, donorDay, -n, worstDay, +n);

            double delta = dPref + dAcc;
            if (delta < bestDelta) {
                bestDelta = delta;
                bestFam = famIdx;
            }
        }

        if (bestFam == -1) {
            continue;
        }
        const int n = m_data->families()[bestFam].nPeople;
        removeFromDay(bestFam, donorDay);
        occ[donorDay] -= n;
        assign[bestFam] = worstDay;
        occ[worstDay] += n;
        addToDay(bestFam, worstDay);
    }

    return assign;
}

void SolverWorker::run()
{
    m_stop.store(false);
    std::mt19937 rng(m_params.seed);

    emit log("Building initial feasible schedule...");
    std::vector<int> current = makeFeasibleInitial(rng);

    std::vector<int> occ;
    double pref = 0.0, acc = 0.0;
    double currentCost = m_cost->totalCost(current, &occ, &pref, &acc);
    for (int d = 1; d <= 100; ++d) {
        if (occ[d] < 125 || occ[d] > 300) {
            emit log("WARNING: Initial schedule violated constraints (should not happen).");
            break;
        }
    }

    std::vector<int> best = current;
    double bestCost = currentCost;
    const int F = m_data->familyCount();
    std::vector<std::vector<int>> dayToFamilies(101);
    std::vector<int> posInDay(F, 0);
    for (int i = 0; i < F; ++i) {
        int d = current[i];
        posInDay[i] = (int)dayToFamilies[d].size();
        dayToFamilies[d].push_back(i);
    }

    auto removeFromDay = [&](int fam, int day){
        auto& v = dayToFamilies[day];
        int p = posInDay[fam];
        int last = v.back();
        v[p] = last;
        posInDay[last] = p;
        v.pop_back();
    };
    auto addToDay = [&](int fam, int day){
        posInDay[fam] = (int)dayToFamilies[day].size();
        dayToFamilies[day].push_back(fam);
    };

    std::uniform_int_distribution<int> famDist(0, F - 1);
    std::uniform_int_distribution<int> dayDist(1, 100);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    auto temperatureAt = [&](int iter) -> double {
        const double t0 = m_params.startTemp;
        const double t1 = m_params.endTemp;
        const double a = (double)iter / std::max(1, m_params.maxIterations);
        return t0 * std::pow(t1 / t0, a);
    };

    emit log("Starting simulated annealing...");
    for (int iter = 1; iter <= m_params.maxIterations && !m_stop.load(); ++iter) {

        const double T = temperatureAt(iter);
        const bool doSwap = (uni(rng) < 0.30);

        double delta = 0.0;

        if (!doSwap) {
            // MOVE
            int f = famDist(rng);
            int oldDay = current[f];

            int newDay = oldDay;
            if (uni(rng) < 0.85) {
                int r = (int)(uni(rng) * 10.0);
                r = std::clamp(r, 0, 9);
                newDay = m_data->families()[f].choices[r];
            } else {
                newDay = dayDist(rng);
            }
            if (newDay == oldDay) continue;

            const int n = m_data->families()[f].nPeople;
            if (occ[oldDay] - n < 125) continue;
            if (occ[newDay] + n > 300) continue;

            const double dPref =
                (double)m_cost->preferenceCost(f, newDay) -
                (double)m_cost->preferenceCost(f, oldDay);

            const double dAcc = m_cost->deltaAccounting2(occ, oldDay, -n, newDay, +n);
            delta = dPref + dAcc;

            bool accept = (delta < 0.0) || (uni(rng) < std::exp(-delta / std::max(1e-9, T)));
            if (accept) {
                removeFromDay(f, oldDay);
                occ[oldDay] -= n;
                current[f] = newDay;
                occ[newDay] += n;
                addToDay(f, newDay);

                currentCost += delta;
                if (currentCost < bestCost) {
                    bestCost = currentCost;
                    best = current;
                }
            }

        } else {
            int f1 = famDist(rng);
            int f2 = famDist(rng);
            if (f1 == f2) continue;

            int d1 = current[f1];
            int d2 = current[f2];
            if (d1 == d2) continue;

            const int n1 = m_data->families()[f1].nPeople;
            const int n2 = m_data->families()[f2].nPeople;

            const int newOcc1 = occ[d1] - n1 + n2;
            const int newOcc2 = occ[d2] - n2 + n1;

            if (newOcc1 < 125 || newOcc1 > 300) continue;
            if (newOcc2 < 125 || newOcc2 > 300) continue;

            const double dPref =
                (double)m_cost->preferenceCost(f1, d2) +
                (double)m_cost->preferenceCost(f2, d1) -
                (double)m_cost->preferenceCost(f1, d1) -
                (double)m_cost->preferenceCost(f2, d2);

            const double dAcc =
                m_cost->deltaAccounting2(occ, d1, (-n1 + n2), d2, (-n2 + n1));

            delta = dPref + dAcc;

            bool accept = (delta < 0.0) || (uni(rng) < std::exp(-delta / std::max(1e-9, T)));
            if (accept) {
                removeFromDay(f1, d1);
                removeFromDay(f2, d2);

                occ[d1] = newOcc1;
                occ[d2] = newOcc2;
                current[f1] = d2;
                current[f2] = d1;

                addToDay(f1, d2);
                addToDay(f2, d1);

                currentCost += delta;
                if (currentCost < bestCost) {
                    bestCost = currentCost;
                    best = current;
                }
            }
        }

        if (iter % m_params.reportEvery == 0) {
            QVector<int> occQt(100);
            for (int d = 1; d <= 100; ++d) occQt[d - 1] = occ[d];
            emit progress(iter, currentCost, bestCost, occQt);
        }
    }

    QVector<int> bestQt(F);
    for (int i = 0; i < F; ++i) bestQt[i] = best[i];

    emit finished(bestQt, bestCost);
}
