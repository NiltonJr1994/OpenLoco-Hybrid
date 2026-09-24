#pragma once
#include "Hybrid/ParkManager.h"
#include <filesystem>
#include <span>

namespace OpenLoco::Hybrid::ParkPersistence
{
    std::filesystem::path sidecarPath(const std::filesystem::path& save);
    uint64_t fingerprint(const std::filesystem::path& save);
    std::vector<uint8_t> encode(std::span<const Parks::Park> parks, uint64_t saveFingerprint);
    std::vector<Parks::Park> decode(std::span<const uint8_t> bytes, uint64_t saveFingerprint);
    void save(const std::filesystem::path& path);
    std::vector<Parks::Park> read(const std::filesystem::path& path);
    void restore(std::vector<Parks::Park> parks);
    void reportError(const std::exception& error);
}
