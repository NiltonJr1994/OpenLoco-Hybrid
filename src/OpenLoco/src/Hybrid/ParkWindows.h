#pragma once

#include "Audio/Audio.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/ImageIds.h"
#include "Graphics/TextRenderer.h"
#include "Hybrid/ParkManager.h"
#include "Hybrid/Rct2AssetRegistry.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
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
    static constexpr StringId kStringRotate = 2477;
    static constexpr StringId kStringModelPrevious = 2478;
    static constexpr StringId kStringModelNext = 2479;
    static constexpr StringId kStringPlacementError = 2480;
    inline void installStrings()
    {
        StringManager::swapString(kStringRotate, "Rotate gate");
        StringManager::swapString(kStringModelPrevious, "< Park model");
        StringManager::swapString(kStringModelNext, "Park model >");
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
        constexpr WidgetId rotate{ "hybrid_rotate" };
        constexpr WidgetId modelPrevious{ "hybrid_model_previous" };
        constexpr WidgetId modelNext{ "hybrid_model_next" };
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

    static constexpr Ui::Size kWindowSize = { 570, 418 };

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
        Widgets::Button(Widx::previousPark, { 184, 338 }, { 160, 24 }, WindowColour::secondary, kStringPreviousPark),
        Widgets::Button(Widx::rotate, { 351, 338 }, { 208, 24 }, WindowColour::secondary, kStringRotate),
        Widgets::Button(Widx::modelPrevious, { 12, 372 }, { 160, 24 }, WindowColour::secondary, kStringModelPrevious),
        Widgets::Button(Widx::modelNext, { 184, 372 }, { 160, 24 }, WindowColour::secondary, kStringModelNext));

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

    inline void drawText([[maybe_unused]] const Ui::Window& self, Gfx::TextRenderer& tr, int16_t x, int16_t y, const std::string& text)
    {
        // WindowManager already pushes a clip whose origin is the window.
        // FD uses the native black text palette, just like formatted UI strings.
        char clipped[512]{};
        const auto length = std::min(text.size(), sizeof(clipped) - 8);
        std::copy_n(text.data(), length, clipped);
        Gfx::TextRenderer::clipString(tr.getCurrentFont(), 544, clipped);
        tr.drawString({ x, y }, AdvancedColour::FD(), clipped);
    }

    inline std::string money(currency32_t value)
    {
        char buffer[128]{};
        FormatArguments args;
        args.push(currency48_t(value));
        StringManager::formatString(buffer, sizeof(buffer), StringIds::currency48, args);
        return buffer;
    }

    inline void clearMapSelection()
    {
        World::mapInvalidateSelectionRect();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable | World::MapSelectionFlags::enableConstructionArrow);
        World::mapInvalidateSelectionRect();
    }

    inline void selectParkFootprint(const World::Pos2& position)
    {
        clearMapSelection();
        const auto [minPos, maxPos] = Parks::footprintBounds(position);
        World::setMapSelectionFlags(World::MapSelectionFlags::enable);
        World::setMapSelectionCorner(MapSelectionType::full);
        World::setMapSelectionArea(minPos, maxPos);
        const auto gate = Parks::normaliseCentre(position) + Parks::rotate({ 96, 0 }, Parks::_rotation);
        if (World::validCoords(World::toTileSpace(gate)))
        {
            const auto* surface = World::TileManager::get(gate).surface();
            if (surface)
            {
                World::setConstructionArrow({ { gate.x, gate.y, surface->baseHeight() }, static_cast<uint8_t>((Parks::_rotation + 2) & 3) });
                World::setMapSelectionFlags(World::MapSelectionFlags::enableConstructionArrow);
            }
        }
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

        try
        {
            Parks::preparePreview();
        }
        catch (const std::exception& e)
        {
            Parks::_lastStatus = e.what();
            self.invalidate();
            return;
        }
        clearMapSelection();
        ToolManager::toolSet(self, panel, CursorId::placeTown);
        Input::setFlag(Input::Flags::flag6);
        Ui::Windows::Main::showGridlines();
        _insidePark = false;
        Parks::_lastStatus = "Choose dry land beside a road. Rotate gate to face the road.";
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
        Parks::clearPreview();
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
        if (id == Widx::rotate || id == Widx::modelPrevious || id == Widx::modelNext)
        {
            if (id == Widx::rotate)
            {
                Parks::_rotation = (Parks::_rotation + 1) & 3;
            }
            else
            {
                Parks::_model = (Parks::_model + (id == Widx::modelNext ? 1 : 2)) % Parks::kModels.size();
            }
            try
            {
                Parks::preparePreview();
                if (Parks::_hover)
                {
                    Parks::movePreview(*Parks::_hover);
                }
                else
                {
                    Parks::_lastStatus = "Model/orientation selected. Click Build park, then choose land by a road.";
                }
            }
            catch (const std::exception& e)
            {
                Parks::clearPreview();
                Parks::_lastStatus = e.what();
            }
            self.invalidate();
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
            if (ToolManager::isToolActive(self.type, self.number))
            {
                ToolManager::toolCancel();
            }
            Parks::clearPreview();
            Parks::_groundImage.reset();
            ParkVisuals::reset();
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
        Parks::clearPreview();
        clearMapSelection();
        Ui::Windows::Main::hideGridlines();
    }

    inline void onToolUpdate([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
    {
        const auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!mapPos)
        {
            Parks::clearPreview();
            clearMapSelection();
            return;
        }
        const auto centre = Parks::normaliseCentre(*mapPos);
        if (Parks::_hover && *Parks::_hover == centre)
        {
            return;
        }
        selectParkFootprint(centre);
        try
        {
            Parks::movePreview(centre);
        }
        catch (const std::exception& e)
        {
            Parks::clearPreview();
            Parks::_lastStatus = e.what();
        }
        self.invalidate();
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
            StringManager::swapString(kStringPlacementError, Parks::_lastStatus.c_str());
            Ui::Windows::Error::open(StringIds::error_cant_build_this_here, kStringPlacementError);
            self.invalidate();
            return;
        }

        if (ToolManager::isToolActive(self.type, self.number))
        {
            ToolManager::toolCancel();
        }
        Ui::Windows::Main::hideGridlines();
        clearMapSelection();
        Audio::playSound(Audio::SoundId::construct, Audio::ChannelId::effects, World::Pos3{ park->position.x, park->position.y, park->height });
        self.invalidate();
    }

    inline void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        auto tr = Gfx::TextRenderer(drawingCtx);
        const auto& assets = Rct2Assets::get();

        drawText(self, tr, 12, 27, "OpenLoco Hybrid v0.6.0-alpha - Native RCT2 assets");
        drawText(self, tr, 12, 45, std::string("RCT2: ") + (assets.ready ? "READY" : "NOT READY") + "  Rides/shops: " + std::to_string(assets.rides.size()) + "  Entrances: " + std::to_string(assets.entrances.size()));
        const auto* park = Parks::selectedPark();
        const auto model = _insidePark && park ? park->model : Parks::_model;
        const auto count = _insidePark && park ? park->rides.size() : 3;
        drawText(self, tr, 12, 64, std::string("Park model: ") + Parks::kModels[model].name + "  |  7x7 tiles  |  Gate side: " + std::to_string((_insidePark && park ? park->rotation : Parks::_rotation) + 1) + "/4");
        if (_insidePark && park)
        {
            drawText(self, tr, 12, 84, "Park #" + std::to_string(park->id) + "  Objects: " + std::to_string(count) + "/9  |  Paid: " + money(park->paidConstruction));
        }
        else
        {
            const auto& q = Parks::_quote;
            drawText(self, tr, 12, 84, "Construction: " + money(Parks::constructionCost(model)) + "  |  Trees: " + money(q.clearance) + "  |  Levelling: " + money(q.landscaping));
            drawText(self, tr, 12, 101, "Total: " + money(Parks::constructionCost(model) + q.clearance + q.landscaping) + "  |  Trees to remove: " + std::to_string(q.trees));
        }
        drawText(self, tr, 12, 121, "Monthly park tax: " + money(Parks::monthlyTax(count)) + "  |  Upkeep: " + money(Parks::monthlyUpkeep(count)));
        drawText(self, tr, 12, 138, "Monthly total: " + money(Parks::monthlyTax(count) + Parks::monthlyUpkeep(count)) + "  (inflation adjusted; company Miscellaneous expenses)");
        if (_insidePark && park)
        {
            drawText(self, tr, 12, 155, "Last monthly debit: " + money(park->lastTax + park->lastOperatingCost));
            if (auto ride = Rct2Assets::selectedRide())
            {
                drawText(self, tr, 12, 179, "Object " + std::to_string(Rct2Assets::_selectedRide + 1) + "/" + std::to_string(assets.rides.size()) + ": " + ride->name);
                drawText(self, tr, 12, 199, ride->description.substr(0, 63));
                drawText(self, tr, 12, 219, "Add object increases monthly costs. Animated models are visual proxies.");
                try
                {
                    drawingCtx.drawImage(ZoomLevel::full, { 505, 238 }, ImageId(ParkVisuals::thumbnail(ride)));
                }
                catch (const std::exception& e)
                {
                    Parks::_lastStatus = e.what();
                }
            }
        }
        else
        {
            drawText(self, tr, 12, 166, "Choose a model below. Rotate gate, then place beside a level road.");
            drawText(self, tr, 12, 184, "Water and steep hills are blocked. Trees and minor slopes are quoted.");
            drawText(self, tr, 12, 202, "The miniature appears only at a valid site. Left-click builds the park.");
            drawText(self, tr, 12, 220, "After building, Enter park opens its objects and cost details.");
        }
        const auto status = "Status: " + Parks::_lastStatus;
        const auto split = status.size() > 78 ? status.rfind(' ', 78) : std::string::npos;
        drawText(self, tr, 12, 250, status.substr(0, split));
        if (split != std::string::npos)
        {
            drawText(self, tr, 12, 264, status.substr(split + 1));
        }
        drawText(self, tr, 12, 282, "Session-only parks; no saved parks, visitor simulation or TD6 construction.");
        drawText(self, tr, 12, 402, "RCT2 assets stay separate from Locomotion ObjData.");
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
