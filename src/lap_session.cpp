#include "lap_session.hpp"
#include "csv_parser.hpp"

#include <algorithm>

namespace Telemetry {

LapSession::LapSession(std::string lapName, std::vector<TelemetryRecord> records)
    : m_lapName(std::move(lapName))
    , m_records(std::move(records)) {
}

const std::string& LapSession::lapName() const noexcept {
    return m_lapName;
}

double LapSession::timeOffset() const noexcept {
    return m_timeOffset;
}

void LapSession::setTimeOffset(double offsetMs) noexcept {
    m_timeOffset = offsetMs;
}

bool LapSession::empty() const noexcept {
    return m_records.empty();
}

size_t LapSession::size() const noexcept {
    return m_records.size();
}

const std::vector<TelemetryRecord>& LapSession::records() const noexcept {
    return m_records;
}

std::vector<DataPoint> LapSession::getDownsampledData(size_t threshold) const {
    std::vector<DataPoint> points;
    points.reserve(m_records.size());
    for (const auto& record : m_records) {
        const double xValue = static_cast<double>(record.timestamp) + m_timeOffset;
        points.push_back({xValue, record.speed});
    }
    return Downsampler::lttb(points, threshold);
}

std::vector<DataPoint> LapSession::getDownsampledSeries(
    const std::function<double(const TelemetryRecord&)>& selector,
    size_t threshold) const {
    std::vector<DataPoint> points;
    points.reserve(m_records.size());
    for (const auto& record : m_records) {
        const double xValue = static_cast<double>(record.timestamp) + m_timeOffset;
        points.push_back({xValue, selector(record)});
    }
    return Downsampler::lttb(points, threshold);
}

std::vector<DataPoint> LapSession::getAlignedSeries(
    const std::function<double(const TelemetryRecord&)>& selector) const {
    std::vector<DataPoint> points;
    points.reserve(m_records.size());
    for (const auto& record : m_records) {
        const double xValue = static_cast<double>(record.timestamp) + m_timeOffset;
        points.push_back({xValue, selector(record)});
    }
    return points;
}

} // namespace Telemetry
