#!/usr/bin/env python3
from pathlib import Path

TOOLBAR = Path("src/OpenLoco/src/Ui/Windows/ToolbarTop.cpp")
COMPANY_MANAGER = Path("src/OpenLoco/src/World/CompanyManager.cpp")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"Expected exactly one {label} block, found {count}")
    return text.replace(old, new, 1)


def patch_toolbar() -> None:
    text = TOOLBAR.read_text(encoding="utf-8")

    include_old = '#include "GameState.h"\n'
    include_new = '#include "GameState.h"\n#include "Hybrid/ParkWindows.h"\n'
    if '#include "Hybrid/ParkWindows.h"' not in text:
        text = replace_once(text, include_old, include_new, "GameState include")

    old_mouse_down = '''    static void townsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)\n    {\n        auto interface = ObjectManager::get<InterfaceSkinObject>();\n        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });\n        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });\n        Dropdown::showBelow(&self, widgetIndex, 2, 25, (1 << 6));\n        Dropdown::setHighlightedItem(_defaultTownObjectId);\n    }\n'''

    new_mouse_down = '''    static void townsMenuMouseDown(Window& self, WidgetIndex_t widgetIndex)\n    {\n        auto interface = ObjectManager::get<InterfaceSkinObject>();\n        OpenLoco::Hybrid::ParkWindows::installStrings();\n        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });\n        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });\n        Dropdown::add(2, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, OpenLoco::Hybrid::ParkWindows::kStringMenuParks });\n        Dropdown::showBelow(&self, widgetIndex, 3, 25, (1 << 6));\n        Dropdown::setHighlightedItem(_defaultTownObjectId);\n    }\n'''

    if 'OpenLoco::Hybrid::ParkWindows::kStringMenuParks' not in text:
        text = replace_once(text, old_mouse_down, new_mouse_down, "townsMenuMouseDown")

    old_dropdown = '''        else if (itemIndex == 1)\n        {\n            IndustryList::open();\n            _defaultTownObjectId = 1;\n        }\n'''

    new_dropdown = '''        else if (itemIndex == 1)\n        {\n            IndustryList::open();\n            _defaultTownObjectId = 1;\n        }\n        else if (itemIndex == 2)\n        {\n            OpenLoco::Hybrid::ParkWindows::open();\n            _defaultTownObjectId = 2;\n        }\n'''

    if 'OpenLoco::Hybrid::ParkWindows::open();' not in text:
        text = replace_once(text, old_dropdown, new_dropdown, "townsMenuDropdown")

    TOOLBAR.write_text(text, encoding="utf-8", newline="\n")


def patch_company_manager() -> None:
    text = COMPANY_MANAGER.read_text(encoding="utf-8")

    include_old = '#include "GameState.h"\n'
    include_new = '#include "GameState.h"\n#include "Hybrid/ParkManager.h"\n'
    if '#include "Hybrid/ParkManager.h"' not in text:
        text = replace_once(text, include_old, include_new, "CompanyManager GameState include")

    old_monthly = '''        for (auto& company : companies())\n        {\n            company.updateMonthly1();\n        }\n        Ui::WindowManager::invalidate(Ui::WindowType::company);\n'''

    new_monthly = '''        for (auto& company : companies())\n        {\n            company.updateMonthly1();\n        }\n\n        OpenLoco::Hybrid::Parks::updateMonthly();\n\n        Ui::WindowManager::invalidate(Ui::WindowType::company);\n'''

    if 'OpenLoco::Hybrid::Parks::updateMonthly();' not in text:
        text = replace_once(text, old_monthly, new_monthly, "CompanyManager monthly update")

    COMPANY_MANAGER.write_text(text, encoding="utf-8", newline="\n")


def main() -> None:
    patch_toolbar()
    patch_company_manager()
    print("Hybrid v0.3.1 regional UI + park economy integration applied successfully.")


if __name__ == "__main__":
    main()
