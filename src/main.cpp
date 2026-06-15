#include "csv_parser.hpp"
#include "telemetry_analyzer.hpp"
#include "math_channel_evaluator.hpp"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

void printIntro() {
  std::cout << "=== Analisador de Telemetria CLI ===\n";
  std::cout << "Informe os dados solicitados abaixo ou pressione Enter para usar o valor padrao.\n\n";
}

std::string readLine(const std::string &prompt) {
  std::cout << prompt;
  std::string line;
  std::getline(std::cin, line);
  return line;
}

bool fileExists(const std::string &path) {
  return std::filesystem::exists(path);
}

double parseDoubleOrDefault(const std::string &text, double defaultValue) {
  if (text.empty())
    return defaultValue;
  try {
    return std::stod(text);
  } catch (...) {
    std::cout << "Aviso: entrada invalida. Usando valor padrao (" << defaultValue << ").\n";
    return defaultValue;
  }
}

int32_t parseIntOrDefault(const std::string &text, int32_t defaultValue) {
  if (text.empty())
    return defaultValue;
  try {
    return static_cast<int32_t>(std::stoi(text));
  } catch (...) {
    std::cout << "Aviso: entrada invalida. Usando valor padrao (" << defaultValue << ").\n";
    return defaultValue;
  }
}

std::string normalizeOption(std::string value) {
  for (char &c : value)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return value;
}

int main() {
  printIntro();

  std::string csvPath;
  while (csvPath.empty()) {
    csvPath = readLine("1) Caminho do arquivo CSV: ");
    if (csvPath.empty()) {
      std::cout << "Caminho obrigatorio. Tente novamente.\n";
      continue;
    }
    if (!fileExists(csvPath)) {
      std::cout << "Arquivo nao encontrado: " << csvPath << "\n";
      csvPath.clear();
    }
  }

  double speedLimit = 120.0;
  std::string speedText = readLine("2) Limite de velocidade (km/h) [120]: ");
  speedLimit = parseDoubleOrDefault(speedText, speedLimit);

  int32_t rpmLimit = 3500;
  std::string rpmText = readLine("3) Limite de RPM [3500]: ");
  rpmLimit = parseIntOrDefault(rpmText, rpmLimit);

  std::string exportOption = readLine("4) Exportar formato (none/json/csv) [none]: ");
  exportOption = normalizeOption(exportOption.empty() ? "none" : exportOption);
  if (exportOption != "none" && exportOption != "json" && exportOption != "csv") {
    std::cout << "Formato invalido. Usando 'none'.\n";
    exportOption = "none";
  }

  std::string exportPath;
  if (exportOption == "json") {
    exportPath = readLine("Nome do arquivo JSON [report.json]: ");
    if (exportPath.empty())
      exportPath = "report.json";
  } else if (exportOption == "csv") {
    exportPath = readLine("Nome do arquivo CSV [report.csv]: ");
    if (exportPath.empty())
      exportPath = "report.csv";
  }

  try {
    auto startTime = std::chrono::high_resolution_clock::now();

    auto parser = std::make_unique<Telemetry::CsvParser>();
    auto data = parser->parse(csvPath);
    if (data.empty()) {
      std::cerr << "Erro: Nenhum registro encontrado no arquivo." << std::endl;
      return 1;
    }

    // Pergunta por fórmula customizada para Math Channel
    std::string formula = readLine("Digite uma fórmula customizada para o Math Channel (ex: speed * 3.6) ou pressione Enter para pular: ");
    double customMean = 0.0;
    double customMax = 0.0;
    long long customMs = 0;

    if (!formula.empty()) {
      auto t0 = std::chrono::high_resolution_clock::now();
      auto outputs = Telemetry::processarCanalCustomizado(data, formula);
      if (!outputs.empty()) {
        double sum = 0.0;
        double maxv = outputs[0];
        for (double v : outputs) {
          sum += v;
          if (v > maxv)
            maxv = v;
        }
        customMean = sum / static_cast<double>(outputs.size());
        customMax = maxv;
      }
      auto t1 = std::chrono::high_resolution_clock::now();
      customMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

      std::cout << "\n=== MATH CHANNEL (CUSTOM) ===\n";
      std::cout << "Formula: " << formula << "\n";
      std::cout << std::fixed << std::setprecision(3);
      std::cout << "Tempo de processamento do canal: " << customMs << " ms\n";
      std::cout << "Media do canal: " << customMean << "\n";
      std::cout << "Maximo do canal: " << customMax << "\n";
    }

    Telemetry::TelemetryAnalyzer analyzer(data);
    double averageSpeed = analyzer.getAverageSpeed();
    Telemetry::TelemetryRecord maxRpm = analyzer.getMaxRPM();
    double averageAcceleration = analyzer.getAverageAcceleration();
    double maxLateralG = analyzer.getMaxLateralG();
    int overspeed = analyzer.countOverspeedEvents(speedLimit);
    int highRpm = analyzer.countHighRpmEvents(rpmLimit);
    int hardBraking = analyzer.countHardBrakingEvents(-4.0);
    int curveEvents = analyzer.countCurveEvents(0.5);

    std::cout << "\n=== RESUMO DE ANALISE ===\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Registros: " << data.size() << "\n";
    std::cout << "Velocidade media: " << averageSpeed << " km/h\n";
    std::cout << "RPM maximo: " << maxRpm.rpm << " RPM\n";
    std::cout << "Aceleracao media: " << averageAcceleration << " m/s^2\n";
    std::cout << "Maior lateral_g: " << maxLateralG << " g\n";
    std::cout << "Eventos acima de " << speedLimit << " km/h: " << overspeed << "\n";
    std::cout << "Eventos acima de " << rpmLimit << " RPM: " << highRpm << "\n";
    std::cout << "Frenagens bruscas (< -4.0 m/s^2): " << hardBraking << "\n";
    std::cout << "Eventos de curva (> 0.5 g): " << curveEvents << "\n";

    if (exportOption == "json") {
      if (!formula.empty()) {
        analyzer.exportToJSON(exportPath, speedLimit, rpmLimit, formula, customMean, customMax);
      } else {
        analyzer.exportToJSON(exportPath, speedLimit, rpmLimit);
      }
      std::cout << "Relatorio exportado para: " << exportPath << "\n";
    } else if (exportOption == "csv") {
      analyzer.exportToCSV(exportPath, speedLimit, rpmLimit);
      std::cout << "Relatorio exportado para: " << exportPath << "\n";
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    double elapsedSeconds = duration.count() / 1000.0;
    
    std::cout << "\n=== METRICAS DE DESEMPENHO ===\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Tempo total de processamento: " << elapsedSeconds << " segundos (" 
              << duration.count() << " ms)\n";

    return 0;
  } catch (const std::exception &ex) {
    std::cerr << "Erro: " << ex.what() << std::endl;
    return 1;
  }
}
