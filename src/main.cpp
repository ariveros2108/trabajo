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

bool es_archivo_valido(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string primera_linea;
    if (!std::getline(file, primera_linea)) return false;
    return (primera_linea.find("MONTO") != std::string::npos && 
            primera_linea.find("CODIGO") != std::string::npos);
}

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

    std::cout << "\nConectando a SFTP (137.184.45.251)..." << std::endl;
    SFTPClient sftp_client("137.184.45.251", "utem", "CPyD.2026");
    std::vector<std::string> remote_files = sftp_client.list_csv_files();
    
    std::string local_dir = "datos_csv";
    fs::create_directory(local_dir);

    std::vector<std::string> to_download;
    for(const auto& file : remote_files) {
        std::string path = local_dir + "/" + file;
        bool corrupto = false;
        if(fs::exists(path)) {
            if(!es_archivo_valido(path)) {
                corrupto = true;
                std::cout << "[ALERTA] Archivo corrupto detectado: " << file << ". Re-descargando..." << std::endl;
            }
        }
        if(!fs::exists(path) || corrupto) {
            to_download.push_back(file);
        }
    }

    long pending_files = static_cast<long>(to_download.size());
    std::cout << "Archivos totales en SFTP: " << remote_files.size() << " | Pendientes por descargar: " << pending_files << std::endl;

    omp_set_num_threads(8); // Configuramos 8 hilos globales para descarga y parseo

    if (pending_files > 0) {
        long downloaded = 0;
        #pragma omp parallel for schedule(dynamic)
        for (long i = 0; i < pending_files; ++i) {
            std::string path = local_dir + "/" + to_download[i];
            if (sftp_client.download_file(to_download[i], path)) {
                #pragma omp atomic
                downloaded++;
                
                if (downloaded % 10 == 0 || downloaded == pending_files) {
                    #pragma omp critical
                    std::cout << "\r[DESCARGA] " << downloaded << " / " << pending_files << " archivos..." << std::flush;
                }
            } else {
                log_error("Error al descargar archivo SFTP: " + to_download[i]);
            }
        }
        std::cout << std::endl;
    }
    // ----------------------------------------

    // --- PROCESAMIENTO CSV ---
    std::vector<std::string> files_to_process;
    for (const auto& entry : fs::directory_iterator(local_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") {
            files_to_process.push_back(entry.path().string());
        }
    }

    double sum_femenino = 0.0, sum_masculino = 0.0;
    long count_femenino = 0, count_masculino = 0;
    long total = static_cast<long>(files_to_process.size());
    long procesados = 0;

    std::cout << "\nParseando " << total << " archivos..." << std::endl;

    #pragma omp parallel for reduction(+:sum_femenino, count_femenino, sum_masculino, count_masculino) schedule(dynamic)
    for (long i = 0; i < total; ++i) {
        try {
            io::CSVReader<3, io::trim_chars<' ', '"'>, io::no_quote_escape<';'>> in(files_to_process[i]);
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
        } catch (...) { log_error("Error critico en archivo: " + files_to_process[i]); }

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

    // --- RESULTADOS ---
    double p_fem = count_femenino ? sum_femenino/count_femenino : 0;
    double p_masc = count_masculino ? sum_masculino/count_masculino : 0;
    double t_total = omp_get_wtime() - start_time;

    std::cout << "\n=== RESULTADOS FINALES ===" << std::endl;
    std::cout << "FEMENINO = " << (long)p_fem << std::endl;
    std::cout << "MASCULINO = " << (long)p_masc << std::endl;
    std::cout << "TIEMPO = " << t_total << " segundos" << std::endl;
    
    std::ofstream res("resultados.txt");
    if (res.is_open()) {
        res << "FEMENINO = " << (long)p_fem << "\n";
        res << "MASCULINO = " << (long)p_masc << "\n";
        res << "TIEMPO = " << t_total << " segundos\n";
    }

    curl_global_cleanup();
    return 0;
}
