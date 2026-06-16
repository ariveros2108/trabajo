#pragma once
#include <unordered_map>
#include <string>
#include <shared_mutex> 
#include <mutex>       
#include <optional>

class APICache {
private:
    std::unordered_map<std::string, std::string> cache;
    std::shared_mutex mutex;

public:
    std::optional<std::string> get(const std::string& uuid) {
        std::shared_lock<std::shared_mutex> lock(mutex);
        auto it = cache.find(uuid);
        if (it != cache.end()) return it->second;
        return std::nullopt;
    }

    void put(const std::string& uuid, const std::string& gender) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        cache[uuid] = gender;
    }
};
