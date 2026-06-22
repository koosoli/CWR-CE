#pragma once

#include <Poseidon/World/Model/Model.hpp>
#include <string>
#include <map>
#include <memory>

namespace Poseidon {

namespace Model {
    struct Model;
}

class ModelCache {
public:
    ModelCache() = default;
    ~ModelCache() = default;

    ModelCache(const ModelCache&) = delete;
    ModelCache& operator=(const ModelCache&) = delete;

    std::shared_ptr<Model::Model> load(const std::string& filePath);

    bool isLoaded(const std::string& filePath) const;

    std::shared_ptr<Model::Model> get(const std::string& filePath) const;

    bool unload(const std::string& filePath);

    void clear();

    size_t size() const { return _cache.size(); }

    struct Stats {
        size_t totalLoads = 0;
        size_t cacheHits = 0;
        size_t cacheMisses = 0;
        size_t loadFailures = 0;
        size_t cachedModels = 0;
    };

    Stats getStats() const { return _stats; }
    void resetStats() { _stats = Stats{0, 0, 0, 0, _cache.size()}; }

private:
    std::string normalizePath(const std::string& path) const;

    std::shared_ptr<Model::Model> loadFromFile(const std::string& filePath);

    std::shared_ptr<Model::Model> loadFromFileServer(const std::string& filePath);

    std::map<std::string, std::shared_ptr<Model::Model>> _cache;
    mutable Stats _stats;
};

} // namespace Poseidon

