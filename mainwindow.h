#pragma once
#include <QMainWindow>
#include <QThread>
#include <QVector>

#include "problemdata.h"
#include "costmodel.h"
#include "solver.h"

// Qt Charts
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

class QLabel;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onLoadFamilyData();
    void onStart();
    void onStop();
    void onSave();

    void onSolverProgress(int iter, double currentCost, double bestCost, QVector<int> occupancy);
    void onSolverFinished(QVector<int> bestAssignment, double bestCost);
    void onSolverLog(const QString& msg);

private:
    void setupUi();
    void resetCharts();
    void updateOccupancySeries(const QVector<int>& occ100);
    void appendCostPoint(int iter, double currentCost, double bestCost);

    ProblemData m_data;
    std::unique_ptr<CostModel> m_cost;

    QVector<int> m_bestAssignment;
    double m_bestCost = 0.0;

    // UI widgets
    QPushButton* m_btnLoad = nullptr;
    QPushButton* m_btnStart = nullptr;
    QPushButton* m_btnStop = nullptr;
    QPushButton* m_btnSave = nullptr;

    QSpinBox* m_spinIters = nullptr;
    QSpinBox* m_spinReport = nullptr;
    QDoubleSpinBox* m_spinT0 = nullptr;
    QDoubleSpinBox* m_spinT1 = nullptr;

    QLabel* m_status = nullptr;

    // Charts
    QChartView* m_occView = nullptr;
    QLineSeries* m_occSeries = nullptr;
    QLineSeries* m_minSeries = nullptr;
    QLineSeries* m_maxSeries = nullptr;

    QChartView* m_costView = nullptr;
    QLineSeries* m_currCostSeries = nullptr;
    QLineSeries* m_bestCostSeries = nullptr;


    // Solver thread
    QThread* m_thread = nullptr;
    SolverWorker* m_worker = nullptr;
};
