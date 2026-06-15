#pragma once

#include <cstdint>
#include <string>

namespace Telemetry {

// Estrutura de domínio única utilizada em todo o projeto.
struct TelemetryRecord {
  uint64_t timestamp; // milissegundos desde epoch
  double speed;       // km/h
  int32_t rpm;        // rotações por minuto

  // Campos derivados de análise de transição entre registros.
  double acceleration{0.0}; // m/s^2
  double lateral_g{0.0};    // unidades g aproximadas
};

} // namespace Telemetry