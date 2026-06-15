#include <fstream>
#include <iostream>
#include <random>
#include <string>

int main() {
    // Definindo 90 milhões de registros para garantir ~2.2 GB
    const long long TOTAL_RECORDS = 90000000; 
    std::string file_path = "data/large_test.csv";
    
    std::cout << "Gerando " << file_path << " com " << TOTAL_RECORDS << " registros..." << std::endl;

    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Erro: Nao foi possivel criar o arquivo em " << file_path << std::endl;
        return 1;
    }

    file << "id,speed,rpm\n";

    std::mt19937 rng(1337);
    std::uniform_real_distribution<double> speed_dist(20.0, 150.0);
    std::uniform_int_distribution<int> rpm_dist(800, 7000);

    // Geracao dos dados
    for (long long i = 0; i < TOTAL_RECORDS; ++i) {
        file << i << "," << speed_dist(rng) << "," << rpm_dist(rng) << "\n";
        
        // Log de progresso a cada 5 milhoes de registros para nao travar o console
        if (i > 0 && i % 5000000 == 0) {
            std::cout << "Progresso: " << (i / 1000000) << " milhoes de registros escritos..." << std::endl;
        }
    }

    file.close();
    std::cout << "Arquivo gigante gerado com sucesso!" << std::endl;
    return 0;
}