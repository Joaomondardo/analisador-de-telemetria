#include <gtest/gtest.h>
#include "math_channel_evaluator.hpp"
#include "telemetry_record.hpp"

#include <cmath>
#include <vector>

using namespace Telemetry;

static std::vector<TelemetryRecord> createMockTelemetryData() {
    std::vector<TelemetryRecord> data;

    TelemetryRecord r1{};
    r1.timestamp = 0;
    r1.speed = 50.0;
    r1.rpm = 2000;
    r1.lateral_g = 0.5;
    r1.acceleration = 1.0;
    data.push_back(r1);

    TelemetryRecord r2{};
    r2.timestamp = 1000;
    r2.speed = 80.0;
    r2.rpm = 3500;
    r2.lateral_g = 0.1;
    r2.acceleration = 0.8;
    data.push_back(r2);

    TelemetryRecord r3{};
    r3.timestamp = 2000;
    r3.speed = 120.0;
    r3.rpm = 4200;
    r3.lateral_g = 0.4;
    r3.acceleration = 1.2;
    data.push_back(r3);

    TelemetryRecord r4{};
    r4.timestamp = 3000;
    r4.speed = 0.0;
    r4.rpm = 1000;
    r4.lateral_g = 0.0;
    r4.acceleration = 0.0;
    data.push_back(r4);

    return data;
}

TEST(MathChannelEvaluatorTest, SpeedTimesFactor) {
    auto data = createMockTelemetryData();
    auto outputs = processarCanalCustomizado(data, "speed * 3.6");

    ASSERT_EQ(outputs.size(), data.size());
    EXPECT_NEAR(outputs[0], 50.0 * 3.6, 0.0001);
    EXPECT_NEAR(outputs[1], 80.0 * 3.6, 0.0001);
    EXPECT_NEAR(outputs[2], 120.0 * 3.6, 0.0001);
    EXPECT_NEAR(outputs[3], 0.0 * 3.6, 0.0001);
}

TEST(MathChannelEvaluatorTest, CombinedGForceFormula) {
    auto data = createMockTelemetryData();
    auto outputs = processarCanalCustomizado(data, "sqrt(lateral_g^2 + acceleration^2)");

    ASSERT_EQ(outputs.size(), data.size());
    EXPECT_NEAR(outputs[0], std::sqrt(0.5 * 0.5 + 1.0 * 1.0), 0.0001);
    EXPECT_NEAR(outputs[1], std::sqrt(0.1 * 0.1 + 0.8 * 0.8), 0.0001);
    EXPECT_NEAR(outputs[2], std::sqrt(0.4 * 0.4 + 1.2 * 1.2), 0.0001);
    EXPECT_NEAR(outputs[3], std::sqrt(0.0 * 0.0 + 0.0 * 0.0), 0.0001);
}

TEST(MathChannelEvaluatorTest, SyntaxErrorDoesNotCrash) {
    MathChannelEvaluator evaluator;
    bool compiled = evaluator.compile("speed * * 2");

    EXPECT_FALSE(compiled);
    EXPECT_FALSE(evaluator.errorMessage().empty());
}
