#pragma once

#include "Economy/Expenditures.h"
#include "SceneManager.h"
#include "World/CompanyManager.h"
#include "World/TownManager.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace OpenLoco::Hybrid::Parks
{
    enum class AttractionType : uint8_t
    {
        carousel,
        ferrisWheel,
        compactCoaster,
        foodStall,
    };

    struct Park
    {
        uint16_t id{};
        World::Pos2 position{};
        CompanyId owner{ CompanyId::null };
        uint16_t closestTownId{ 0xFFFF };

        bool open{ true };
        uint16_t popularity{ 550 };
        uint16_t capacity{ 250 };
        currency32_t ticketPrice{ 20 };

        uint16_t carouselCount{};
        uint16_t ferrisWheelCount{};
        uint16_t coasterCount{};
        uint16_t foodStallCount{};

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

    constexpr currency32_t kParkConstructionCost = 75000;

    inline uint16_t attractionCount(const Park& park)
    {
        return park.carouselCount + park.ferrisWheelCount + park.coasterCount;
    }

    inline Park* getPark(uint16_t id)
    {
        auto it = std::find_if(_parks.begin(), _parks.end(), [id](const Park& p) { return p.id == id; });
        return it == _parks.end() ? nullptr : &*it;
    }

    inline const Park* getPark(uint16_t id, int)
    {
        auto it = std::find_if(_parks.begin(), _parks.end(), [id](const Park& p) { return p.id == id; });
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
        if (!result.has_value())
        {
            park.closestTownId = 0xFFFF;
            return;
        }
        park.closestTownId = enumValue(result->first);
    }

    inline uint32_t estimateMonthlyVisitors(const Park& park)
    {
        if (!park.open || attractionCount(park) == 0)
        {
            return 0;
        }

        const uint64_t population = closestTownPopulation(park);
        if (population == 0)
        {
            return 0;
        }

        // Regional alpha model: each real attraction increases the share of the
        // nearest town that is willing to visit. Popularity and park capacity
        // then cap the actual attendance. Transport routing replaces part of
        // this equation in the next layer.
        const uint32_t attractionScore = std::min<uint32_t>(42, 4 + attractionCount(park) * 6 + park.foodStallCount * 2);
        uint64_t visitors = population * attractionScore / 100;
        visitors = visitors * park.popularity / 1000;

        const uint64_t monthlyCapacity = static_cast<uint64_t>(park.capacity) * 30;
        return static_cast<uint32_t>(std::min<uint64_t>(visitors, monthlyCapacity));
    }

    inline bool pay(CompanyId owner, currency32_t amount, ExpenditureType type, const World::Pos2& position)
    {
        if (amount <= 0)
        {
            return true;
        }
        if (!CompanyManager::ensureCompanyFunding(owner, amount))
        {
            _lastStatus = "Not enough company funds for this park investment.";
            return false;
        }

        CompanyManager::applyPaymentToCompany(owner, amount, type);
        CompanyManager::spendMoneyEffect(World::Pos3{ position.x, position.y, 24 }, owner, amount);
        return true;
    }

    inline Park* createPark(const World::Pos2& position)
    {
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
        _lastStatus = "Park created. Enter it and build attractions before the first visitors arrive.";
        return &_parks.back();
    }

    inline currency32_t attractionBuildCost(AttractionType type)
    {
        switch (type)
        {
            case AttractionType::carousel:
                return 8000;
            case AttractionType::ferrisWheel:
                return 15000;
            case AttractionType::compactCoaster:
                return 35000;
            case AttractionType::foodStall:
                return 5000;
        }
        return 0;
    }

    inline const char* attractionName(AttractionType type)
    {
        switch (type)
        {
            case AttractionType::carousel:
                return "Carousel";
            case AttractionType::ferrisWheel:
                return "Ferris Wheel";
            case AttractionType::compactCoaster:
                return "Compact Coaster";
            case AttractionType::foodStall:
                return "Food Stall";
        }
        return "Attraction";
    }

    inline bool buildAttraction(Park& park, AttractionType type)
    {
        const auto cost = attractionBuildCost(type);
        if (!pay(park.owner, cost, ExpenditureType::Construction, park.position))
        {
            return false;
        }

        switch (type)
        {
            case AttractionType::carousel:
                park.carouselCount++;
                park.capacity += 180;
                park.popularity = std::min<uint16_t>(1000, park.popularity + 35);
                break;
            case AttractionType::ferrisWheel:
                park.ferrisWheelCount++;
                park.capacity += 260;
                park.popularity = std::min<uint16_t>(1000, park.popularity + 50);
                break;
            case AttractionType::compactCoaster:
                park.coasterCount++;
                park.capacity += 520;
                park.popularity = std::min<uint16_t>(1000, park.popularity + 85);
                break;
            case AttractionType::foodStall:
                park.foodStallCount++;
                park.capacity += 40;
                park.popularity = std::min<uint16_t>(1000, park.popularity + 15);
                break;
        }

        _lastStatus = std::string(attractionName(type)) + " built successfully.";
        return true;
    }

    inline void adjustTicketPrice(Park& park, int delta)
    {
        const int32_t next = std::clamp<int32_t>(park.ticketPrice + delta, 0, 100);
        park.ticketPrice = next;
        _lastStatus = "Park ticket price updated.";
    }

    inline void updateMonthly()
    {
        if (SceneManager::isEditorMode() || SceneManager::isTitleMode() || SceneManager::isNetworked())
        {
            return;
        }

        for (auto& park : _parks)
        {
            if (!park.open)
            {
                continue;
            }

            refreshClosestTown(park);
            const auto visitors = estimateMonthlyVisitors(park);
            park.visitorsLastMonth = visitors;
            park.lifetimeVisitors += visitors;

            const auto internalSpend = static_cast<currency32_t>(visitors * (2 + park.foodStallCount * 2));
            park.lastRevenue = static_cast<currency32_t>(visitors) * park.ticketPrice + internalSpend;
            park.lastTax = static_cast<currency32_t>(900 + park.capacity / 8 + attractionCount(park) * 250 + park.foodStallCount * 100);
            park.lastOperatingCost = static_cast<currency32_t>(1800 + attractionCount(park) * 900 + park.foodStallCount * 400 + visitors);
            park.lastProfit = park.lastRevenue - park.lastTax - park.lastOperatingCost;

            // Preserve the vanilla SV5 expenditure layout: park construction is
            // recorded as Construction, while the monthly net operating result
            // is booked under Miscellaneous. The Hybrid finance page keeps the
            // detailed tax / maintenance / revenue breakdown.
            CompanyManager::applyPaymentToCompany(park.owner, -park.lastProfit, ExpenditureType::Miscellaneous);

            if (auto* town = park.closestTownId == 0xFFFF ? nullptr : TownManager::get(TownId(park.closestTownId)); town != nullptr)
            {
                if (park.popularity >= 750)
                {
                    town->adjustCompanyRating(park.owner, 2);
                }
                else if (park.popularity <= 350)
                {
                    town->adjustCompanyRating(park.owner, -2);
                }
            }

            if (park.lastProfit > 0)
            {
                park.popularity = std::min<uint16_t>(1000, park.popularity + 4);
            }
            else if (park.popularity > 100)
            {
                park.popularity = std::max<uint16_t>(100, park.popularity - 6);
            }
        }

        _lastStatus = "Monthly park finances processed and posted to the company.";
    }
}
