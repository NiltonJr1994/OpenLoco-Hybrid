#pragma once

#include "Economy/Expenditures.h"
#include "Hybrid/Rct2AssetRegistry.h"
#include "Map/TileManager.h"
#include "SceneManager.h"
#include "World/CompanyManager.h"
#include "World/TownManager.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace OpenLoco::Hybrid::Parks
{
    // v0.4 alpha deliberately uses a low construction charge so a fresh test
    // scenario can exercise the complete park flow without taking a large loan.
    // The final balance model will derive land price from the site and era.
    constexpr currency32_t kParkConstructionCost = 5000;
    constexpr int16_t kTileWorldSize = 32;
    constexpr int16_t kParkFootprintTiles = 7;
    constexpr int16_t kParkFootprintRadius = kParkFootprintTiles / 2;
    constexpr int16_t kMaxTownDistanceTiles = 48;

    struct Park
    {
        uint16_t id{};
        World::Pos2 position{}; // centre tile of the regional 7x7 site
        CompanyId owner{ CompanyId::null };
        uint16_t closestTownId{ 0xFFFF };
        bool open{ true };
        bool detailedParkLaunched{};

        uint16_t popularity{ 550 };
        uint16_t capacity{ 250 };
        currency32_t ticketPrice{ 20 };
        uint32_t visitorsLastMonth{};
        uint64_t lifetimeVisitors{};
        currency32_t lastRevenue{};
        currency32_t lastTax{};
        currency32_t lastOperatingCost{};
        currency32_t lastProfit{};
    };

    inline std::vector<Park> _parks{};
    inline uint16_t _nextParkId{ 1 };
    inline uint16_t _selectedParkId{};
    inline std::string _lastStatus{ "Hybrid park system ready." };

    inline bool hasRct2Assets()
    {
        return Rct2Assets::ready();
    }

    inline World::Pos2 normaliseCentre(const World::Pos2& position)
    {
        return World::toWorldSpace(World::toTileSpace(position));
    }

    inline std::pair<World::Pos2, World::Pos2> footprintBounds(const World::Pos2& position)
    {
        const auto centre = normaliseCentre(position);
        constexpr int16_t radiusWorld = kParkFootprintRadius * kTileWorldSize;
        return {
            World::Pos2{ static_cast<coord_t>(centre.x - radiusWorld), static_cast<coord_t>(centre.y - radiusWorld) },
            World::Pos2{ static_cast<coord_t>(centre.x + radiusWorld), static_cast<coord_t>(centre.y + radiusWorld) },
        };
    }

    inline bool footprintsOverlap(const World::Pos2& a, const World::Pos2& b)
    {
        const auto [aMin, aMax] = footprintBounds(a);
        const auto [bMin, bMax] = footprintBounds(b);
        return !(aMax.x < bMin.x || bMax.x < aMin.x || aMax.y < bMin.y || bMax.y < aMin.y);
    }

    inline Park* getPark(uint16_t id)
    {
        const auto it = std::find_if(_parks.begin(), _parks.end(), [id](const Park& p) { return p.id == id; });
        return it == _parks.end() ? nullptr : &*it;
    }

    inline Park* selectedPark()
    {
        return getPark(_selectedParkId);
    }

    inline uint32_t closestTownPopulation(const Park& park)
    {
        if (park.closestTownId == 0xFFFF)
        {
            return 0;
        }
        auto* town = TownManager::get(TownId(park.closestTownId));
        return town == nullptr ? 0 : town->population;
    }

    inline void refreshClosestTown(Park& park)
    {
        const auto result = TownManager::getClosestTownAndDensity(park.position);
        park.closestTownId = result.has_value() ? enumValue(result->first) : 0xFFFF;
    }

    inline bool validateParkSite(const World::Pos2& inputPosition, std::string& reason)
    {
        if (!Rct2Assets::ready())
        {
            reason = Rct2Assets::get().status;
            return false;
        }

        const auto centre = normaliseCentre(inputPosition);
        const auto closest = TownManager::getClosestTownAndDensity(centre);
        if (!closest.has_value())
        {
            reason = "A regional park must be built near an existing town.";
            return false;
        }

        auto* town = TownManager::get(closest->first);
        if (town == nullptr)
        {
            reason = "The nearest town could not be resolved.";
            return false;
        }

        const auto distanceWorld = std::abs(static_cast<int32_t>(town->x) - centre.x)
            + std::abs(static_cast<int32_t>(town->y) - centre.y);
        if ((distanceWorld / kTileWorldSize) > kMaxTownDistanceTiles)
        {
            reason = "Park site rejected: it must be within 48 tiles of a town centre.";
            return false;
        }

        for (const auto& existing : _parks)
        {
            if (footprintsOverlap(existing.position, centre))
            {
                reason = "Park site rejected: the 7x7 area overlaps another Hybrid park.";
                return false;
            }
        }

        for (int16_t y = -kParkFootprintRadius; y <= kParkFootprintRadius; ++y)
        {
            for (int16_t x = -kParkFootprintRadius; x <= kParkFootprintRadius; ++x)
            {
                const World::Pos2 worldPos{
                    static_cast<coord_t>(centre.x + x * kTileWorldSize),
                    static_cast<coord_t>(centre.y + y * kTileWorldSize),
                };
                const auto tilePos = World::toTileSpace(worldPos);
                if (!World::validCoords(tilePos))
                {
                    reason = "Park site rejected: the full 7x7 footprint must fit inside the map.";
                    return false;
                }

                const auto tile = World::TileManager::get(tilePos);
                const auto* surface = tile.surface();
                if (surface == nullptr || surface->water())
                {
                    reason = "Park site rejected: all 49 tiles must be dry land.";
                    return false;
                }

                // For this alpha, do not silently bulldoze roads, rails, stations,
                // buildings, industries or scenery. A park site must be completely
                // clear. Later builds can offer an explicit clearance cost preview.
                size_t elementCount = 0;
                for ([[maybe_unused]] const auto& element : tile)
                {
                    ++elementCount;
                }
                if (elementCount > 1)
                {
                    reason = "Park site rejected: the highlighted 7x7 area must be clear of roads, tracks, buildings and scenery.";
                    return false;
                }
            }
        }

        reason.clear();
        return true;
    }

    inline bool pay(CompanyId owner, currency32_t amount, ExpenditureType type, const World::Pos2& position)
    {
        if (amount <= 0)
        {
            return true;
        }
        if (!CompanyManager::ensureCompanyFunding(owner, amount))
        {
            _lastStatus = "Not enough company funds. Hybrid alpha park construction costs 5,000.";
            return false;
        }

        CompanyManager::applyPaymentToCompany(owner, amount, type);
        CompanyManager::spendMoneyEffect(World::Pos3{ position.x, position.y, 24 }, owner, amount);
        return true;
    }

    inline Park* createPark(const World::Pos2& inputPosition)
    {
        const auto position = normaliseCentre(inputPosition);
        std::string siteError;
        if (!validateParkSite(position, siteError))
        {
            _lastStatus = siteError;
            return nullptr;
        }

        const auto owner = CompanyManager::getControllingId();
        if (owner == CompanyId::null)
        {
            _lastStatus = "No controlling company is available.";
            return nullptr;
        }
        if (!pay(owner, kParkConstructionCost, ExpenditureType::Construction, position))
        {
            return nullptr;
        }

        Park park{};
        park.id = _nextParkId++;
        park.position = position;
        park.owner = owner;
        refreshClosestTown(park);

        _parks.push_back(park);
        _selectedParkId = park.id;
        _lastStatus = "Regional park created. Use ENTER PARK to open the real RCT2 detailed park layer.";
        return &_parks.back();
    }

    inline uint32_t estimateMonthlyVisitors(const Park& park)
    {
        if (!park.open || !park.detailedParkLaunched)
        {
            return 0;
        }
        const uint64_t population = closestTownPopulation(park);
        if (population == 0)
        {
            return 0;
        }

        // Temporary regional placeholder until OpenRCT2 park statistics are
        // synchronised back through the Hybrid sidecar bridge.
        uint64_t visitors = population * 4 / 100;
        visitors = visitors * park.popularity / 1000;
        const int32_t priceDemandPercent = std::clamp<int32_t>(120 - park.ticketPrice, 25, 120);
        visitors = visitors * priceDemandPercent / 100;
        return static_cast<uint32_t>(std::min<uint64_t>(visitors, static_cast<uint64_t>(park.capacity) * 30));
    }

    inline void updateMonthly()
    {
        if (SceneManager::isEditorMode() || SceneManager::isTitleMode() || SceneManager::isNetworked())
        {
            return;
        }

        for (auto& park : _parks)
        {
            if (!park.open || !park.detailedParkLaunched)
            {
                continue;
            }

            refreshClosestTown(park);
            const auto visitors = estimateMonthlyVisitors(park);
            park.visitorsLastMonth = visitors;
            park.lifetimeVisitors += visitors;

            park.lastRevenue = static_cast<currency32_t>(visitors) * park.ticketPrice;
            park.lastTax = static_cast<currency32_t>(500 + park.capacity / 10);
            park.lastOperatingCost = static_cast<currency32_t>(900 + visitors);
            park.lastProfit = park.lastRevenue - park.lastTax - park.lastOperatingCost;
            CompanyManager::applyPaymentToCompany(park.owner, -park.lastProfit, ExpenditureType::Miscellaneous);
        }
    }
}
