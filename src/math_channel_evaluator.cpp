#include "math_channel_evaluator.hpp"

#include <algorithm>
#include <execution>

namespace Telemetry {

MathChannelEvaluator::MathChannelEvaluator() {
    m_symbol_table.add_constants();

    m_symbol_table.add_variable("timestamp", m_timestamp_ref);
    m_symbol_table.add_variable("speed", m_speed_ref);
    m_symbol_table.add_variable("rpm", m_rpm_ref);
    m_symbol_table.add_variable("lateral_g", m_lateral_g_ref);
    m_symbol_table.add_variable("acceleration", m_acceleration_ref);

    m_expression.register_symbol_table(m_symbol_table);
}

bool MathChannelEvaluator::compile(const std::string& formula) {
    m_lastError.clear();

    if (!m_parser.compile(formula, m_expression)) {
        m_lastError = m_parser.error();
        return false;
    }

    return true;
}

inline double MathChannelEvaluator::evaluate(const TelemetryRecord& record) const {
    m_timestamp_ref = static_cast<double>(record.timestamp);
    m_speed_ref = record.speed;
    m_rpm_ref = static_cast<double>(record.rpm);
    m_lateral_g_ref = record.lateral_g;
    m_acceleration_ref = record.acceleration;

    return m_expression.value();
}

const std::string& MathChannelEvaluator::errorMessage() const noexcept {
    return m_lastError;
}

std::vector<double> processarCanalCustomizado(
    const std::vector<TelemetryRecord>& data,
    const std::string& formula) {

    std::vector<double> outputs;
    outputs.resize(data.size());

    std::transform(
        std::execution::par_unseq,
        data.begin(),
        data.end(),
        outputs.begin(),
        [&formula](const TelemetryRecord& record) {
            thread_local MathChannelEvaluator thread_local_evaluator;
            thread_local std::string last_compiled_formula;

            // Recompile if this is the first use in the thread or the formula changed
            if (last_compiled_formula != formula) {
                thread_local_evaluator.compile(formula);
                last_compiled_formula = formula;
            }

            return thread_local_evaluator.evaluate(record);
        });

    return outputs;
}

} // namespace Telemetry
