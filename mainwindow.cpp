#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

#include <QtCharts/QChart>.
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegend>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    resetCharts();
}

MainWindow::~MainWindow()
{
    onStop();
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    auto* controls = new QHBoxLayout();

    m_btnLoad = new QPushButton("Load family_data.csv");
    m_btnStart = new QPushButton("Start");
    m_btnStop  = new QPushButton("Stop");
    m_btnSave  = new QPushButton("Save submission.csv");

    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(false);
    m_btnSave->setEnabled(false);

    m_spinIters = new QSpinBox();
    m_spinIters->setRange(1000, 50000000);
    m_spinIters->setValue(200000);

    m_spinReport = new QSpinBox();
    m_spinReport->setRange(100, 5000000);
    m_spinReport->setValue(2000);

    m_spinT0 = new QDoubleSpinBox();
    m_spinT0->setRange(0.1, 1e9);
    m_spinT0->setDecimals(1);
    m_spinT0->setValue(10000.0);

    m_spinT1 = new QDoubleSpinBox();
    m_spinT1->setRange(0.0001, 1e9);
    m_spinT1->setDecimals(4);
    m_spinT1->setValue(1.0);

    controls->addWidget(m_btnLoad);
    controls->addWidget(new QLabel("Iters:"));
    controls->addWidget(m_spinIters);
    controls->addWidget(new QLabel("ReportEvery:"));
    controls->addWidget(m_spinReport);
    controls->addWidget(new QLabel("T0:"));
    controls->addWidget(m_spinT0);
    controls->addWidget(new QLabel("T1:"));
    controls->addWidget(m_spinT1);
    controls->addWidget(m_btnStart);
    controls->addWidget(m_btnStop);
    controls->addWidget(m_btnSave);

    root->addLayout(controls);

    // --- Charts layout
    auto* charts = new QHBoxLayout();

    m_occView = new QChartView();
    m_costView = new QChartView();

    m_occView->setRenderHint(QPainter::Antialiasing);
    m_costView->setRenderHint(QPainter::Antialiasing);

    charts->addWidget(m_occView, 1);
    charts->addWidget(m_costView, 1);

    root->addLayout(charts, 1);

    m_status = new QLabel("Load family_data.csv to begin.");
    root->addWidget(m_status);

    setCentralWidget(central);
    resize(1200, 700);

    connect(m_btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadFamilyData);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(m_btnStop,  &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_btnSave,  &QPushButton::clicked, this, &MainWindow::onSave);
}

void MainWindow::resetCharts()
{
    auto* occChart = new QChart();
    occChart->setTitle("Daily occupancy (1..100) with min/max");

    m_occSeries = new QLineSeries();
    m_occSeries->setName("Occupancy");

    m_minSeries = new QLineSeries();
    m_minSeries->setName("Min=125");
    for (int d = 1; d <= 100; ++d) m_minSeries->append(d, 125);

    m_maxSeries = new QLineSeries();
    m_maxSeries->setName("Max=300");
    for (int d = 1; d <= 100; ++d) m_maxSeries->append(d, 300);

    occChart->addSeries(m_occSeries);
    occChart->addSeries(m_minSeries);
    occChart->addSeries(m_maxSeries);

    auto* xOcc = new QValueAxis();
    xOcc->setRange(1, 100);
    xOcc->setTitleText("Day");

    auto* yOcc = new QValueAxis();
    yOcc->setRange(100, 320);
    yOcc->setTitleText("People");

    occChart->addAxis(xOcc, Qt::AlignBottom);
    occChart->addAxis(yOcc, Qt::AlignLeft);

    m_occSeries->attachAxis(xOcc); m_occSeries->attachAxis(yOcc);
    m_minSeries->attachAxis(xOcc); m_minSeries->attachAxis(yOcc);
    m_maxSeries->attachAxis(xOcc); m_maxSeries->attachAxis(yOcc);

    m_occView->setChart(occChart);
    auto* costChart = new QChart();
    costChart->setTitle("Cost over iterations (current vs best)");

    m_currCostSeries = new QLineSeries();
    m_currCostSeries->setName("Current");

    m_bestCostSeries = new QLineSeries();
    m_bestCostSeries->setName("Best");

    costChart->addSeries(m_currCostSeries);
    costChart->addSeries(m_bestCostSeries);

    auto* xCost = new QValueAxis();
    xCost->setRange(0, std::max(1000, m_spinIters->value()));
    xCost->setTitleText("Iteration");

    auto* yCost = new QValueAxis();
    yCost->setTitleText("Cost");

    costChart->addAxis(xCost, Qt::AlignBottom);
    costChart->addAxis(yCost, Qt::AlignLeft);

    m_currCostSeries->attachAxis(xCost); m_currCostSeries->attachAxis(yCost);
    m_bestCostSeries->attachAxis(xCost); m_bestCostSeries->attachAxis(yCost);

    m_costView->setChart(costChart);
}

void MainWindow::onLoadFamilyData()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Select family_data.csv", QString(), "CSV (*.csv)");

    if (path.isEmpty()) return;

    QString err;
    if (!m_data.loadFamilyCsv(path, &err)) {
        QMessageBox::critical(this, "Load failed", err);
        return;
    }

    m_cost = std::make_unique<CostModel>(m_data);
    m_cost->build();

    m_btnStart->setEnabled(true);
    m_status->setText(QString("Loaded %1 families. Total people=%2. Ready.")
                          .arg(m_data.familyCount())
                          .arg(m_data.totalPeople()));
    resetCharts();
}

void MainWindow::onStart()
{
    if (!m_cost) return;
    if (m_thread) return;

    m_btnStart->setEnabled(false);
    m_btnStop->setEnabled(true);
    m_btnSave->setEnabled(false);

    SolverParams params;
    params.maxIterations = m_spinIters->value();
    params.reportEvery = m_spinReport->value();
    params.startTemp = m_spinT0->value();
    params.endTemp = m_spinT1->value();
    params.seed = 42;

    m_thread = new QThread(this);
    m_worker = new SolverWorker(&m_data, m_cost.get(), QVector<int>(), params);
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &SolverWorker::run);
    connect(m_worker, &SolverWorker::progress, this, &MainWindow::onSolverProgress);
    connect(m_worker, &SolverWorker::finished, this, &MainWindow::onSolverFinished);
    connect(m_worker, &SolverWorker::log, this, &MainWindow::onSolverLog);

    connect(m_worker, &SolverWorker::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    m_thread->start();

    m_status->setText("Solver started...");
}

void MainWindow::onStop()
{
    if (m_worker) m_worker->stop();
    m_worker = nullptr;
    m_thread = nullptr;

    m_btnStart->setEnabled(m_cost != nullptr);
    m_btnStop->setEnabled(false);
}

void MainWindow::onSave()
{
    if (m_bestAssignment.isEmpty()) return;

    const QString path = QFileDialog::getSaveFileName(
        this, "Save submission.csv", "submission.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Save failed", "Cannot write: " + path);
        return;
    }

    QTextStream out(&f);
    out << "family_id,assigned_day\n";
    for (int i = 0; i < m_data.familyCount(); ++i) {
        out << m_data.families()[i].id << "," << m_bestAssignment[i] << "\n";
    }

    m_status->setText("Saved: " + path);
}

void MainWindow::onSolverProgress(int iter, double currentCost, double bestCost, QVector<int> occupancy)
{
    updateOccupancySeries(occupancy);
    appendCostPoint(iter, currentCost, bestCost);

    m_status->setText(QString("Iter=%1  Current=%2  Best=%3")
                          .arg(iter)
                          .arg(currentCost, 0, 'f', 2)
                          .arg(bestCost, 0, 'f', 2));
}

void MainWindow::onSolverFinished(QVector<int> bestAssignment, double bestCost)
{
    m_bestAssignment = bestAssignment;
    m_bestCost = bestCost;

    m_btnSave->setEnabled(true);
    m_btnStart->setEnabled(true);
    m_btnStop->setEnabled(false);

    m_status->setText(QString("Finished. Best cost=%1 (you can Save submission.csv)")
                          .arg(bestCost, 0, 'f', 2));
}

void MainWindow::onSolverLog(const QString& msg)
{
    m_status->setText(msg);
}

void MainWindow::updateOccupancySeries(const QVector<int>& occ100)
{
    QVector<QPointF> pts;
    pts.reserve(100);
    for (int d = 1; d <= 100; ++d) {
        pts.push_back(QPointF(d, occ100[d - 1]));
    }
    m_occSeries->replace(pts);
}

void MainWindow::appendCostPoint(int iter, double currentCost, double bestCost)
{
    m_currCostSeries->append(iter, currentCost);
    m_bestCostSeries->append(iter, bestCost);
    auto* chart = m_costView->chart();
    auto axes = chart->axes(Qt::Horizontal);
    if (!axes.isEmpty()) {
        auto* x = qobject_cast<QValueAxis*>(axes.first());
        if (x && iter > x->max()) x->setMax(iter);
    }
}
