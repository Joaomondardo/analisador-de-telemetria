#include <gtest/gtest.h>
#include "downsampler.hpp"

using namespace Telemetry;

TEST(DownsamplerTest, PreservesEndpointsAndCount) {
    std::vector<DataPoint> data;
    for (int i = 0; i < 10; ++i) {
        data.push_back({static_cast<double>(i), static_cast<double>(i * i)});
    }

    auto downsampled = Downsampler::lttb(data, 5);

    ASSERT_EQ(downsampled.size(), 5);
    EXPECT_EQ(downsampled.front().x, data.front().x);
    EXPECT_EQ(downsampled.back().x, data.back().x);
}

TEST(DownsamplerTest, ReturnsOriginalDataWhenThresholdTooLarge) {
    std::vector<DataPoint> data = {
        {0.0, 0.0},
        {1.0, 1.0},
        {2.0, 4.0}
    };

    auto result = Downsampler::lttb(data, 10);

    ASSERT_EQ(result.size(), data.size());
    EXPECT_EQ(result[1].x, 1.0);
}

TEST(DownsamplerTest, RetainsProminentPeak) {
    std::vector<DataPoint> data = {
        {0.0, 0.0},
        {1.0, 5.0},
        {2.0, 2.0},
        {3.0, 100.0},
        {4.0, 3.0},
        {5.0, 0.0}
    };

    auto sampled = Downsampler::lttb(data, 4);

    ASSERT_EQ(sampled.size(), 4);
    EXPECT_EQ(sampled.front().x, 0.0);
    EXPECT_EQ(sampled.back().x, 5.0);
    bool containsPeak = false;
    for (auto const &point : sampled) {
        if (point.y == 100.0) {
            containsPeak = true;
            break;
        }
    }
    EXPECT_TRUE(containsPeak);
}

TEST(DownsamplerTest, ZeroThresholdReturnsEmpty) {
    std::vector<DataPoint> data = {
        {0.0, 0.0},
        {1.0, 1.0}
    };

    auto result = Downsampler::lttb(data, 0);

    EXPECT_TRUE(result.empty());
}
