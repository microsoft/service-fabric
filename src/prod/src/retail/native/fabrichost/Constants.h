// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string>
#include "Common/Common.h"

namespace FabricHost
{
    struct Constants
    {
        static std::wstring const ServiceName;
        static std::wstring const ServiceDisplayName;
        static std::wstring const ActivatorConfigFileName;
        static std::wstring const ConfigSectionName;

        static std::wstring const DataPath;

        static std::wstring const CommandLineOptionService;
        static std::wstring const CommandLineOptionServiceShort;
        static std::wstring const CommandLineOptionConsole;
        static std::wstring const CommandLineOptionConsoleShort;
        static std::wstring const CommandLineOptionActivateHidden;
        static std::wstring const CommandLineOptionActivateHiddenShort;
        static std::wstring const CommandLineOptionSkipFabricSetup;
        static std::wstring const CommandLineOptionSkipFabricSetupShort;
        static std::wstring const CommandLineOptionInstall;
        static std::wstring const CommandLineOptionInstallShort;
        static std::wstring const CommandLineOptionUninstall;
        static std::wstring const CommandLineOptionUninstallShort;
        static std::wstring const CommandLineOptionHelp;
        static std::wstring const CommandLineOptionHelpShort;

        //process exit code
        static const unsigned int ExitCodeUnhandledException = 3;

        static const unsigned int PreShutdownBlockTimeInMinutes = 120;
        static const unsigned int NodeDisableWaitTimeInMinutes = 120;
    };
}

