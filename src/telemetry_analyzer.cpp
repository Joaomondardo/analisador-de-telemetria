#include "telemetry_analyzer.hpp"
#include <algorithm>
#include <execution>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <stdexcept>

namespace Telemetry {

TelemetryAnalyzer::TelemetryAnalyzer(const std::vector<TelemetryData> &data)
    : m_data(data) {}

double TelemetryAnalyzer::getAverageSpeed() const {
    if (m_data.empty())
        return 0.0;
    double sum = std::reduce(std::execution::par, m_data.begin(), m_data.end(), 0.0,
        [](double s, const TelemetryData &d) { return s + d.speed; });
    return sum / m_data.size();
}

TelemetryData TelemetryAnalyzer::getMaxRPM() const {
    if (m_data.empty())
        return TelemetryData{};
    auto it = std::max_element(m_data.begin(), m_data.end(),
        [](const TelemetryData &a, const TelemetryData &b) { return a.rpm < b.rpm; });
    return *it;
}

int TelemetryAnalyzer::countOverspeedEvents(double speedLimit) const {
    return static_cast<int>(std::count_if(std::execution::par, m_data.begin(), m_data.end(),
        [speedLimit](const TelemetryData &d) { return d.speed > speedLimit; }));
}

int TelemetryAnalyzer::countHighRpmEvents(int32_t rpmLimit) const {
    return static_cast<int>(std::count_if(std::execution::par, m_data.begin(), m_data.end(),
        [rpmLimit](const TelemetryData &d) { return d.rpm > rpmLimit; }));
}

void TelemetryAnalyzer::generateReport(const std::string &filename, double speedLimit, int32_t rpmLimit) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Nao foi possivel criar o arquivo de relatorio: " + filename);
    }

    int overspeedCount = countOverspeedEvents(speedLimit);
    int highRpmCount = countHighRpmEvents(rpmLimit);

    out << "=== RELATORIO DE TELEMETRIA ===\n";
    out << "Registros processados: " << m_data.size() << "\n";
    out << std::fixed << std::setprecision(2);
    out << "Velocidade Media: " << getAverageSpeed() << " km/h\n";
    out << "RPM Maximo: " << getMaxRPM().rpm << " RPM\n";
    out << "--- ALERTAS CRITICOS ---\n";
    out << "Eventos acima de " << speedLimit << " km/h: " << overspeedCount << "\n";
    out << "Eventos acima de " << rpmLimit << " RPM: " << highRpmCount << "\n";
}

} // namespace Telemetry