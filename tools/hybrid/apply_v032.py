#!/usr/bin/env python3
from pathlib import Path

PARK_WINDOWS = Path("src/OpenLoco/src/Hybrid/ParkWindows.h")
TOOLBAR = Path("src/OpenLoco/src/Ui/Windows/ToolbarTop.cpp")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"Expected exactly one {label} block, found {count}")
    return text.replace(old, new, 1)


def patch_park_windows() -> None:
    text = PARK_WINDOWS.read_text(encoding="utf-8")

    text = replace_once(
        text,
        '        tr.drawString({ self.x + x, self.y + y }, Colour::black, text.c_str());\n',
        '        tr.drawString({ x, y }, Colour::black, text.c_str());\n',
        "window-local drawText coordinates",
    )

    text = replace_once(
        text,
        '                const auto centreX = self.x + widget.left + (widget.width() / 2);\n                const auto y = self.y + widget.top + 5;\n',
        '                const auto centreX = widget.left + (widget.width() / 2);\n                const auto y = widget.top + 5;\n',
        "window-local button text coordinates",
    )

    old_anchor = '''    inline void selectParkAnchor(const World::Pos2& position)\n    {\n        clearMapSelection();\n        World::setMapSelectionFlags(World::MapSelectionFlags::enable);\n        World::setMapSelectionCorner(MapSelectionType::full);\n        World::setMapSelectionArea(position, position);\n        World::mapInvalidateSelectionRect();\n    }\n'''
    new_anchor = '''    inline void selectParkAnchor(const World::Pos2& position)\n    {\n        clearMapSelection();\n        const auto [minPos, maxPos] = Parks::footprintBounds(position);\n        World::setMapSelectionFlags(World::MapSelectionFlags::enable);\n        World::setMapSelectionCorner(MapSelectionType::full);\n        World::setMapSelectionArea(minPos, maxPos);\n        World::mapInvalidateSelectionRect();\n    }\n'''
    text = replace_once(text, old_anchor, new_anchor, "7x7 park selection")

    old_tool_update = '''        World::setMapSelectionFlags(World::MapSelectionFlags::enable);\n        World::setMapSelectionCorner(MapSelectionType::full);\n        World::setMapSelectionArea(*mapPos, *mapPos);\n        World::mapInvalidateSelectionRect();\n'''
    new_tool_update = '''        selectParkAnchor(*mapPos);\n'''
    text = replace_once(text, old_tool_update, new_tool_update, "7x7 placement preview")

    old_prepare = '''        const bool hasPark = Parks::selectedPark() != nullptr;\n        if (auto* w = findWidget(self, RegionalWidx::enterPark); w != nullptr)\n            w->hidden = !hasPark;\n        if (auto* w = findWidget(self, RegionalWidx::previous); w != nullptr)\n            w->hidden = Parks::_parks.size() < 2;\n        if (auto* w = findWidget(self, RegionalWidx::next); w != nullptr)\n            w->hidden = Parks::_parks.size() < 2;\n'''
    new_prepare = '''        const bool hasPark = Parks::selectedPark() != nullptr;\n        if (auto* w = findWidget(self, RegionalWidx::buildPark); w != nullptr)\n            w->disabled = !Parks::hasRct2Assets();\n        if (auto* w = findWidget(self, RegionalWidx::enterPark); w != nullptr)\n            w->hidden = !hasPark;\n        if (auto* w = findWidget(self, RegionalWidx::previous); w != nullptr)\n            w->hidden = Parks::_parks.size() < 2;\n        if (auto* w = findWidget(self, RegionalWidx::next); w != nullptr)\n            w->hidden = Parks::_parks.size() < 2;\n'''
    text = replace_once(text, old_prepare, new_prepare, "asset-gated build button")

    text = text.replace(
        'OpenLoco Hybrid v0.3.1-alpha - regional park integration',
        'OpenLoco Hybrid v0.3.2-alpha - regional park footprint integration',
    )
    text = text.replace(
        'Park construction cost: 75,000 (booked as Construction)',
        'RCT2 assets: READY | Regional site: 7x7 tiles (49 tiles)',
    )
    text = text.replace(
        'Parks owned in this session: ',
        'Construction: 75,000 | Parks in this session: ',
    )
    text = text.replace(
        'Map anchor: X ',
        'Site centre: X ',
    )
    text = text.replace(
        'The highlighted tile is the park anchor',
        'The highlighted 7x7 area is the regional park footprint',
    )

    PARK_WINDOWS.write_text(text, encoding="utf-8", newline="\n")


def patch_toolbar() -> None:
    text = TOOLBAR.read_text(encoding="utf-8")

    old_mouse_down = '''    static void townsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)\n    {\n        auto interface = ObjectManager::get<InterfaceSkinObject>();\n        OpenLoco::Hybrid::ParkWindows::installStrings();\n        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });\n        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });\n        Dropdown::add(2, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, OpenLoco::Hybrid::ParkWindows::kStringMenuParks });\n        Dropdown::showBelow(&self, widgetIndex, 3, 25, (1 << 6));\n        Dropdown::setHighlightedItem(_defaultTownObjectId);\n    }\n'''
    new_mouse_down = '''    static void townsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)\n    {\n        auto interface = ObjectManager::get<InterfaceSkinObject>();\n        OpenLoco::Hybrid::ParkWindows::installStrings();\n        const bool parksAvailable = OpenLoco::Hybrid::Parks::hasRct2Assets();\n        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });\n        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });\n        if (parksAvailable)\n        {\n            Dropdown::add(2, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, OpenLoco::Hybrid::ParkWindows::kStringMenuParks });\n        }\n        const uint8_t itemCount = parksAvailable ? 3 : 2;\n        if (!parksAvailable && _defaultTownObjectId > 1)\n        {\n            _defaultTownObjectId = 0;\n        }\n        Dropdown::showBelow(&self, widgetIndex, itemCount, 25, (1 << 6));\n        Dropdown::setHighlightedItem(_defaultTownObjectId);\n    }\n'''
    text = replace_once(text, old_mouse_down, new_mouse_down, "RCT2 assets toolbar gate")

    old_dropdown = '''        else if (itemIndex == 2)\n        {\n            OpenLoco::Hybrid::ParkWindows::open();\n            _defaultTownObjectId = 2;\n        }\n'''
    new_dropdown = '''        else if (itemIndex == 2 && OpenLoco::Hybrid::Parks::hasRct2Assets())\n        {\n            OpenLoco::Hybrid::ParkWindows::open();\n            _defaultTownObjectId = 2;\n        }\n'''
    text = replace_once(text, old_dropdown, new_dropdown, "RCT2 assets dropdown gate")

    TOOLBAR.write_text(text, encoding="utf-8", newline="\n")


def main() -> None:
    patch_park_windows()
    patch_toolbar()
    print("Hybrid v0.3.2 assets gate + 7x7 park footprint patch applied successfully.")


if __name__ == "__main__":
    main()
