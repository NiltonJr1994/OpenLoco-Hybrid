#pragma once

#include "Economy/Expenditures.h"
#include "Hybrid/Rct2AssetRegistry.h"
#include "Hybrid/Rct2Graphics.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "SceneManager.h"
#include "World/CompanyManager.h"
#include "World/TownManager.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace OpenLoco::Hybrid::Parks
{
    // This alpha deliberately uses a low construction charge so a fresh test
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
        std::shared_ptr<const Rct2::Definition> entrance;
        uint32_t entranceImage{};
        uint32_t groundImage{};
        struct RideInstance
        {
            std::shared_ptr<const Rct2::Definition> definition;
            uint32_t image{};
        };
        std::vector<RideInstance> rides;

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
    inline std::optional<Park> _preview;
    inline std::optional<uint32_t> _groundImage;
    inline uint16_t _nextParkId{ 1 };
    inline uint16_t _selectedParkId{};
    inline std::string _lastStatus{ "Hybrid park system ready." };

    inline void reset()
    {
        _preview.reset();
        _groundImage.reset();
        _parks.clear();
        _nextParkId = 1;
        _selectedParkId = 0;
        Rct2Graphics::reset();
        _lastStatus = "Native parks are session-only in this alpha.";
    }

    inline bool contains(const World::Pos2& position)
    {
        for (const auto& park : _parks)
        {
            const int dx = std::abs(static_cast<int>(position.x) - park.position.x);
            const int dy = std::abs(static_cast<int>(position.y) - park.position.y);
            if (dx <= kParkFootprintRadius * kTileWorldSize && dy <= kParkFootprintRadius * kTileWorldSize)
            {
                return true;
            }
        }
        return false;
    }

    inline bool hasRct2Assets()
    {
        return !SceneManager::isNetworked() && !SceneManager::isEditorMode() && Rct2Assets::ready();
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

    inline uint32_t groundImage()
    {
        if (!_groundImage)
        {
            _groundImage = Rct2Graphics::loadGround();
        }
        return *_groundImage;
    }

    inline void clearPreview()
    {
        if (_preview)
        {
            _preview.reset();
            Gfx::invalidateScreen();
        }
    }

    inline void preparePreview()
    {
        groundImage();
        Rct2Graphics::load(Rct2Assets::get().entrances.front());
    }

    inline void movePreview(const World::Pos2& position)
    {
        const auto centre = normaliseCentre(position);
        if (_preview && _preview->position == centre)
        {
            return;
        }
        Park preview;
        preview.position = centre;
        preview.entrance = Rct2Assets::get().entrances.front();
        preview.entranceImage = Rct2Graphics::load(preview.entrance);
        preview.groundImage = groundImage();
        _preview = std::move(preview);
        Gfx::invalidateScreen();
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

                const auto* centreSurface = World::TileManager::get(centre).surface();
                if (surface->slope() != 0 || !centreSurface || surface->baseHeight() != centreSurface->baseHeight())
                {
                    reason = "Park site rejected: this slice needs a flat 7x7 site at one height.";
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
        if (SceneManager::isNetworked() || SceneManager::isEditorMode() || SceneManager::isTitleMode() || _parks.size() >= 64)
        {
            _lastStatus = "Native parks require single-player gameplay (maximum 64 parks).";
            return nullptr;
        }
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
        auto entrance = Rct2Assets::get().entrances.front();
        uint32_t entranceImage;
        try
        {
            entranceImage = Rct2Graphics::load(entrance);
            groundImage();
        }
        catch (const std::exception& e)
        {
            _lastStatus = e.what();
            return nullptr;
        }
        _parks.reserve(_parks.size() + 1);
        if (!pay(owner, kParkConstructionCost, ExpenditureType::Construction, position))
        {
            return nullptr;
        }

        Park park{};
        park.id = _nextParkId++;
        park.position = position;
        park.owner = owner;
        park.entrance = entrance;
        park.entranceImage = entranceImage;
        park.groundImage = groundImage();
        refreshClosestTown(park);

        _parks.push_back(std::move(park));
        Gfx::invalidateScreen();
        _selectedParkId = park.id;
        _lastStatus = "Park created with a native RCT2 entrance. Enter park to browse real objects.";
        return &_parks.back();
    }

    inline bool instantiateSelectedRide()
    {
        auto* park = selectedPark();
        auto definition = Rct2Assets::selectedRide();
        if (!park || !definition || SceneManager::isNetworked() || SceneManager::isEditorMode() || SceneManager::isTitleMode() || park->owner != CompanyManager::getControllingId())
        {
            return false;
        }
        if (park->rides.size() >= 9)
        {
            _lastStatus = "This native slice supports nine object instances per park.";
            return false;
        }
        try
        {
            const auto image = Rct2Graphics::load(definition);
            park->rides.push_back({ definition, image });
            _lastStatus = "Instantiated " + definition->id + " in park #" + std::to_string(park->id);
            Gfx::invalidateScreen();
            return true;
        }
        catch (const std::exception& e)
        {
            _lastStatus = e.what();
            return false;
        }
    }

    // No fabricated visitor revenue: operational simulation is outside this slice.
    inline void updateMonthly() {}
}
