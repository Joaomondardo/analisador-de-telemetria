#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace Telemetry {

struct DataPoint {
    double x;
    double y;
};

class Downsampler {
public:
    // Executes Largest-Triangle-Three-Buckets downsampling.
    // Returns a reduced set of points with size equal to threshold when possible.
    static std::vector<DataPoint> lttb(const std::vector<DataPoint>& data, size_t threshold) {
        if (threshold == 0 || data.empty()) {
            return {};
        }

        if (threshold >= data.size() || threshold < 3) {
            return data;
        }

        const size_t dataLength = data.size();
        std::vector<DataPoint> sampled;
        sampled.reserve(threshold);
        sampled.push_back(data[0]);

        const double bucketSize = static_cast<double>(dataLength - 2) / static_cast<double>(threshold - 2);
        size_t a = 0;

        for (size_t i = 0; i < threshold - 2; ++i) {
            const size_t rangeStart = static_cast<size_t>(std::floor((i + 1) * bucketSize)) + 1;
            const size_t rangeEnd = static_cast<size_t>(std::floor((i + 2) * bucketSize)) + 1;
            const size_t rangeEndClamped = std::min(rangeEnd, dataLength - 1);

            const size_t nextRangeStart = rangeEndClamped;
            const size_t nextRangeEnd = (i + 3 >= threshold) ? dataLength : std::min(static_cast<size_t>(std::floor((i + 3) * bucketSize)) + 1, dataLength);

            double avgX = 0.0;
            double avgY = 0.0;
            const size_t avgRangeStart = rangeStart;
            const size_t avgRangeEnd = nextRangeEnd;
            const size_t avgRangeLength = avgRangeEnd > avgRangeStart ? avgRangeEnd - avgRangeStart : 0;

            if (avgRangeLength > 0) {
                for (size_t idx = avgRangeStart; idx < avgRangeEnd; ++idx) {
                    avgX += data[idx].x;
                    avgY += data[idx].y;
                }
                avgX /= static_cast<double>(avgRangeLength);
                avgY /= static_cast<double>(avgRangeLength);
            }

            const DataPoint& pointA = data[a];
            double maxArea = -1.0;
            size_t selected = rangeStart;

            const size_t pointRangeEnd = rangeEndClamped;
            for (size_t idx = rangeStart; idx < pointRangeEnd; ++idx) {
                const double area = std::abs(
                    (pointA.x - avgX) * (data[idx].y - pointA.y) -
                    (pointA.x - data[idx].x) * (avgY - pointA.y)) * 0.5;

                if (area > maxArea) {
                    maxArea = area;
                    selected = idx;
                }
            }

            sampled.push_back(data[selected]);
            a = selected;
        }

        sampled.push_back(data.back());
        return sampled;
    }
};

} // namespace Telemetry
