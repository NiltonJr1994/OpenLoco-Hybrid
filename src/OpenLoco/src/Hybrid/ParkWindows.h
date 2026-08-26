#pragma once

#include "Hybrid/ParkManager.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Map/MapSelection.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace OpenLoco::Hybrid::ParkWindows
{
    using namespace OpenLoco::Ui;

    static constexpr auto kParkListWindowType = static_cast<Ui::WindowType>(62);
    static constexpr auto kParkInteriorWindowType = static_cast<Ui::WindowType>(63);

    static constexpr StringId kStringMenuParks = 2468;
    static constexpr StringId kStringTitleParks = 2469;
    static constexpr StringId kStringTitleInterior = 2470;
    inline constexpr char kMenuParksText[] = "Parks (Hybrid)";
    inline constexpr char kTitleParksText[] = "Hybrid Parks - Regional Management";
    inline constexpr char kTitleInteriorText[] = "Park Interior - Hybrid Prototype";

    inline void installStrings()
    {
        StringManager::swapString(kStringMenuParks, kMenuParksText);
        StringManager::swapString(kStringTitleParks, kTitleParksText);
        StringManager::swapString(kStringTitleInterior, kTitleInteriorText);
    }

    namespace RegionalWidx
    {
        constexpr WidgetId close{ "park_close" };
        constexpr WidgetId panel{ "park_panel" };
        constexpr WidgetId buildPark{ "park_build" };
        constexpr WidgetId enterPark{ "park_enter" };
        constexpr WidgetId previous{ "park_prev" };
        constexpr WidgetId next{ "park_next" };
    }

    enum RegionalWidgetIndex
    {
        regionalFrame,
        regionalCaption,
        regionalClose,
        regionalPanel,
        regionalBuildPark,
        regionalEnterPark,
        regionalPrevious,
        regionalNext,
    };

    static constexpr Ui::Size kRegionalSize = { 470, 265 };

    static constexpr auto kRegionalWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kRegionalSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kRegionalSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, kStringTitleParks),
        Widgets::ImageButton(RegionalWidx::close, { kRegionalSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(RegionalWidx::panel, { 0, 15 }, { kRegionalSize.width, kRegionalSize.height - 15 }, WindowColour::secondary),
        Widgets::Button(RegionalWidx::buildPark, { 10, 228 }, { 120, 22 }, WindowColour::secondary),
        Widgets::Button(RegionalWidx::enterPark, { 140, 228 }, { 120, 22 }, WindowColour::secondary),
        Widgets::Button(RegionalWidx::previous, { 300, 228 }, { 70, 22 }, WindowColour::secondary),
        Widgets::Button(RegionalWidx::next, { 380, 228 }, { 70, 22 }, WindowColour::secondary));

    namespace InteriorWidx
    {
        constexpr WidgetId close{ "interior_close" };
        constexpr WidgetId panel{ "interior_panel" };
        constexpr WidgetId overview{ "interior_overview" };
        constexpr WidgetId rides{ "interior_rides" };
        constexpr WidgetId finance{ "interior_finance" };
        constexpr WidgetId back{ "interior_back" };
        constexpr WidgetId carousel{ "ride_carousel" };
        constexpr WidgetId ferrisWheel{ "ride_ferris" };
        constexpr WidgetId coaster{ "ride_coaster" };
        constexpr WidgetId foodStall{ "ride_food" };
        constexpr WidgetId ticketMinus{ "ticket_minus" };
        constexpr WidgetId ticketPlus{ "ticket_plus" };
    }

    enum class InteriorTab : uint8_t
    {
        overview,
        rides,
        finance,
    };

    inline InteriorTab _interiorTab{ InteriorTab::overview };

    static constexpr Ui::Size kInteriorSize = { 560, 350 };

    static constexpr auto kInteriorWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kInteriorSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kInteriorSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, kStringTitleInterior),
        Widgets::ImageButton(InteriorWidx::close, { kInteriorSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(InteriorWidx::panel, { 0, 15 }, { kInteriorSize.width, kInteriorSize.height - 15 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::overview, { 10, 22 }, { 90, 20 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::rides, { 105, 22 }, { 90, 20 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::finance, { 200, 22 }, { 90, 20 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::back, { 445, 22 }, { 100, 20 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::carousel, { 18, 274 }, { 120, 24 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::ferrisWheel, { 148, 274 }, { 120, 24 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::coaster, { 278, 274 }, { 120, 24 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::foodStall, { 408, 274 }, { 120, 24 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::ticketMinus, { 160, 274 }, { 90, 24 }, WindowColour::secondary),
        Widgets::Button(InteriorWidx::ticketPlus, { 260, 274 }, { 90, 24 }, WindowColour::secondary));

    inline Widget* findWidget(Window& window, const WidgetId id)
    {
        for (auto& widget : window.widgets)
        {
            if (widget.id == id)
            {
                return &widget;
            }
        }
        return nullptr;
    }

    inline void clearMapSelection()
    {
        World::mapInvalidateSelectionRect();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
        World::mapInvalidateSelectionRect();
    }

    inline void selectParkAnchor(const World::Pos2& position)
    {
        clearMapSelection();
        World::setMapSelectionFlags(World::MapSelectionFlags::enable);
        World::setMapSelectionCorner(MapSelectionType::full);
        World::setMapSelectionArea(position, position);
        World::mapInvalidateSelectionRect();
    }

    inline void beginPlacement(Ui::Window& self)
    {
        clearMapSelection();
        ToolManager::toolSet(self, regionalPanel, CursorId::placeTown);
        Input::setFlag(Input::Flags::flag6);
        Ui::Windows::Main::showGridlines();
        Parks::_lastStatus = "Placement mode: click a clear land tile on the regional map.";
        self.invalidate();
    }

    inline void drawText(Gfx::TextRenderer& tr, const Ui::Window& self, int16_t x, int16_t y, const std::string& text)
    {
        tr.drawString({ self.x + x, self.y + y }, Colour::black, text.c_str());
    }

    inline void drawButtonText(Gfx::TextRenderer& tr, const Ui::Window& self, const WidgetId id, const std::string& text)
    {
        for (const auto& widget : self.widgets)
        {
            if (widget.id == id && !widget.hidden)
            {
                const auto centreX = self.x + widget.left + (widget.width() / 2);
                const auto y = self.y + widget.top + 5;
                StringManager::setString(StringIds::buffer_1250, text);
                tr.drawStringCentred({ static_cast<int16_t>(centreX), static_cast<int16_t>(y) }, Colour::black, StringIds::buffer_1250);
                return;
            }
        }
    }

    inline void selectPreviousPark()
    {
        if (Parks::_parks.empty())
        {
            return;
        }
        auto it = std::find_if(Parks::_parks.begin(), Parks::_parks.end(), [](const Parks::Park& p) { return p.id == Parks::_selectedParkId; });
        if (it == Parks::_parks.end() || it == Parks::_parks.begin())
        {
            Parks::_selectedParkId = Parks::_parks.back().id;
        }
        else
        {
            Parks::_selectedParkId = std::prev(it)->id;
        }
    }

    inline void selectNextPark()
    {
        if (Parks::_parks.empty())
        {
            return;
        }
        auto it = std::find_if(Parks::_parks.begin(), Parks::_parks.end(), [](const Parks::Park& p) { return p.id == Parks::_selectedParkId; });
        if (it == Parks::_parks.end() || std::next(it) == Parks::_parks.end())
        {
            Parks::_selectedParkId = Parks::_parks.front().id;
        }
        else
        {
            Parks::_selectedParkId = std::next(it)->id;
        }
    }

    inline Ui::Window* openInterior(uint16_t parkId);

    inline void prepareRegional(Ui::Window& self)
    {
        const bool hasPark = Parks::selectedPark() != nullptr;
        if (auto* w = findWidget(self, RegionalWidx::enterPark); w != nullptr)
            w->hidden = !hasPark;
        if (auto* w = findWidget(self, RegionalWidx::previous); w != nullptr)
            w->hidden = Parks::_parks.size() < 2;
        if (auto* w = findWidget(self, RegionalWidx::next); w != nullptr)
            w->hidden = Parks::_parks.size() < 2;
    }

    inline void onRegionalClose(Ui::Window& self)
    {
        if (ToolManager::isToolActive(self.type, self.number))
        {
            ToolManager::toolCancel();
        }
        clearMapSelection();
        Ui::Windows::Main::hideGridlines();
    }

    inline void onRegionalMouseUp(Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (id == RegionalWidx::close)
        {
            WindowManager::close(&self);
            return;
        }
        if (id == RegionalWidx::buildPark)
        {
            beginPlacement(self);
            return;
        }
        if (id == RegionalWidx::enterPark)
        {
            if (auto* park = Parks::selectedPark(); park != nullptr)
            {
                openInterior(park->id);
            }
            return;
        }
        if (id == RegionalWidx::previous)
        {
            selectPreviousPark();
        }
        else if (id == RegionalWidx::next)
        {
            selectNextPark();
        }

        if (auto* park = Parks::selectedPark(); park != nullptr)
        {
            selectParkAnchor(park->position);
        }
        self.invalidate();
    }

    inline void onRegionalToolAbort([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        clearMapSelection();
        Ui::Windows::Main::hideGridlines();
    }

    inline void onRegionalToolUpdate([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
    {
        clearMapSelection();
        const auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!mapPos)
        {
            return;
        }

        World::setMapSelectionFlags(World::MapSelectionFlags::enable);
        World::setMapSelectionCorner(MapSelectionType::full);
        World::setMapSelectionArea(*mapPos, *mapPos);
        World::mapInvalidateSelectionRect();
    }

    inline void onRegionalToolDown(Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
    {
        const auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!mapPos)
        {
            return;
        }

        auto* park = Parks::createPark(*mapPos);
        if (park == nullptr)
        {
            self.invalidate();
            return;
        }

        if (ToolManager::isToolActive(self.type, self.number))
        {
            ToolManager::toolCancel();
        }
        Ui::Windows::Main::hideGridlines();
        selectParkAnchor(park->position);
        self.invalidate();
    }

    inline void drawRegional(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawText(tr, self, 12, 26, "OpenLoco Hybrid v0.3.1-alpha - regional park integration");
        drawText(tr, self, 12, 43, "Park construction cost: 75,000 (booked as Construction)");
        drawText(tr, self, 12, 60, "Parks owned in this session: " + std::to_string(Parks::_parks.size()));

        if (auto* park = Parks::selectedPark(); park != nullptr)
        {
            drawText(tr, self, 12, 86, "Adventure Park #" + std::to_string(park->id) + (park->open ? " - OPEN" : " - CLOSED"));
            drawText(tr, self, 12, 103, "Map anchor: X " + std::to_string(park->position.x) + "  Y " + std::to_string(park->position.y));
            drawText(tr, self, 12, 120, "Closest town: #" + std::to_string(park->closestTownId) + "  Population: " + std::to_string(Parks::closestTownPopulation(*park)));
            drawText(tr, self, 12, 137, "Popularity: " + std::to_string(park->popularity) + "/1000   Capacity/day: " + std::to_string(park->capacity));
            drawText(tr, self, 12, 154, "Attractions: " + std::to_string(Parks::attractionCount(*park)) + "   Food stalls: " + std::to_string(park->foodStallCount));
            drawText(tr, self, 12, 171, "Projected visitors/month: " + std::to_string(Parks::estimateMonthlyVisitors(*park)));
            drawText(tr, self, 12, 188, "Last month visitors: " + std::to_string(park->visitorsLastMonth) + "   Result: " + std::to_string(park->lastProfit));
        }
        else
        {
            drawText(tr, self, 12, 88, "No parks exist yet.");
            drawText(tr, self, 12, 107, "Click BUILD PARK, then click a tile on the regional map.");
            drawText(tr, self, 12, 126, "The company will pay for the land/construction immediately.");
        }

        drawText(tr, self, 12, 211, "Status: " + Parks::_lastStatus);
        drawButtonText(tr, self, RegionalWidx::buildPark, "BUILD PARK");
        drawButtonText(tr, self, RegionalWidx::enterPark, "ENTER PARK");
        drawButtonText(tr, self, RegionalWidx::previous, "PREV");
        drawButtonText(tr, self, RegionalWidx::next, "NEXT");
    }

    inline constexpr WindowEventList kRegionalEvents = {
        .onClose = onRegionalClose,
        .onMouseUp = onRegionalMouseUp,
        .onToolUpdate = onRegionalToolUpdate,
        .onToolDown = onRegionalToolDown,
        .onToolAbort = onRegionalToolAbort,
        .prepareDraw = prepareRegional,
        .draw = drawRegional,
    };

    inline void prepareInterior(Ui::Window& self)
    {
        const bool rides = _interiorTab == InteriorTab::rides;
        const bool finance = _interiorTab == InteriorTab::finance;
        for (auto id : { InteriorWidx::carousel, InteriorWidx::ferrisWheel, InteriorWidx::coaster, InteriorWidx::foodStall })
        {
            if (auto* w = findWidget(self, id); w != nullptr)
                w->hidden = !rides;
        }
        for (auto id : { InteriorWidx::ticketMinus, InteriorWidx::ticketPlus })
        {
            if (auto* w = findWidget(self, id); w != nullptr)
                w->hidden = !finance;
        }
    }

    inline Parks::Park* interiorPark(Ui::Window& self)
    {
        return Parks::getPark(self.number);
    }

    inline void onInteriorMouseUp(Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (id == InteriorWidx::close || id == InteriorWidx::back)
        {
            WindowManager::close(&self);
            WindowManager::bringToFront(kParkListWindowType, 0);
            return;
        }

        if (id == InteriorWidx::overview)
            _interiorTab = InteriorTab::overview;
        else if (id == InteriorWidx::rides)
            _interiorTab = InteriorTab::rides;
        else if (id == InteriorWidx::finance)
            _interiorTab = InteriorTab::finance;

        auto* park = interiorPark(self);
        if (park == nullptr)
        {
            self.invalidate();
            return;
        }

        if (id == InteriorWidx::carousel)
            Parks::buildAttraction(*park, Parks::AttractionType::carousel);
        else if (id == InteriorWidx::ferrisWheel)
            Parks::buildAttraction(*park, Parks::AttractionType::ferrisWheel);
        else if (id == InteriorWidx::coaster)
            Parks::buildAttraction(*park, Parks::AttractionType::compactCoaster);
        else if (id == InteriorWidx::foodStall)
            Parks::buildAttraction(*park, Parks::AttractionType::foodStall);
        else if (id == InteriorWidx::ticketMinus)
            Parks::adjustTicketPrice(*park, -5);
        else if (id == InteriorWidx::ticketPlus)
            Parks::adjustTicketPrice(*park, 5);

        self.invalidate();
        WindowManager::invalidate(kParkListWindowType);
    }

    inline void drawInterior(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        auto tr = Gfx::TextRenderer(drawingCtx);
        auto* park = interiorPark(self);

        drawButtonText(tr, self, InteriorWidx::overview, "OVERVIEW");
        drawButtonText(tr, self, InteriorWidx::rides, "RIDES / SHOPS");
        drawButtonText(tr, self, InteriorWidx::finance, "FINANCE");
        drawButtonText(tr, self, InteriorWidx::back, "REGIONAL MAP");

        if (park == nullptr)
        {
            drawText(tr, self, 18, 62, "Park data is no longer available.");
            return;
        }

        drawText(tr, self, 18, 52, "Adventure Park #" + std::to_string(park->id));

        if (_interiorTab == InteriorTab::overview)
        {
            drawText(tr, self, 18, 78, "This is the first in-game park interior/management layer.");
            drawText(tr, self, 18, 98, "Popularity: " + std::to_string(park->popularity) + "/1000");
            drawText(tr, self, 18, 118, "Capacity/day: " + std::to_string(park->capacity));
            drawText(tr, self, 18, 138, "Attractions: " + std::to_string(Parks::attractionCount(*park)) + "   Food stalls: " + std::to_string(park->foodStallCount));
            drawText(tr, self, 18, 158, "Projected visitors/month: " + std::to_string(Parks::estimateMonthlyVisitors(*park)));
            drawText(tr, self, 18, 178, "Last month visitors: " + std::to_string(park->visitorsLastMonth));
            drawText(tr, self, 18, 206, "Use RIDES / SHOPS to add attractions, then FINANCE to adjust admission.");
            drawText(tr, self, 18, 226, "The future RCT2 detailed map will replace this prototype interior layer.");
        }
        else if (_interiorTab == InteriorTab::rides)
        {
            drawText(tr, self, 18, 78, "BUILD ATTRACTIONS - charged to the company as Construction");
            drawText(tr, self, 18, 103, "Carousel: " + std::to_string(park->carouselCount) + "   Cost 8,000   Popularity +35   Capacity +180/day");
            drawText(tr, self, 18, 123, "Ferris Wheel: " + std::to_string(park->ferrisWheelCount) + "   Cost 15,000   Popularity +50   Capacity +260/day");
            drawText(tr, self, 18, 143, "Compact Coaster: " + std::to_string(park->coasterCount) + "   Cost 35,000   Popularity +85   Capacity +520/day");
            drawText(tr, self, 18, 163, "Food Stall: " + std::to_string(park->foodStallCount) + "   Cost 5,000   Raises visitor spending");
            drawText(tr, self, 18, 195, "These are management prototypes; ride placement/track construction comes with the RCT2 map layer.");
            drawText(tr, self, 18, 218, "Status: " + Parks::_lastStatus);

            drawButtonText(tr, self, InteriorWidx::carousel, "BUILD CAROUSEL");
            drawButtonText(tr, self, InteriorWidx::ferrisWheel, "BUILD FERRIS");
            drawButtonText(tr, self, InteriorWidx::coaster, "BUILD COASTER");
            drawButtonText(tr, self, InteriorWidx::foodStall, "BUILD FOOD");
        }
        else
        {
            drawText(tr, self, 18, 78, "PARK FINANCE - monthly result is posted to company Miscellaneous");
            drawText(tr, self, 18, 103, "Admission price: " + std::to_string(park->ticketPrice));
            drawText(tr, self, 18, 123, "Projected visitors/month: " + std::to_string(Parks::estimateMonthlyVisitors(*park)));
            drawText(tr, self, 18, 150, "Last month revenue: " + std::to_string(park->lastRevenue));
            drawText(tr, self, 18, 170, "Park tax / land licence: " + std::to_string(park->lastTax));
            drawText(tr, self, 18, 190, "Maintenance + staff + guest service: " + std::to_string(park->lastOperatingCost));
            drawText(tr, self, 18, 210, "Last month net result: " + std::to_string(park->lastProfit));
            drawText(tr, self, 18, 238, "The tax grows with park capacity and attractions; rides also add maintenance.");

            drawButtonText(tr, self, InteriorWidx::ticketMinus, "TICKET -5");
            drawButtonText(tr, self, InteriorWidx::ticketPlus, "TICKET +5");
        }
    }

    inline constexpr WindowEventList kInteriorEvents = {
        .onMouseUp = onInteriorMouseUp,
        .prepareDraw = prepareInterior,
        .draw = drawInterior,
    };

    inline Ui::Window* openInterior(uint16_t parkId)
    {
        installStrings();
        _interiorTab = InteriorTab::overview;

        if (auto* existing = WindowManager::bringToFront(kParkInteriorWindowType, parkId); existing != nullptr)
        {
            existing->invalidate();
            return existing;
        }

        auto* window = WindowManager::createWindowCentred(kParkInteriorWindowType, kInteriorSize, WindowFlags::none, kInteriorEvents);
        window->number = parkId;
        window->setWidgets(kInteriorWidgets);
        window->initScrollWidgets();

        if (auto* skin = ObjectManager::get<InterfaceSkinObject>(); skin != nullptr)
        {
            window->setColour(WindowColour::primary, skin->windowTitlebarColour);
            window->setColour(WindowColour::secondary, skin->windowColour);
        }
        return window;
    }

    inline Ui::Window* open()
    {
        installStrings();

        auto* window = WindowManager::bringToFront(kParkListWindowType, 0);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(kParkListWindowType, kRegionalSize, WindowFlags::none, kRegionalEvents);
            window->number = 0;
            window->setWidgets(kRegionalWidgets);
            window->initScrollWidgets();

            if (auto* skin = ObjectManager::get<InterfaceSkinObject>(); skin != nullptr)
            {
                window->setColour(WindowColour::primary, skin->windowTitlebarColour);
                window->setColour(WindowColour::secondary, skin->windowColour);
            }
        }

        if (auto* park = Parks::selectedPark(); park != nullptr)
        {
            selectParkAnchor(park->position);
        }
        window->invalidate();
        return window;
    }
}
