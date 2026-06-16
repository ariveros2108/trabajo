#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <omp.h>
#include <curl/curl.h> 
#include "sftp_client.h"
#include "api_client.h"
#include "api_cache.h"
#include "csv.h"

namespace fs = std::filesystem;

std::string clean_string(std::string std_str) {
    std_str.erase(0, std_str.find_first_not_of(" \t\r\n\"'"));
    size_t last = std_str.find_last_not_of(" \t\r\n\"'");
    if (last != std::string::npos) std_str.erase(last + 1);
    std::transform(std_str.begin(), std_str.end(), std_str.begin(), ::toupper);
    return std_str;
}

void log_error(const std::string& message) {
    #pragma omp critical
    {
        std::ofstream log_file("log.txt", std::ios::app);
        if (log_file.is_open()) log_file << message << "\n";
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    double start_time = omp_get_wtime();

    std::cout << "=== PROCESAMIENTO ===" << std::endl;
    
    bool local_mode = false; 
    std::cout << "Seleccione entorno: 1. Produccion | 2. Local: ";
    int choice; std::cin >> choice;
    local_mode = (choice == 2);

    APICache api_cache;
    APIClient api_client;
    if (!api_client.init_session(local_mode)) return 1;

    SFTPClient sftp_client("137.184.45.251", "utem", "CPyD.2026");
    fs::create_directory("datos_csv");

    std::vector<std::string> files;
    for (const auto& entry : fs::directory_iterator("datos_csv")) 
        if(entry.path().extension() == ".csv") files.push_back(entry.path().string());

    double sum_femenino = 0.0, sum_masculino = 0.0;
    long count_femenino = 0, count_masculino = 0;
    long total = (long)files.size();
    long procesados = 0;

    std::cout << "Parseando " << total << " archivos..." << std::endl;

    omp_set_num_threads(8);
    #pragma omp parallel for reduction(+:sum_femenino, count_femenino, sum_masculino, count_masculino) schedule(dynamic)
    for (long i = 0; i < total; ++i) {
        try {
            io::CSVReader<3, io::trim_chars<' ', '"'>, io::no_quote_escape<';'>> in(files[i]);
            in.read_header(io::ignore_extra_column, "MONTO APLICADO", "CODIGO CLIENTE", "BOLETA");
            
            std::string r_monto, r_codigo, r_boleta;
            while (in.read_row(r_monto, r_codigo, r_boleta)) {
                std::string id = clean_string(r_codigo);
                float monto = std::stof(clean_string(r_monto));
                std::string genero = clean_string(api_client.fetch_gender(id, api_cache));

                if (genero == "FEMENINO") {
                    sum_femenino += monto; count_femenino++;
                } else if (genero == "MASCULINO") {
                    sum_masculino += monto; count_masculino++;
                }
            }
        } catch (...) { log_error("Error critico en archivo: " + files[i]); }

        #pragma omp atomic
        procesados++;

        if (procesados % 5 == 0 || procesados == total) {
            #pragma omp critical
            {
                std::cout << "\r[PROGRESO] " << procesados << " / " << total << " archivos procesados..." << std::flush;
                if (procesados == total) std::cout << std::endl;
            }
        }
    } 

    
    double p_fem = count_femenino ? sum_femenino/count_femenino : 0;
    double p_masc = count_masculino ? sum_masculino/count_masculino : 0;
    double t_total = omp_get_wtime() - start_time;

    std::cout << "\nFEMENINO = " << (int)p_fem << "\nMANCULINO = " << (int)p_masc << "\nTIEMPO = " << t_total << "s\n";
    
    std::ofstream res("resultados.txt");
    res << "FEMENINO = " << (int)p_fem << "\n" << "MANCULINO = " << (int)p_masc << "\nTIEMPO = " << t_total << "s\n";

    curl_global_cleanup();
    return 0;
} 
