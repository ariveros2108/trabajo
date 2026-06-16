#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <string>
#include <curl/curl.h>
#include "api_cache.h"

class APIClient {
private:
    bool use_local_mock;
    std::string base_url;
    std::string global_jwt_token;
    
    std::string get_bearer_token();
    std::string perform_get_request(const std::string& url); // Asegurado como const ref

    // callback
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);

public:
    bool init_session(bool local_mode);
    std::string fetch_gender(const std::string& id, APICache& cache); // Asegurado como const ref
};

#endif
