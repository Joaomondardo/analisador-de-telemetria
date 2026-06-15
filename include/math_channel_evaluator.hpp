#pragma once

#include <string>
#include <vector>
#include "exprtk.hpp"
#include "telemetry_record.hpp"

namespace Telemetry {

class MathChannelEvaluator {
public:
    MathChannelEvaluator();

    // Compila a expressão e constrói a AST.
    // Retorna true se a fórmula for válida.
    bool compile(const std::string& formula);

    // Avalia a expressão para um registro de telemetria.
    // Deve ser chamado milhões de vezes sem alocação de heap.
    double evaluate(const TelemetryRecord& record) const;

    // Texto de erro da última compilação falha.
    const std::string& errorMessage() const noexcept;

private:
    mutable double m_timestamp_ref{0.0};
    mutable double m_speed_ref{0.0};
    mutable double m_rpm_ref{0.0};
    mutable double m_lateral_g_ref{0.0};
    mutable double m_acceleration_ref{0.0};

    exprtk::symbol_table<double> m_symbol_table;
    exprtk::expression<double> m_expression;
    exprtk::parser<double> m_parser;
    std::string m_lastError;
};

std::vector<double> processarCanalCustomizado(
    const std::vector<TelemetryRecord>& data,
    const std::string& formula);

} // namespace Telemetry
