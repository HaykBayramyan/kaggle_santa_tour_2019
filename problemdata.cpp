#include "problemdata.h"
#include <QFile>
#include <QTextStream>

bool ProblemData::loadFamilyCsv(const QString& path, QString* errorOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorOut) *errorOut = "Cannot open file: " + path;
        return false;
    }

    QTextStream in(&f);
    QString header = in.readLine();
    if (header.isEmpty()) {
        if (errorOut) *errorOut = "Empty CSV.";
        return false;
    }

    m_families.clear();
    m_totalPeople = 0;

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        const QStringList parts = line.split(',');
        if (parts.size() < 12) continue;

        Family fam;
        fam.id = parts[0].toInt();
        for (int i = 0; i < 10; ++i)
            fam.choices[i] = parts[1 + i].toInt();
        fam.nPeople = parts[11].toInt();

        m_totalPeople += fam.nPeople;
        m_families.push_back(fam);
    }

    if (m_families.empty()) {
        if (errorOut) *errorOut = "No rows parsed from CSV.";
        return false;
    }
    return true;
}
