#pragma once

#include "Graphics/Colour.h"
#include "Graphics/DrawingContext.h"
#include "Graphics/Gfx.h"
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
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"

#include <cstdint>
#include <string>
#include <vector>

namespace OpenLoco::Hybrid::ParkWindows
{
    using namespace Ui;

    // Hybrid window types deliberately live outside the original OpenLoco range.
    // They are kept local for the alpha so the SV5 ABI and vanilla WindowType
    // layout remain untouched.
    static constexpr auto kParkListWindowType = static_cast<Ui::WindowType>(62);

    // The current English language table ends at 2467. These IDs are still
    // within StringManager's pointer table, but do not have backing storage of
    // their own. installStrings() therefore points them at static Hybrid text
    // instead of trying to memcpy into a null string pointer.
    static constexpr StringId kStringMenuParks = 2468;
    static constexpr StringId kStringTitleParks = 2469;
    inline constexpr char kMenuParksText[] = "Parks (Hybrid)";
    inline constexpr char kTitleParksText[] = "Hybrid Parks";

    struct Park
    {
        uint16_t id{};
        World::Pos2 position{};
        uint16_t popularity{ 600 };
        uint16_t capacity{ 2500 };
        uint32_t visitorsThisMonth{};
        int32_t monthlyProfit{};
    };

    inline std::vector<Park> _parks{};
    inline uint16_t _nextParkId{ 1 };

    inline void installStrings()
    {
        StringManager::swapString(kStringMenuParks, kMenuParksText);
        StringManager::swapString(kStringTitleParks, kTitleParksText);
    }

    enum widx
    {
        frame,
        caption,
        closeButton,
        panel,
    };

    namespace Widx
    {
        constexpr WidgetId kCloseButton{ "close_button" };
        constexpr WidgetId kPanel{ "panel" };
    }

    static constexpr Ui::Size kWindowSize = { 390, 178 };

    static constexpr auto kWidgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, kStringTitleParks),
        Widgets::ImageButton(Widx::kCloseButton, { kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel(Widx::kPanel, { 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WindowColour::secondary));

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
        ToolManager::toolSet(self, widx::panel, CursorId::placeTown);
        Input::setFlag(Input::Flags::flag6);
        Ui::Windows::Main::showGridlines();
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
        if (id == Widx::kCloseButton)
        {
            WindowManager::close(&self);
        }
    }

    inline void onToolAbort([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        clearMapSelection();
        Ui::Windows::Main::hideGridlines();
    }

    inline void onToolUpdate([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
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

    inline void onToolDown(Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t x, int16_t y)
    {
        const auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
        if (!mapPos)
        {
            return;
        }

        Park park{};
        park.id = _nextParkId++;
        park.position = *mapPos;
        _parks.push_back(park);

        // End placement after one park so the chosen anchor remains visible.
        // Selecting Parks from the toolbar again re-enters placement mode.
        if (ToolManager::isToolActive(self.type, self.number))
        {
            ToolManager::toolCancel();
        }
        selectParkAnchor(park.position);
        self.invalidate();
    }

    inline void drawLine(Gfx::TextRenderer& tr, int16_t x, int16_t y, const std::string& text)
    {
        StringManager::setString(StringIds::buffer_1250, text);
        tr.drawStringLeft({ x, y }, Colour::black, StringIds::buffer_1250);
    }

    inline void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawLine(tr, 8, 24, "OpenLoco Hybrid v0.3.0-alpha - native map integration");
        drawLine(tr, 8, 39, "Select Parks again to place a new park on the map.");
        drawLine(tr, 8, 54, "Parks in this session: " + std::to_string(_parks.size()));

        if (_parks.empty())
        {
            drawLine(tr, 8, 79, "Placement active: click a land tile to create Adventure Park #1.");
            drawLine(tr, 8, 94, "No vehicle DAT, cargo DAT or industry DAT is modified.");
            return;
        }

        const auto& park = _parks.back();
        drawLine(tr, 8, 79, "Adventure Park #" + std::to_string(park.id));
        drawLine(tr, 8, 94, "Map anchor: X " + std::to_string(park.position.x) + "  Y " + std::to_string(park.position.y));
        drawLine(tr, 8, 109, "Popularity: " + std::to_string(park.popularity) + "/1000   Capacity: " + std::to_string(park.capacity));
        drawLine(tr, 8, 124, "Visitors this month: " + std::to_string(park.visitorsThisMonth));
        drawLine(tr, 8, 139, "Monthly park result: " + std::to_string(park.monthlyProfit));
        drawLine(tr, 8, 154, "The highlighted tile is the park anchor for the next integration step.");
    }

    inline constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onToolUpdate = onToolUpdate,
        .onToolDown = onToolDown,
        .onToolAbort = onToolAbort,
        .draw = draw,
    };

    inline Ui::Window* open()
    {
        installStrings();

        auto* window = WindowManager::bringToFront(kParkListWindowType, 0);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(
                kParkListWindowType,
                kWindowSize,
                WindowFlags::none,
                kEvents);
            window->number = 0;
            window->setWidgets(kWidgets);
            window->initScrollWidgets();

            if (auto* skin = ObjectManager::get<InterfaceSkinObject>(); skin != nullptr)
            {
                window->setColour(WindowColour::primary, skin->windowTitlebarColour);
                window->setColour(WindowColour::secondary, skin->windowColour);
            }
        }

        beginPlacement(*window);
        window->invalidate();
        return window;
    }
}
