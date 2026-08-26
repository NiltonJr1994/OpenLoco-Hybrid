#pragma once

#include "Hybrid/Rct2AssetRegistry.h"

#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Platform/Platform.h>

#include <cstdint>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace OpenLoco::Hybrid::Rct2Bridge
{
    inline std::string _lastStatus{ "OpenRCT2 bridge has not been launched yet." };

    inline fs::path runtimeExecutable()
    {
#ifdef _WIN32
        return Platform::getCurrentExecutablePath().parent_path() / "rct2-runtime" / "openrct2.exe";
#else
        return Platform::getCurrentExecutablePath().parent_path() / "rct2-runtime" / "openrct2";
#endif
    }

    inline fs::path parkUserDataPath(uint16_t parkId)
    {
        return Platform::getCurrentExecutablePath().parent_path() / "hybrid-data" / "parks" / ("park-" + std::to_string(parkId));
    }

    inline bool runtimeAvailable()
    {
        try
        {
            return fs::is_regular_file(runtimeExecutable());
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

#ifdef _WIN32
    inline std::wstring quote(const fs::path& path)
    {
        return L"\"" + path.wstring() + L"\"";
    }
#endif

    inline bool launchDetailedPark(uint16_t parkId)
    {
        const auto& assets = Rct2Assets::get();
        if (!assets.ready)
        {
            _lastStatus = "RCT2 assets are not ready. Rescan the RCT2 folder first.";
            return false;
        }

        const auto* scenario = Rct2Assets::selectedScenario();
        if (scenario == nullptr)
        {
            _lastStatus = "No RCT2 .SC6 scenario is available for the park template.";
            return false;
        }

        const auto runtime = runtimeExecutable();
        if (!runtimeAvailable())
        {
            _lastStatus = "OpenRCT2 runtime is missing from rct2-runtime beside OpenLoco.exe.";
            return false;
        }

        const auto userData = parkUserDataPath(parkId);
        try
        {
            fs::create_directories(userData);
        }
        catch (const std::exception& e)
        {
            _lastStatus = std::string("Unable to create park user-data folder: ") + e.what();
            return false;
        }

#ifdef _WIN32
        std::wstring commandLine = quote(runtime);
        commandLine += L" " + quote(*scenario);
        commandLine += L" --rct2-data-path " + quote(assets.root);
        commandLine += L" --user-data-path " + quote(userData);

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        auto mutableCommand = commandLine;
        const auto workingDir = runtime.parent_path().wstring();

        const BOOL launched = CreateProcessW(
            nullptr,
            mutableCommand.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NEW_PROCESS_GROUP,
            nullptr,
            workingDir.c_str(),
            &startupInfo,
            &processInfo);

        if (!launched)
        {
            _lastStatus = "Windows could not start the OpenRCT2 detailed-park runtime (error " + std::to_string(GetLastError()) + ").";
            return false;
        }

        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        _lastStatus = "Detailed park opened in OpenRCT2. TD6 designs are read from the original RCT2 Tracks folder.";
        return true;
#else
        (void)parkId;
        _lastStatus = "The detailed-park launcher is enabled for the Windows x64 alpha build.";
        return false;
#endif
    }
}
