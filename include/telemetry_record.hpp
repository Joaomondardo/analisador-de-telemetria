#pragma once

#include <cstdint>
#include <string>

namespace Telemetry {

// Estrutura de domínio única utilizada em todo o projeto.
struct TelemetryRecord {
  uint64_t timestamp; // milissegundos desde epoch
  double speed;       // km/h
  int32_t rpm;        // rotações por minuto
};

} // namespace Telemetry