#pragma once
#include <array>
#include <vector>
#include <QString>

struct Family {
    int id = 0;
    int nPeople = 0;
    std::array<int, 10> choices{};
};

class ProblemData {
public:
    bool loadFamilyCsv(const QString& path, QString* errorOut = nullptr);

    int familyCount() const { return static_cast<int>(m_families.size()); }
    const std::vector<Family>& families() const { return m_families; }
    int totalPeople() const { return m_totalPeople; }

private:
    std::vector<Family> m_families;
    int m_totalPeople = 0;
};
