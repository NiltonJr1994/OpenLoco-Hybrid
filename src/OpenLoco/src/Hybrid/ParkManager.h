#pragma once
#include "Date.h"
#include "Economy/Economy.h"
#include "Economy/Expenditures.h"
#include "GameCommands/GameCommands.h"
#include "Hybrid/ParkVisuals.h"
#include "Map/RoadElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TreeElement.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TreeObject.h"
#include "Scenario/ScenarioOptions.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include "World/TownManager.h"
#include <algorithm>
#include <functional>
#include <optional>

namespace OpenLoco::Hybrid::Parks
{
    constexpr currency32_t kParkConstructionCost = 5000; // 1900 base, inflated like native construction.
    constexpr int16_t kTileWorldSize = 32;
    constexpr int16_t kParkFootprintTiles = 7;
    constexpr int16_t kParkFootprintRadius = 3;
    constexpr int16_t kMaxTownDistanceTiles = 48;
    struct Model
    {
        const char* name;
        std::array<uint8_t, 3> rideTypes;
        std::array<World::Pos2, 3> sites;
        int16_t baseCost;
    };
    inline constexpr std::array<Model, 3> kModels = { {
        { "Classic Fair", { 37, 33, 21 }, { { { -2, -2 }, { 1, 1 }, { -2, 2 } } }, 5000 },
        { "Coaster Park", { 52, 37, 33 }, { { { -1, -1 }, { -2, 2 }, { 2, -2 } } }, 6500 },
        { "Family Gardens", { 33, 21, 37 }, { { { -2, 0 }, { 1, -2 }, { 1, 2 } } }, 4500 },
    } };
    struct Park
    {
        uint16_t id{};
        World::Pos2 position{};
        CompanyId owner{ CompanyId::null };
        uint16_t closestTownId{ 0xFFFF };
        uint8_t rotation{}, model{};
        coord_t height{};
        bool open{ true };
        std::shared_ptr<const Rct2::Definition> entrance;
        uint32_t entranceImage{}, groundImage{};
        struct RideInstance
        {
            std::shared_ptr<const Rct2::Definition> definition;
            uint32_t image{};
            ParkVisuals::Kind kind{ ParkVisuals::Kind::object };
            World::Pos2 site{}; // tile offset from park centre, rotated with park
        };
        std::vector<RideInstance> rides;
        struct SceneryInstance
        {
            std::shared_ptr<const Rct2::Definition> definition;
            World::Pos2 tile{}; // Independent 12x12 interior, never regional coordinates.
            uint8_t rotation{};
            uint32_t image{};
        };
        std::vector<SceneryInstance> scenery;
        currency32_t paidInterior{};
        currency32_t paidConstruction{}, lastTax{}, lastOperatingCost{};
        uint32_t lastChargedMonth{};
    };
    struct SiteObjects
    {
        std::function<const RoadObject*(size_t)> road = [](size_t id) { return ObjectManager::get<RoadObject>(id); };
        std::function<const LandObject*(size_t)> land = [](size_t id) { return ObjectManager::get<LandObject>(id); };
        std::function<const TreeObject*(size_t)> tree = [](size_t id) { return ObjectManager::get<TreeObject>(id); };
    };
    struct SiteQuote
    {
        bool valid{};
        coord_t height{};
        currency32_t construction{}, clearance{}, landscaping{};
        uint32_t trees{};
        std::string reason;
        currency32_t total() const { return construction + clearance + landscaping; }
    };
    inline std::vector<Park> _parks;
    inline std::optional<Park> _preview;
    inline std::optional<World::Pos2> _hover;
    inline std::optional<uint32_t> _groundImage;
    inline SiteQuote _quote;
    inline uint16_t _nextParkId{ 1 }, _selectedParkId{};
    inline uint8_t _rotation{}, _model{};
    inline uint32_t _animationTicks{};
    inline std::string _lastStatus{ "Choose a model, rotate its entrance, then Build park." };

    inline uint32_t monthKey() { return getCurrentYear() * 12U + enumValue(getCurrentMonth()); }
    inline currency32_t constructionCost(uint8_t model) { return Economy::getInflationAdjustedCost(kModels.at(model).baseCost, 8, 10); }
    inline currency32_t monthlyTax(size_t objects) { return Economy::getInflationAdjustedCost(static_cast<int16_t>(49 * 2 + objects * 5), 8, 10); }
    inline currency32_t monthlyUpkeep(size_t objects) { return Economy::getInflationAdjustedCost(static_cast<int16_t>(60 + objects * 10), 0, 10); }
    inline World::Pos2 rotate(World::Pos2 pos, uint8_t rotation)
    {
        for (uint8_t r = 0; r < (rotation & 3); ++r)
        {
            pos = { static_cast<coord_t>(-pos.y), pos.x };
        }
        return pos;
    }
    inline World::Pos2 normaliseCentre(const World::Pos2& position) { return World::toWorldSpace(World::toTileSpace(position)); }
    inline std::pair<World::Pos2, World::Pos2> footprintBounds(const World::Pos2& position)
    {
        const auto c = normaliseCentre(position);
        return { c - World::Pos2(96, 96), c + World::Pos2(96, 96) };
    }
    inline bool footprintsOverlap(const World::Pos2& a, const World::Pos2& b)
    {
        const auto [aMin, aMax] = footprintBounds(a);
        const auto [bMin, bMax] = footprintBounds(b);
        return !(aMax.x < bMin.x || bMax.x < aMin.x || aMax.y < bMin.y || bMax.y < aMin.y);
    }
    inline bool contains(const World::Pos2& position)
    {
        const auto c = normaliseCentre(position);
        return std::any_of(_parks.begin(), _parks.end(), [&](const Park& p) { return std::abs(c.x - p.position.x) <= 96 && std::abs(c.y - p.position.y) <= 96; });
    }
    inline Park* getPark(uint16_t id)
    {
        auto it = std::find_if(_parks.begin(), _parks.end(), [=](const Park& p) { return p.id == id; });
        return it == _parks.end() ? nullptr : &*it;
    }
    inline Park* selectedPark() { return getPark(_selectedParkId); }
    inline bool hasRct2Assets() { return !SceneManager::isNetworked() && !SceneManager::isEditorMode() && Rct2Assets::ready(); }
    inline void clearPreview()
    {
        if (_preview)
        {
            _preview.reset();
            Gfx::invalidateScreen();
        }
        _hover.reset();
    }
    inline void reset()
    {
        clearPreview();
        _parks.clear();
        _groundImage.reset();
        ParkVisuals::reset();
        Rct2Graphics::reset();
        _nextParkId = 1;
        _selectedParkId = 0;
        _rotation = 0;
        _model = 0;
        _animationTicks = 0;
        _quote = {};
        _lastStatus = "Build a park or load a save with its .olh companion.";
    }
    inline uint32_t groundImage()
    {
        if (!_groundImage)
        {
            _groundImage = Rct2Graphics::loadGround();
        }
        return *_groundImage;
    }
    inline std::shared_ptr<const Rct2::Definition> definitionFor(uint8_t type)
    {
        for (const auto& d : Rct2Assets::get().rides)
        {
            if (std::find(d->rideTypes.begin(), d->rideTypes.end(), type) != d->rideTypes.end())
            {
                return d;
            }
        }
        throw std::runtime_error("This model needs missing RCT2 ride type " + std::to_string(type));
    }
    inline Park::RideInstance makeRide(std::shared_ptr<const Rct2::Definition> definition, World::Pos2 site)
    {
        auto kind = ParkVisuals::kind(*definition);
        auto image = kind == ParkVisuals::Kind::object ? ParkVisuals::thumbnail(definition) : ParkVisuals::animated(kind);
        return { std::move(definition), image, kind, site };
    }
    inline Park makeModel()
    {
        Park p;
        p.rotation = _rotation;
        p.model = _model;
        p.groundImage = groundImage();
        auto& entrances = Rct2Assets::get().entrances;
        if (entrances.empty())
        {
            throw std::runtime_error("No RCT2 entrance definitions found.");
        }
        p.entrance = entrances[_model % entrances.size()];
        p.entranceImage = ParkVisuals::entrance(p.entrance);
        for (size_t i = 0; i < 3; ++i)
        {
            p.rides.push_back(makeRide(definitionFor(kModels[_model].rideTypes[i]), kModels[_model].sites[i]));
        }
        return p;
    }
    inline void preparePreview() { makeModel(); }
    inline World::Pos2 roadPosition(World::Pos2 centre, uint8_t rotation) { return centre + rotate({ 128, 0 }, rotation); }
    inline bool hasRoadAccess(World::Pos2 centre, uint8_t rotation, coord_t height, const SiteObjects& objects = {})
    {
        const auto pos = roadPosition(centre, rotation);
        if (!World::validCoords(World::toTileSpace(pos)))
        {
            return false;
        }
        const auto tile = World::TileManager::get(pos);
        const auto* surface = tile.surface();
        if (!surface || surface->water() || surface->slope() || surface->baseHeight() != height)
        {
            return false;
        }
        for (const auto& el : tile)
        {
            auto* road = el.as<World::RoadElement>();
            if (!road || road->isGhost() || road->isAiAllocated() || road->hasBridge() || road->baseHeight() != height || (road->roadId() >= 5 && road->roadId() <= 8))
            {
                continue;
            }
            auto* obj = objects.road(road->roadObjectId());
            if (obj && obj->hasFlags(RoadObjectFlags::isRoad) && (road->owner() == CompanyId::neutral || road->owner() == CompanyManager::getControllingId() || obj->hasFlags(RoadObjectFlags::allowUseByAllCompanies)))
            {
                return true;
            }
        }
        return false;
    }
    inline SiteQuote quoteSite(const World::Pos2& inputPosition, uint8_t rotation, uint8_t model, const SiteObjects& objects = {})
    {
        SiteQuote q;
        q.construction = constructionCost(model);
        const auto fail = [&](const char* why) {q.reason=why;return q; };
        if (!Rct2Assets::ready())
        {
            return fail("RCT2 assets are not ready. Check Data and ObjData.");
        }
        const auto centre = normaliseCentre(inputPosition);
        const auto [minPos, maxPos] = footprintBounds(centre);
        if (!World::validCoords(World::toTileSpace(minPos)) || !World::validCoords(World::toTileSpace(maxPos)))
        {
            return fail("The full 7x7 park must fit inside the map.");
        }
        auto* middle = World::TileManager::get(centre).surface();
        if (!middle)
        {
            return fail("No land surface at this site.");
        }
        q.height = middle->baseHeight();
        // Airport-style levelling quote: land cost per height step, applied only
        // after the entire site and funding have been checked. Large hills are refused.
        for (int y = -3; y <= 3; ++y)
        {
            for (int x = -3; x <= 3; ++x)
            {
                auto pos = centre + World::Pos2(x * 32, y * 32);
                const auto tile = World::TileManager::get(pos);
                const auto* s = tile.surface();
                if (!s || s->water())
                {
                    return fail("Cannot build on water: all 49 tiles must be dry land.");
                }
                if (s->isIndustrial())
                {
                    return fail("Industrial land cannot be used for a park.");
                }
                const int slopeRise = s->slope() ? (s->slope() & World::SurfaceSlope::doubleHeight ? 32 : 16) : 0;
                if (std::abs(s->baseHeight() - q.height) > 16 || std::abs(s->baseHeight() + slopeRise - q.height) > 16)
                {
                    return fail("Terrain is too steep. Level this 7x7 site first.");
                }
                const auto steps = std::abs(s->baseZ() - middle->baseZ()) + (s->slope() ? 1 : 0);
                if (steps)
                {
                    auto* land = objects.land(s->terrain());
                    if (!land)
                    {
                        return fail("Missing terrain definition.");
                    }
                    q.landscaping += Economy::getInflationAdjustedCost(land->costFactor, land->costIndex, 10) * steps;
                }
                for (const auto& el : tile)
                {
                    if (el.type() == World::ElementType::surface)
                    {
                        continue;
                    }
                    const auto* tree = el.as<World::TreeElement>();
                    if (!tree || tree->isGhost() || tree->isAiAllocated())
                    {
                        return fail("Roads, rails, buildings or other structures block this site.");
                    }
                    const auto* obj = objects.tree(tree->treeObjectId());
                    if (!obj)
                    {
                        return fail("Missing tree definition.");
                    }
                    q.clearance += Economy::getInflationAdjustedCost(obj->clearCostFactor, obj->costIndex, 12);
                    ++q.trees;
                }
            }
        }
        for (const auto& p : _parks)
        {
            if (footprintsOverlap(p.position, centre))
            {
                return fail("This site overlaps another park.");
            }
        }
        if (!hasRoadAccess(centre, rotation, q.height, objects))
        {
            return fail("Rotate the gate towards an adjacent, level road at ground height.");
        }
        const auto closest = TownManager::getClosestTownAndDensity(centre);
        if (!closest)
        {
            return fail("Build near an existing town.");
        }
        const auto* town = TownManager::get(closest->first);
        if (!town || std::abs(town->x - centre.x) + std::abs(town->y - centre.y) > 48 * 32)
        {
            return fail("Build within 48 tiles of a town centre.");
        }
        q.valid = true;
        q.reason = "Valid site. Click to build; quoted trees/levelling are included.";
        return q;
    }
    inline bool validateParkSite(const World::Pos2& pos, std::string& reason)
    {
        const auto q = quoteSite(pos, _rotation, _model);
        reason = q.reason;
        return q.valid;
    }
    inline void movePreview(const World::Pos2& position, const SiteObjects& objects = {})
    {
        _hover = normaliseCentre(position);
        _quote = quoteSite(*_hover, _rotation, _model, objects);
        _lastStatus = _quote.reason;
        if (!_quote.valid)
        {
            if (_preview)
            {
                _preview.reset();
                Gfx::invalidateScreen();
            }
            return;
        }
        if (_preview && _preview->model == _model && _preview->rotation == _rotation)
        {
            _preview->position = *_hover;
            _preview->height = _quote.height;
        }
        else
        {
            _preview = makeModel();
            _preview->position = *_hover;
            _preview->height = _quote.height;
        }
        Gfx::invalidateScreen();
    }
    inline void clearAndLevel(const World::Pos2& centre, const SiteQuote& quote)
    {
        for (int y = -3; y <= 3; ++y)
        {
            for (int x = -3; x <= 3; ++x)
            {
                const auto pos = centre + World::Pos2(x * 32, y * 32);
                for (;;)
                {
                    auto tile = World::TileManager::get(pos);
                    auto it = std::find_if(tile.begin(), tile.end(), [](const auto& el) { return el.type() == World::ElementType::tree; });
                    if (it == tile.end())
                    {
                        break;
                    }
                    World::TileManager::removeTree(*it, GameCommands::Flags::apply, pos);
                }
                auto* surface = World::TileManager::get(pos).surface();
                surface->setBaseZ(quote.height / World::kSmallZStep);
                surface->setClearZ(surface->baseZ());
                surface->setSlope(0);
                surface->setSnowCoverage(0);
                surface->setGrowthStage(0);
                World::TileManager::setTerrainStyleAsCleared(pos);
            }
        }
    }
    inline Park* createPark(const World::Pos2& inputPosition, const SiteObjects& objects = {})
    {
        if (SceneManager::isNetworked() || SceneManager::isEditorMode() || SceneManager::isTitleMode() || _parks.size() >= 64)
        {
            _lastStatus = "Single-player gameplay only; maximum 64 parks.";
            return nullptr;
        }
        const auto centre = normaliseCentre(inputPosition);
        _quote = quoteSite(centre, _rotation, _model, objects);
        if (!_quote.valid)
        {
            _lastStatus = _quote.reason;
            return nullptr;
        }
        const auto owner = CompanyManager::getControllingId();
        if (owner == CompanyId::null || owner == CompanyId::neutral)
        {
            _lastStatus = "Create a company before building a park.";
            return nullptr;
        }
        Park park;
        try
        {
            park = makeModel();
            _parks.reserve(_parks.size() + 1);
        }
        catch (const std::exception& e)
        {
            _lastStatus = e.what();
            return nullptr;
        }
        if (!CompanyManager::ensureCompanyFunding(owner, _quote.total()))
        {
            _lastStatus = "Not enough company funds for the displayed total.";
            return nullptr;
        }
        // All validation and fallible asset allocation precede terrain changes/payment.
        const auto previous = GameCommands::getUpdatingCompanyId();
        GameCommands::setUpdatingCompanyId(owner);
        clearAndLevel(centre, _quote);
        GameCommands::setUpdatingCompanyId(previous);
        CompanyManager::applyPaymentToCompany(owner, _quote.total(), ExpenditureType::Construction);
        park.id = _nextParkId++;
        park.position = centre;
        park.owner = owner;
        park.height = _quote.height;
        park.paidConstruction = _quote.total();
        park.lastChargedMonth = monthKey();
        if (auto town = TownManager::getClosestTownAndDensity(centre))
        {
            park.closestTownId = enumValue(town->first);
        }
        _selectedParkId = park.id;
        _parks.push_back(std::move(park));
        clearPreview();
        Scenario::getOptions().madeAnyChanges = 1;
        Gfx::invalidateScreen();
        _lastStatus = "Park built. Enter park to inspect its RCT2 objects and monthly costs.";
        return &_parks.back();
    }
    inline bool instantiateSelectedRide()
    {
        auto* park = selectedPark();
        auto def = Rct2Assets::selectedRide();
        if (!park || !def || SceneManager::isNetworked() || SceneManager::isEditorMode() || SceneManager::isTitleMode() || park->owner != CompanyManager::getControllingId())
        {
            return false;
        }
        if (park->rides.size() >= 9)
        {
            _lastStatus = "Maximum nine objects in this park.";
            return false;
        }
        try
        {
            World::Pos2 site;
            bool found = false;
            for (int y = -2; y <= 2 && !found; y += 2)
            {
                for (int x = -2; x <= 2 && !found; x += 2)
                {
                    site = { static_cast<coord_t>(x), static_cast<coord_t>(y) };
                    found = std::none_of(park->rides.begin(), park->rides.end(), [&](const auto& r) { return r.site == site; });
                }
            }
            park->rides.push_back(makeRide(def, site));
            _lastStatus = "Object added; monthly tax and upkeep updated.";
            Gfx::invalidateScreen();
            return true;
        }
        catch (const std::exception& e)
        {
            _lastStatus = e.what();
            return false;
        }
    }
    inline void updateMonthly()
    {
        if (SceneManager::isNetworked() || SceneManager::isEditorMode() || SceneManager::isTitleMode())
        {
            return;
        }
        for (auto& p : _parks)
        {
            if (p.lastChargedMonth == monthKey())
            {
                continue;
            }
            p.lastChargedMonth = monthKey();
            p.lastTax = monthlyTax(p.rides.size());
            p.lastOperatingCost = monthlyUpkeep(p.rides.size());
            // Same monthly debit path as VehicleHead::updateMonthly; no forced loan.
            CompanyManager::applyPaymentToCompany(p.owner, p.lastTax + p.lastOperatingCost, ExpenditureType::Miscellaneous);
        }
        Ui::WindowManager::invalidate(Ui::WindowType::hybridParks);
    }
    inline void tick()
    {
        if ((++_animationTicks % 4) != 0)
        {
            return;
        }
        for (const auto& p : _parks)
        {
            for (const auto& r : p.rides)
            {
                World::TileManager::mapInvalidateTileFull(p.position + rotate({ static_cast<coord_t>(r.site.x * 32), static_cast<coord_t>(r.site.y * 32) }, p.rotation));
            }
        }
        if (_preview)
        {
            Gfx::invalidateScreen();
        }
    }
}
