#include <gtest/gtest.h>
#include "telemetry_analyzer.hpp"
#include "csv_parser.hpp"
#include "telemetry_record.hpp"

#include <vector>
#include <stdexcept>

using namespace Telemetry;

// Helper to create mock telemetry data
static std::vector<TelemetryRecord> createMockData() {
    return {
        {0, 80.0, 2500},
        {1000, 120.0, 3500},
        {2000, 95.0, 2800},
        {3000, 105.0, 3200},
        {4000, 90.0, 3100}
    };
}

TEST(TelemetryAnalyzerTest, MathCalculations) {
    auto data = createMockData();
    TelemetryAnalyzer analyzer(data);
    EXPECT_NEAR(analyzer.getAverageSpeed(), (80.0 + 120.0 + 95.0 + 105.0 + 90.0) / 5.0, 1e-5);
    TelemetryRecord maxRpm = analyzer.getMaxRPM();
    EXPECT_EQ(maxRpm.rpm, 3500);
}

TEST(TelemetryAnalyzerTest, CriticalEventsCounting) {
    auto data = createMockData();
    TelemetryAnalyzer analyzer(data);
    // Speed limit 100 km/h
    EXPECT_EQ(analyzer.countOverspeedEvents(100.0), 2); // 120 and 105 exceed
    // RPM limit 3000
    EXPECT_EQ(analyzer.countHighRpmEvents(3000), 3); // 3500, 3200, 3100 exceed
}

TEST(TelemetryParserTest, FileLoadError) {
    CsvParser parser;
    EXPECT_THROW(parser.parse("nonexistent_file.csv"), std::runtime_error);
}
