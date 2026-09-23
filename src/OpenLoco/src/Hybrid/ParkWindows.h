#pragma once

#include "Graphics/DrawingContext.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Hybrid/ParkManager.h"
#include "Hybrid/Rct2AssetRegistry.h"
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

    static constexpr auto kParkListWindowType = Ui::WindowType::hybridParks;
    inline bool _insidePark = false;

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
    inline constexpr char kPreviousTemplateText[] = "< Object";
    inline constexpr char kNextTemplateText[] = "Object >";

    static constexpr StringId kStringInstantiate = 2475;
    static constexpr StringId kStringPreviousPark = 2476;
    inline void installStrings()
    {
        StringManager::swapString(kStringInstantiate, "Add object");
        StringManager::swapString(kStringPreviousPark, "Previous park");
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
        constexpr WidgetId instantiate{ "hybrid_instantiate" };
        constexpr WidgetId previousPark{ "hybrid_previous_park" };
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

    static constexpr Ui::Size kWindowSize = { 570, 390 };

    static constexpr auto kWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, kStringTitleParks),
        Widgets::ImageButton(Widx::close, { kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(Widx::panel, { 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WindowColour::secondary),
        Widgets::Button(Widx::buildPark, { 12, 304 }, { 105, 24 }, WindowColour::secondary, kStringBuildPark),
        Widgets::Button(Widx::enterPark, { 125, 304 }, { 105, 24 }, WindowColour::secondary, kStringEnterPark),
        Widgets::Button(Widx::rescanRct2, { 238, 304 }, { 105, 24 }, WindowColour::secondary, kStringRescanRct2),
        Widgets::Button(Widx::previousTemplate, { 351, 304 }, { 100, 24 }, WindowColour::secondary, kStringPreviousTemplate),
        Widgets::Button(Widx::nextTemplate, { 459, 304 }, { 100, 24 }, WindowColour::secondary, kStringNextTemplate),
        Widgets::Button(Widx::instantiate, { 12, 338 }, { 160, 24 }, WindowColour::secondary, kStringInstantiate),
        Widgets::Button(Widx::previousPark, { 184, 338 }, { 160, 24 }, WindowColour::secondary, kStringPreviousPark));

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
        const auto clipped = text.substr(0, 84);
        tr.drawString({ x, y }, Colour::black, clipped.c_str());
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
        if (Parks::selectedPark() == nullptr)
        {
            _insidePark = false;
        }
        const bool assetsReady = Parks::hasRct2Assets();
        if (auto* w = findWidget(self, Widx::instantiate))
        {
            w->disabled = !_insidePark || !Rct2Assets::selectedRide() || !Parks::selectedPark() || Parks::selectedPark()->rides.size() >= 9;
        }
        if (auto* w = findWidget(self, Widx::previousPark))
        {
            w->disabled = Parks::_parks.size() < 2;
        }
        if (auto* w = findWidget(self, Widx::rescanRct2))
        {
            w->disabled = !Parks::_parks.empty();
        }
        const bool hasPark = Parks::selectedPark() != nullptr;

        if (auto* w = findWidget(self, Widx::buildPark); w != nullptr)
        {
            w->disabled = !assetsReady;
        }
        if (auto* w = findWidget(self, Widx::enterPark); w != nullptr)
        {
            w->disabled = !hasPark;
        }
        if (auto* w = findWidget(self, Widx::previousTemplate); w != nullptr)
        {
            w->disabled = !_insidePark || Rct2Assets::get().rides.size() < 2;
        }
        if (auto* w = findWidget(self, Widx::nextTemplate); w != nullptr)
        {
            w->disabled = !_insidePark || Rct2Assets::get().rides.size() < 2;
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
        if (id == Widx::instantiate)
        {
            if (_insidePark)
            {
                Parks::instantiateSelectedRide();
            }
            self.invalidate();
            return;
        }
        if (id == Widx::previousPark)
        {
            selectPreviousPark();
            if (auto* p = Parks::selectedPark())
            {
                selectParkFootprint(p->position);
            }
            self.invalidate();
            return;
        }
        if (id == Widx::buildPark)
        {
            beginPlacement(self);
            return;
        }
        if (id == Widx::rescanRct2)
        {
            if (!Parks::_parks.empty())
            {
                return;
            }
            Rct2Graphics::reset();
            Rct2Assets::scan();
            Parks::_lastStatus = Rct2Assets::get().status;
            self.invalidate();
            return;
        }
        if (id == Widx::previousTemplate)
        {
            Rct2Assets::selectRide(-1);
            Parks::_lastStatus = "Previous native RCT2 object selected.";
            self.invalidate();
            return;
        }
        if (id == Widx::nextTemplate)
        {
            Rct2Assets::selectRide(1);
            Parks::_lastStatus = "Next native RCT2 object selected.";
            self.invalidate();
            return;
        }
        if (id == Widx::enterPark)
        {
            if (auto* park = Parks::selectedPark(); park != nullptr)
            {
                _insidePark = true;
                Parks::_lastStatus = "Browse RCT2 definitions, then Add object to this park.";
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

        drawText(tr, 12, 27, "OpenLoco Hybrid v0.5.0-alpha - Native RCT2 assets");
        drawText(tr, 12, 47, std::string("Decoded registry: ") + (assets.ready ? "READY" : "NOT READY"));
        drawText(tr, 12, 64, "Rides/shops: " + std::to_string(assets.rides.size()) + "    Entrances: " + std::to_string(assets.entrances.size()));
        drawText(tr, 12, 81, "Unsupported classes: " + std::to_string(assets.unsupported) + "    Rejected files: " + std::to_string(assets.rejected));
        if (auto* park = Parks::selectedPark())
        {
            drawText(tr, 12, 103, "Park #" + std::to_string(park->id) + "    7x7 tiles    Objects: " + std::to_string(park->rides.size()) + "/9");
            drawText(tr, 12, 120, "Entrance: " + park->entrance->name + " [" + park->entrance->id + "]");
            if (_insidePark)
            {
                if (auto ride = Rct2Assets::selectedRide())
                {
                    drawText(tr, 12, 146, "Object " + std::to_string(Rct2Assets::_selectedRide + 1) + "/" + std::to_string(assets.rides.size()) + ": " + ride->name.substr(0, 46));
                    drawText(tr, 12, 163, "DAT: " + ride->id + "    RCT2 ride type: " + std::to_string(ride->rideTypes[0]));
                    drawText(tr, 12, 180, ride->description.substr(0, 65));
                    drawText(tr, 12, 197, "Capacity definition: " + ride->capacity.substr(0, 42));
                    drawText(tr, 12, 214, "Decoded definition bytes: " + std::to_string(ride->payload.size()));
                    try
                    {
                        const auto image = Rct2Graphics::load(ride);
                        drawingCtx.drawImage(ZoomLevel::full, 495, 190, ImageId(image));
                    }
                    catch (const std::exception& e)
                    {
                        Parks::_lastStatus = e.what();
                    }
                }
            }
            else
            {
                drawText(tr, 12, 146, "Enter park opens the native object browser here.");
            }
            if (!park->rides.empty())
            {
                drawText(tr, 12, 234, "Last instance: " + park->rides.back().definition->name);
            }
        }
        else
        {
            drawText(tr, 12, 110, "Build park: select a clear, flat 7x7 site within 48 tiles of a town.");
            drawText(tr, 12, 130, "Construction charge: 5,000. Native object instances are free in this alpha.");
        }
        drawText(tr, 12, 259, "Status: " + Parks::_lastStatus);
        drawText(tr, 12, 280, "Session-only: parks are not saved. TD6 and ride simulation are not implemented.");
        drawText(tr, 12, 372, "RCT2 ObjData remains isolated from all Locomotion objects and mods.");
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
        if (SceneManager::isNetworked() || SceneManager::isEditorMode())
        {
            return nullptr;
        }

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
