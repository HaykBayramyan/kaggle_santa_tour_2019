# Santa Workshop Tour 2019 — Qt C++ Solver & Visualizer

This project is a C++ / Qt-based solver and visualizer for the Kaggle competition  
**Santa Workshop Tour 2019**.

Repository URL:  
https://www.kaggle.com/competitions/santa-workshop-tour-2019

The goal is to assign 5,000 families to 100 days while respecting strict daily
capacity constraints and minimizing a complex cost function.

---

## Problem Description

Each family:
- Has a fixed number of people
- Provides up to 10 preferred days
- Must be assigned to exactly one day

Each day:
- Must have **125 ≤ total people ≤ 300**

### Objective Function

Total cost consists of:

- **Preference Cost**  
  Penalty based on how far the assigned day is from the family's preferred days.

- **Accounting Cost**  
  Penalizes large occupancies and sharp changes between consecutive days.

```
Total Cost = Preference Cost + Accounting Cost
```

---

## Solution Overview

### 1. Initial Feasible Assignment
- Families are sorted by size
- Assigned greedily to preferred days if capacity allows
- Guarantees all constraints are satisfied

### 2. Optimization Method
- Simulated Annealing
- Neighborhood operations:
  - Move one family to another day
  - Swap two families between days
- Probabilistic acceptance of worse solutions enables escaping local minima

### 3. Visualization (Qt Charts)
- Daily occupancy plot with constraint bounds
- Cost over iterations:
  - Current solution
  - Best solution found

---

## Project Structure

```
kaggle_santa_tour_2019/
├── main.cpp
├── mainwindow.h / mainwindow.cpp   # Qt GUI and visualization
├── solver.h / solver.cpp           # Simulated Annealing algorithm
├── costmodel.h / costmodel.cpp     # Cost computation
├── problemdata.h / problemdata.cpp # CSV parsing
├── santa-2019.pro                  # Qt qmake project file
└── README.md
```

---

## Build Instructions

### Requirements
- Qt 6.4 or newer
- Qt Charts module
- C++17 compatible compiler

### Install Qt Charts (Linux)
```bash
sudo apt install qt6-charts-dev
```

### Build with Qt Creator
1. Open `santa-2019.pro`
2. Select a Qt 6 kit
3. Build and run

### Build from Terminal
```bash
qmake
make
./santa-2019
```

---

## Usage

1. Download `family_data.csv` from Kaggle
2. Run the application
3. Click **Load family_data.csv**
4. Start optimization
5. Observe real-time visualization
6. Save `submission.csv`
7. Upload to Kaggle

---

## Algorithm Details

### Neighborhood Moves
- Single-family reassignment
- Two-family swap

### Acceptance Rule

```
Accept move if:
  Δcost < 0
  OR
  exp(-Δcost / T) > random(0,1)
```

### Cooling Schedule
- Exponential cooling from initial temperature to final temperature

---

## Correctness Guarantees

- Capacity constraints are always respected
- Output CSV follows Kaggle submission format
- Cost functions follow official competition rules

---

## Possible Improvements

- Parallel tempering
- Large neighborhood search
- Hybrid heuristic + MILP approaches
- Performance optimizations

---

## Author

Hayk Bayramyan  
GitHub: https://github.com/HaykBayramyan

---

## License

This project is provided for educational and research purposes.
