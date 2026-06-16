#include "api_client.h"
#include <iostream>
#include <unistd.h>

// Callback estático requerido por libcurl
size_t APIClient::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total_size = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

bool APIClient::init_session(bool local_mode) {
    this->use_local_mock = local_mode;
    this->base_url = local_mode ? "http://localhost:8080" : "https://api.sebastian.cl";
    
    this->global_jwt_token = get_bearer_token(); 
    return !this->global_jwt_token.empty();
}

std::string APIClient::get_bearer_token() {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string url = this->base_url + "/cpyd/v1/login/authenticate";
    std::string json_payload = "{\"email\": \"ariverosa@utem.cl\", \"rut\": \"20.832.429-2\"}";
    std::string response_string;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, APIClient::write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);         

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    size_t token_start = response_string.find("eyJ");
    if (token_start != std::string::npos) {
        size_t token_end = response_string.find("\"", token_start);
        if (token_end != std::string::npos) {
            return response_string.substr(token_start, token_end - token_start);
        }
    }
    return "";
}

std::string APIClient::perform_get_request(const std::string& url) {
    int retries = 0;
    std::string response_string;
    long http_code = 0;

    while (retries < 2) {
        if (this->global_jwt_token.empty()) {
            if (!init_session(this->use_local_mock)) return "";
        }

        CURL* curl = curl_easy_init();
        if (!curl) return "";

        response_string.clear();
        struct curl_slist* headers = NULL;
        std::string auth_header = "Authorization: Bearer " + this->global_jwt_token;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, APIClient::write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);          

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (http_code == 401) {
            this->global_jwt_token = ""; 
            retries++;
        } else if (res == CURLE_OK && http_code == 200) {
            return response_string;
        } else {
            break; 
        }
    }
    return "";
}

std::string APIClient::fetch_gender(const std::string& id, APICache& cache) {
    auto cached_gender = cache.get(id);
    if (cached_gender) return *cached_gender;

    std::string gender = "DESCONOCIDO";
    if (!id.empty()) {
        std::string url = this->base_url + "/cpyd/v1/person/" + id;
        std::string json_response = perform_get_request(url);

        // Extractor ROBUSTO que ignora basura en el buffer
        size_t start = json_response.find("{\"gender\"");
        if (start == std::string::npos) start = json_response.find("\"gender\"");
        
        if (start != std::string::npos) {
            size_t colon = json_response.find(":", start);
            size_t val_start = json_response.find("\"", colon);
            size_t val_end = json_response.find("\"", val_start + 1);
            
            if (val_start != std::string::npos && val_end != std::string::npos) {
                gender = json_response.substr(val_start + 1, val_end - val_start - 1);
            }
        }
    }
    
    // PAUSA
    usleep(1000); 
    cache.put(id, gender);
    return gender;
}
