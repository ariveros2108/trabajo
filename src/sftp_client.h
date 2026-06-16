#pragma once
#include <string>
#include <vector>

class SFTPClient {
private:
    std::string host;
    std::string user;
    std::string password;

    static size_t write_file_callback(void* ptr, size_t size, size_t nmemb, void* stream);
    static size_t list_directory_callback(void* ptr, size_t size, size_t nmemb, void* data);

public:
    SFTPClient(std::string h, std::string u, std::string p);
    std::vector<std::string> list_csv_files();
    bool download_file(const std::string& remote_file, const std::string& local_path);
};