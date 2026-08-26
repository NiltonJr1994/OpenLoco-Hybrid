#pragma once

#include "Graphics/DrawingContext.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Hybrid/ParkManager.h"
#include "Hybrid/Rct2AssetRegistry.h"
#include "Hybrid/Rct2Bridge.h"
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

    static constexpr StringId kStringMenuParks = 2468;
    static constexpr StringId kStringTitleParks = 2469;
    static constexpr StringId kStringBuildPark = 2470;
    static constexpr StringId kStringEnterPark = 2471;
    static constexpr StringId kStringRescanRct2 = 2472;
    static constexpr StringId kStringPreviousTemplate = 2473;
    static constexpr StringId kStringNextTemplate = 2474;

    inline constexpr char kMenuParksText[] = "Parks (Hybrid)";
    inline constexpr char kTitleParksText[] = "OpenLoco Hybrid - Regional Parks";
    inline constexpr char kBuildParkText[] = "Build park";
    inline constexpr char kEnterParkText[] = "Enter park";
    inline constexpr char kRescanRct2Text[] = "Rescan RCT2";
    inline constexpr char kPreviousTemplateText[] = "< Template";
    inline constexpr char kNextTemplateText[] = "Template >";

    inline void installStrings()
    {
        StringManager::swapString(kStringMenuParks, kMenuParksText);
        StringManager::swapString(kStringTitleParks, kTitleParksText);
        StringManager::swapString(kStringBuildPark, kBuildParkText);
        StringManager::swapString(kStringEnterPark, kEnterParkText);
        StringManager::swapString(kStringRescanRct2, kRescanRct2Text);
        StringManager::swapString(kStringPreviousTemplate, kPreviousTemplateText);
        StringManager::swapString(kStringNextTemplate, kNextTemplateText);
    }

    namespace Widx
    {
        constexpr WidgetId close{ "hybrid_park_close" };
        constexpr WidgetId panel{ "hybrid_park_panel" };
        constexpr WidgetId buildPark{ "hybrid_park_build" };
        constexpr WidgetId enterPark{ "hybrid_park_enter" };
        constexpr WidgetId rescanRct2{ "hybrid_rct2_rescan" };
        constexpr WidgetId previousTemplate{ "hybrid_template_prev" };
        constexpr WidgetId nextTemplate{ "hybrid_template_next" };
    }

    enum WidgetIndex
    {
        frame,
        caption,
        close,
        panel,
        buildPark,
        enterPark,
        rescanRct2,
        previousTemplate,
        nextTemplate,
    };

    static constexpr Ui::Size kWindowSize = { 570, 345 };

    static constexpr auto kWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, kStringTitleParks),
        Widgets::ImageButton(Widx::close, { kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(Widx::panel, { 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WindowColour::secondary),
        Widgets::Button(Widx::buildPark, { 12, 304 }, { 105, 24 }, WindowColour::secondary, kStringBuildPark),
        Widgets::Button(Widx::enterPark, { 125, 304 }, { 105, 24 }, WindowColour::secondary, kStringEnterPark),
        Widgets::Button(Widx::rescanRct2, { 238, 304 }, { 105, 24 }, WindowColour::secondary, kStringRescanRct2),
        Widgets::Button(Widx::previousTemplate, { 351, 304 }, { 100, 24 }, WindowColour::secondary, kStringPreviousTemplate),
        Widgets::Button(Widx::nextTemplate, { 459, 304 }, { 100, 24 }, WindowColour::secondary, kStringNextTemplate));

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

    inline void drawText(Gfx::TextRenderer& tr, int16_t x, int16_t y, const std::string& text)
    {
        tr.drawString({ x, y }, Colour::black, text.c_str());
    }

    inline void clearMapSelection()
    {
        World::mapInvalidateSelectionRect();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
        World::mapInvalidateSelectionRect();
    }

    inline void selectParkFootprint(const World::Pos2& position)
    {
        clearMapSelection();
        const auto [minPos, maxPos] = Parks::footprintBounds(position);
        World::setMapSelectionFlags(World::MapSelectionFlags::enable);
        World::setMapSelectionCorner(MapSelectionType::full);
        World::setMapSelectionArea(minPos, maxPos);
        World::mapInvalidateSelectionRect();
    }

    inline void beginPlacement(Ui::Window& self)
    {
        if (!Rct2Assets::ready())
        {
            Parks::_lastStatus = Rct2Assets::get().status;
            self.invalidate();
            return;
        }

        clearMapSelection();
        ToolManager::toolSet(self, panel, CursorId::placeTown);
        Input::setFlag(Input::Flags::flag6);
        Ui::Windows::Main::showGridlines();
        Parks::_lastStatus = "Placement mode: choose a clear 7x7 area within 48 tiles of a town centre.";
        self.invalidate();
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

    inline void prepareDraw(Ui::Window& self)
    {
        const bool assetsReady = Rct2Assets::ready();
        const bool hasPark = Parks::selectedPark() != nullptr;

        if (auto* w = findWidget(self, Widx::buildPark); w != nullptr)
        {
            w->disabled = !assetsReady;
        }
        if (auto* w = findWidget(self, Widx::enterPark); w != nullptr)
        {
            w->disabled = !hasPark || !assetsReady || !Rct2Bridge::runtimeAvailable();
        }
        if (auto* w = findWidget(self, Widx::previousTemplate); w != nullptr)
        {
            w->disabled = Rct2Assets::get().scenarios.size() < 2;
        }
        if (auto* w = findWidget(self, Widx::nextTemplate); w != nullptr)
        {
            w->disabled = Rct2Assets::get().scenarios.size() < 2;
        }
    }

    inline void onClose(Ui::Window& self)
    {
        if (ToolManager::isToolActive(self.type, self.number))
        {
            ToolManager::toolCancel();
        }
        clearMapSelection();
        Ui::Windows::Main::hideGridlines();
    }

    inline void onMouseUp(Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, const WidgetId id)
    {
        if (id == Widx::close)
        {
            WindowManager::close(&self);
            return;
        }
        if (id == Widx::buildPark)
        {
            beginPlacement(self);
            return;
        }
        if (id == Widx::rescanRct2)
        {
            Rct2Assets::scan();
            Parks::_lastStatus = Rct2Assets::get().status;
            self.invalidate();
            return;
        }
        if (id == Widx::previousTemplate)
        {
            Rct2Assets::selectPreviousScenario();
            Parks::_lastStatus = "Previous RCT2 scenario template selected.";
            self.invalidate();
            return;
        }
        if (id == Widx::nextTemplate)
        {
            Rct2Assets::selectNextScenario();
            Parks::_lastStatus = "Next RCT2 scenario template selected.";
            self.invalidate();
            return;
        }
        if (id == Widx::enterPark)
        {
            if (auto* park = Parks::selectedPark(); park != nullptr)
            {
                if (Rct2Bridge::launchDetailedPark(park->id))
                {
                    park->detailedParkLaunched = true;
                    Parks::_lastStatus = Rct2Bridge::_lastStatus;
                }
                else
                {
                    Parks::_lastStatus = Rct2Bridge::_lastStatus;
                }
                self.invalidate();
            }
        }
    }

    inline void onToolAbort([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        clearMapSelection();
        Ui::Windows::Main::hideGridlines();
    }

    inline void onToolUpdate([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
    {
        const auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!mapPos)
        {
            clearMapSelection();
            return;
        }
        selectParkFootprint(*mapPos);
    }

    inline void onToolDown(Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
    {
        const auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!mapPos)
        {
            Parks::_lastStatus = "No valid land tile is under the cursor.";
            self.invalidate();
            return;
        }

        auto* park = Parks::createPark(*mapPos);
        if (park == nullptr)
        {
            // Keep placement mode active so the player can immediately try a
            // different site after reading the visible rejection reason.
            self.invalidate();
            return;
        }

        if (ToolManager::isToolActive(self.type, self.number))
        {
            ToolManager::toolCancel();
        }
        Ui::Windows::Main::hideGridlines();
        selectParkFootprint(park->position);
        self.invalidate();
    }

    inline void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        auto tr = Gfx::TextRenderer(drawingCtx);
        const auto& assets = Rct2Assets::get();

        drawText(tr, 12, 27, "OpenLoco Hybrid v0.4.0-alpha - RCT2 bridge diagnostics");
        drawText(tr, 12, 47, std::string("RCT2 registry: ") + (assets.ready ? "READY" : "NOT READY"));
        drawText(tr, 12, 64, "Data files: " + std::to_string(assets.dataFiles) + "    ObjData: " + std::to_string(assets.objectFiles));
        drawText(tr, 12, 81, "Rides/shops: " + std::to_string(assets.objectTypes[0]) + "    Small scenery: " + std::to_string(assets.objectTypes[1]) + "    Large scenery: " + std::to_string(assets.objectTypes[2]));
        drawText(tr, 12, 98, "Walls: " + std::to_string(assets.objectTypes[3]) + "    Park entrances: " + std::to_string(assets.objectTypes[8]));
        drawText(tr, 12, 115, "Track designs (.TD6): " + std::to_string(assets.trackDesignFiles) + "    Scenarios (.SC6): " + std::to_string(assets.scenarioFiles) + "    Saved parks: " + std::to_string(assets.savedParkFiles));
        drawText(tr, 12, 132, std::string("OpenRCT2 runtime: ") + (Rct2Bridge::runtimeAvailable() ? "READY" : "MISSING"));

        if (const auto* scenario = Rct2Assets::selectedScenario(); scenario != nullptr)
        {
            drawText(tr, 12, 155, "Detailed-park template: " + scenario->filename().string());
        }
        else
        {
            drawText(tr, 12, 155, "Detailed-park template: none");
        }

        if (auto* park = Parks::selectedPark(); park != nullptr)
        {
            drawText(tr, 12, 183, "Adventure Park #" + std::to_string(park->id) + "    Regional footprint: 7x7 tiles");
            drawText(tr, 12, 200, "Centre X " + std::to_string(park->position.x) + "  Y " + std::to_string(park->position.y) + "    Closest town population: " + std::to_string(Parks::closestTownPopulation(*park)));
            drawText(tr, 12, 217, std::string("Detailed park layer: ") + (park->detailedParkLaunched ? "launched" : "not launched yet"));
        }
        else
        {
            drawText(tr, 12, 183, "No regional park exists yet. BUILD PARK reserves a clear 7x7 site near a town.");
            drawText(tr, 12, 200, "Hybrid alpha construction charge: 5,000. Obstacles are never silently bulldozed.");
        }

        drawText(tr, 12, 242, "Status: " + Parks::_lastStatus);
        drawText(tr, 12, 261, "ENTER PARK uses the real OpenRCT2 engine and your RCT2 ObjData/Tracks/Scenarios.");
        drawText(tr, 12, 278, "TD6 designs remain in RCT2\\Tracks; OpenRCT2 indexes that directory through --rct2-data-path.");
    }

    inline constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onToolUpdate = onToolUpdate,
        .onToolDown = onToolDown,
        .onToolAbort = onToolAbort,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    inline Ui::Window* open()
    {
        installStrings();
        Rct2Assets::get();

        auto* window = WindowManager::bringToFront(kParkListWindowType, 0);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(kParkListWindowType, kWindowSize, WindowFlags::none, kEvents);
            window->number = 0;
            window->setWidgets(kWidgets);
            window->initScrollWidgets();

            if (auto* skin = ObjectManager::get<InterfaceSkinObject>(); skin != nullptr)
            {
                window->setColour(WindowColour::primary, skin->windowTitlebarColour);
                window->setColour(WindowColour::secondary, skin->windowColour);
            }
        }

        if (auto* park = Parks::selectedPark(); park != nullptr)
        {
            selectParkFootprint(park->position);
        }
        window->invalidate();
        return window;
    }
}
