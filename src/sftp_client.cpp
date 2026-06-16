#include "sftp_client.h"
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <sstream>

SFTPClient::SFTPClient(std::string h, std::string u, std::string p) 
    : host(h), user(u), password(p) {}

size_t SFTPClient::write_file_callback(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::ofstream* out = static_cast<std::ofstream*>(stream);
    size_t total = size * nmemb;
    out->write(static_cast<const char*>(ptr), total);
    return total;
}

size_t SFTPClient::list_directory_callback(void* ptr, size_t size, size_t nmemb, void* data) {
    std::string* output = static_cast<std::string*>(data);
    size_t total = size * nmemb;
    output->append(static_cast<const char*>(ptr), total);
    return total;
}

std::vector<std::string> SFTPClient::list_csv_files() {
    CURL* curl = curl_easy_init();
    std::vector<std::string> files;
    if (!curl) return files;

    std::string raw_list;
    std::string url = "sftp://" + host + "/"; 

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);
    
    // Ignorar certificados y llaves SSH no verificadas
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, list_directory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &raw_list);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res == CURLE_OK) {
        std::stringstream ss(raw_list);
        std::string line;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            // Filtrar solo los archivos solicitados [cite: 60]
            if (line.rfind("reporte_", 0) == 0 && line.find(".csv") != std::string::npos) {
                files.push_back(line);
            }
        }
    }
    return files;
}

bool SFTPClient::download_file(const std::string& remote_file, const std::string& local_path) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = "sftp://" + host + "/" + remote_file;
    std::ofstream out_file(local_path, std::ios::binary);
    
    if (!out_file.is_open()) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out_file);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    out_file.close();

    return (res == CURLE_OK);
}