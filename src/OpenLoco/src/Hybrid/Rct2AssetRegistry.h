#pragma once

#include <OpenLoco/Core/FileSystem.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <OpenLoco/Platform/Platform.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace OpenLoco::Hybrid::Rct2Assets
{
    struct Registry
    {
        fs::path root{};
        fs::path g1{};
        std::array<uint32_t, 16> objectTypes{};
        uint32_t dataFiles{};
        uint32_t objectFiles{};
        uint32_t trackDesignFiles{};
        uint32_t scenarioFiles{};
        uint32_t savedParkFiles{};
        std::vector<fs::path> scenarios{};
        std::vector<fs::path> savedParks{};
        bool scanned{};
        bool ready{};
        std::string status{ "RCT2 assets have not been scanned yet." };
    };

    inline Registry _registry{};
    inline size_t _selectedScenarioIndex{};

    inline std::string lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    inline bool extensionEquals(const fs::path& path, const char* extension)
    {
        return lower(path.extension().string()) == extension;
    }

    inline fs::path rootPath()
    {
        return Platform::getCurrentExecutablePath().parent_path() / "RCT2";
    }

    inline bool readObjectHeader(const fs::path& path, uint8_t& objectType)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            return false;
        }

        std::array<uint8_t, 16> header{};
        stream.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
        if (stream.gcount() != static_cast<std::streamsize>(header.size()))
        {
            return false;
        }

        // RCT2 legacy object header: flags[4], internal name[8], checksum[4].
        // The lower four flag bits identify the object class.
        objectType = static_cast<uint8_t>(header[0] & 0x0F);
        return objectType < 16;
    }

    inline void scanFlatDirectory(const fs::path& directory, const char* extension, uint32_t& count, std::vector<fs::path>* files = nullptr)
    {
        if (!fs::is_directory(directory))
        {
            return;
        }

        for (const auto& entry : fs::directory_iterator(directory))
        {
            if (!entry.is_regular_file() || !extensionEquals(entry.path(), extension))
            {
                continue;
            }
            ++count;
            if (files != nullptr)
            {
                files->push_back(entry.path());
            }
        }
    }

    inline void choosePreferredScenario()
    {
        _selectedScenarioIndex = 0;
        if (_registry.scenarios.empty())
        {
            return;
        }

        constexpr const char* kPreferred = "build your own six flags park.sc6";
        const auto it = std::find_if(_registry.scenarios.begin(), _registry.scenarios.end(), [](const fs::path& p) {
            return lower(p.filename().string()) == kPreferred;
        });
        if (it != _registry.scenarios.end())
        {
            _selectedScenarioIndex = static_cast<size_t>(std::distance(_registry.scenarios.begin(), it));
        }
    }

    inline const fs::path* selectedScenario()
    {
        if (_registry.scenarios.empty())
        {
            return nullptr;
        }
        if (_selectedScenarioIndex >= _registry.scenarios.size())
        {
            _selectedScenarioIndex = 0;
        }
        return &_registry.scenarios[_selectedScenarioIndex];
    }

    inline void selectPreviousScenario()
    {
        if (_registry.scenarios.empty())
        {
            return;
        }
        if (_selectedScenarioIndex == 0)
        {
            _selectedScenarioIndex = _registry.scenarios.size() - 1;
        }
        else
        {
            --_selectedScenarioIndex;
        }
    }

    inline void selectNextScenario()
    {
        if (_registry.scenarios.empty())
        {
            return;
        }
        _selectedScenarioIndex = (_selectedScenarioIndex + 1) % _registry.scenarios.size();
    }

    inline void scan()
    {
        Registry next{};
        next.root = rootPath();
        next.scanned = true;

        try
        {
            const auto data = next.root / "Data";
            const auto objData = next.root / "ObjData";
            const auto tracks = next.root / "Tracks";
            const auto scenarios = next.root / "Scenarios";
            const auto savedGames = next.root / "Saved Games";

            next.g1 = data / "g1.dat";
            if (!fs::is_regular_file(next.g1))
            {
                next.status = "RCT2/Data/g1.dat was not found beside OpenLoco.exe.";
                _registry = std::move(next);
                return;
            }

            if (fs::is_directory(data))
            {
                for (const auto& entry : fs::directory_iterator(data))
                {
                    if (entry.is_regular_file())
                    {
                        ++next.dataFiles;
                    }
                }
            }

            if (fs::is_directory(objData))
            {
                for (const auto& entry : fs::directory_iterator(objData))
                {
                    if (!entry.is_regular_file() || !extensionEquals(entry.path(), ".dat"))
                    {
                        continue;
                    }
                    uint8_t type{};
                    if (readObjectHeader(entry.path(), type))
                    {
                        ++next.objectFiles;
                        ++next.objectTypes[type];
                    }
                }
            }

            scanFlatDirectory(tracks, ".td6", next.trackDesignFiles);
            scanFlatDirectory(scenarios, ".sc6", next.scenarioFiles, &next.scenarios);
            scanFlatDirectory(savedGames, ".sv6", next.savedParkFiles, &next.savedParks);
            uint32_t parkFormatCount{};
            scanFlatDirectory(savedGames, ".park", parkFormatCount, &next.savedParks);
            next.savedParkFiles += parkFormatCount;

            std::sort(next.scenarios.begin(), next.scenarios.end());
            std::sort(next.savedParks.begin(), next.savedParks.end());

            const bool hasRides = next.objectTypes[0] > 0;
            const bool hasParkEntrances = next.objectTypes[8] > 0;
            next.ready = next.objectFiles > 0 && hasRides && hasParkEntrances && next.scenarioFiles > 0;
            if (next.ready)
            {
                next.status = "RCT2 asset registry ready.";
            }
            else
            {
                next.status = "RCT2 folder found, but required ride/entrance/scenario data is incomplete.";
            }
        }
        catch (const std::exception& e)
        {
            next.ready = false;
            next.status = std::string("RCT2 asset scan failed: ") + e.what();
        }

        _registry = std::move(next);
        choosePreferredScenario();
    }

    inline const Registry& get()
    {
        if (!_registry.scanned)
        {
            scan();
        }
        return _registry;
    }

    inline bool ready()
    {
        return get().ready;
    }
}
