#include "csv_parser.hpp"
#include "telemetry_analyzer.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <cstdint>

void printHelp(const char *progName) {
  std::cout << "Uso: " << progName << " [arquivo_csv] [opcoes]\n"
            << "Opcoes:\n"
            << "  --speed-limit <valor>  Define limite de velocidade (default "
               "100.0)\n"
            << "  --rpm-limit <valor>    Define limite de RPM (default 3000)\n"
            << "  -h, --help             Mostra esta mensagem\n";
}

int main(int argc, char *argv[]) {
  std::string csvPath = "data/large_test.csv";
  double speedLimit = 100.0;
  int32_t rpmLimit = 3000;

  // Parser de argumentos
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      printHelp(argv[0]);
      return 0;
    } else if (arg == "--speed-limit" && i + 1 < argc) {
      speedLimit = std::stod(argv[++i]);
    } else if (arg == "--rpm-limit" && i + 1 < argc) {
      rpmLimit = std::stoi(argv[++i]);
    } else if (i == 1 && arg.find(".csv") != std::string::npos) {
      csvPath = arg;
    }
  }

  // Carregamento e Validação
  auto parser = std::make_unique<Telemetry::CsvParser>();
  auto data = parser->parse(csvPath);

  if (data.empty()) {
    std::cerr << "Erro: Nenhum dado carregado. Verifique o arquivo: " << csvPath
              << std::endl;
    return 1;
  }

  Telemetry::TelemetryAnalyzer analyzer(data);

  // Medição de Performance
  auto start = std::chrono::high_resolution_clock::now();
  int overspeed = analyzer.countOverspeedEvents(speedLimit);
  int highRpm = analyzer.countHighRpmEvents(rpmLimit);
  double avgSpeed = analyzer.getAverageSpeed();
  auto maxRpm = analyzer.getMaxRPM();
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  // Saída Formatada
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "=== ANALISE DE TELEMETRIA ===" << std::endl;
  std::cout << "Tempo de processamento: " << duration << " ms" << std::endl;
  std::cout << "Registros: " << data.size() << std::endl;
  std::cout << "Velocidade Media: " << avgSpeed << " km/h" << std::endl;
  std::cout << "RPM Maximo: " << maxRpm.rpm << " RPM" << std::endl;
  std::cout << "--- ALERTAS CRITICOS ---" << std::endl;
  std::cout << "Eventos > " << speedLimit << " km/h: " << overspeed
            << std::endl;
  std::cout << "Eventos > " << rpmLimit << " RPM: " << highRpm << std::endl;

  return 0;
}