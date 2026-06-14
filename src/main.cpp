#include "telemetry_analyzer.hpp"
#include "telemetry_parser.hpp"
#include <iostream>


int main(int argc, char* argv[]) {
    std::string csvPath = "data/test.csv";
    std::string reportPath = "data/relatorio.txt";

    // Help message
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "Uso: " << argv[0] << " [caminho_csv] [caminho_relatorio]\n"
                  << "  caminho_csv       - opcional, caminho para arquivo CSV (default data/test.csv)\n"
                  << "  caminho_relatorio - opcional, caminho para o relatorio (default data/relatorio.txt)\n";
        return 0;
    }

    if (argc > 3) {
        std::cerr << "Numero de argumentos invalido.\n";
        std::cerr << "Uso: " << argv[0] << " [caminho_csv] [caminho_relatorio]\n";
        return 1;
    }
    if (argc >= 2) csvPath = argv[1];
    if (argc == 3) reportPath = argv[2];

    Telemetry::TelemetryParser parser;
    auto data = parser.loadFromCSV(csvPath);
    if (data.empty()) {
        std::cout << "Nenhum dado carregado." << std::endl;
        return 1;
    }

    Telemetry::TelemetryAnalyzer analyzer(data);

    const double speedLimit = 100.0;
    const int32_t rpmLimit = 3000;
    int overspeed = analyzer.countOverspeedEvents(speedLimit);
    int highRpm = analyzer.countHighRpmEvents(rpmLimit);

    std::cout << "=== ANALISE DE TELEMETRIA ===" << std::endl;
    std::cout << "Registros processados: " << data.size() << std::endl;
    std::cout << "Velocidade Media: " << analyzer.getAverageSpeed() << " km/h" << std::endl;
    std::cout << "RPM Maximo: " << analyzer.getMaxRPM().rpm << " RPM" << std::endl;
    std::cout << "--- ALERTAS CRITICOS ---" << std::endl;
    std::cout << "Eventos acima de " << speedLimit << " km/h: " << overspeed << std::endl;
    std::cout << "Eventos acima de " << rpmLimit << " RPM: " << highRpm << std::endl;

    analyzer.generateReport(reportPath, speedLimit, rpmLimit);
    std::cout << "Relatorio exportado para: " << reportPath << std::endl;
    return 0;
}