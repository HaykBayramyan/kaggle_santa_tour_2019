#pragma once
#include <QObject>
#include <QVector>
#include <atomic>
#include <random>
#include <vector>
#include "problemdata.h"
#include "costmodel.h"

struct SolverParams {
    int maxIterations = 200000;
    int reportEvery = 2000;
    double startTemp = 10000.0;
    double endTemp = 1.0;
    uint32_t seed = 42;
};

class SolverWorker : public QObject
{
    Q_OBJECT
public:
    SolverWorker(const ProblemData* data,
                 const CostModel* cost,
                 const QVector<int>& initialAssignment,
                 const SolverParams& params);

public slots:
    void run();
    void stop();

signals:
    void progress(int iter, double currentCost, double bestCost, QVector<int> occupancy);
    void finished(QVector<int> bestAssignment, double bestCost);
    void log(QString msg);

private:
    const ProblemData* m_data = nullptr;
    const CostModel* m_cost = nullptr;
    QVector<int> m_initial;
    SolverParams m_params;
    std::atomic_bool m_stop{false};

    std::vector<int> makeFeasibleInitial(std::mt19937& rng) const;
};
