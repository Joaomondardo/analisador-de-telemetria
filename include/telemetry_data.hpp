#pragma once

#include <cstdint>

struct TelemetryData {
    uint64_t timestamp; // Timestamp em milissegundos
    double speed;       // Velocidade (km/h)
    int32_t rpm;        // Rotações por Minuto (RPM)
};
