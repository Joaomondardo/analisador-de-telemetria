#pragma once

#include "downsampler.hpp"
#include "telemetry_record.hpp"

#include <functional>
#include <string>
#include <vector>

namespace Telemetry {

class LapSession {
public:
    LapSession(std::string lapName, std::vector<TelemetryRecord> records);

    const std::string& lapName() const noexcept;
    double timeOffset() const noexcept;
    void setTimeOffset(double offsetMs) noexcept;
    bool empty() const noexcept;
    size_t size() const noexcept;
    const std::vector<TelemetryRecord>& records() const noexcept;

    std::vector<DataPoint> getDownsampledData(size_t threshold) const;
    std::vector<DataPoint> getDownsampledSeries(
        const std::function<double(const TelemetryRecord&)>& selector,
        size_t threshold) const;
    std::vector<DataPoint> getAlignedSeries(
        const std::function<double(const TelemetryRecord&)>& selector) const;

private:
    std::string m_lapName;
    std::vector<TelemetryRecord> m_records;
    double m_timeOffset{0.0};
};

} // namespace Telemetry
