// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string>
#include "Common/Common.h"

namespace FabricInstallerService
{
    struct Constants
    {
        static std::wstring const ServiceName;
        static std::wstring const ServiceDisplayName;

        static std::wstring const FabricHostServiceName;
       
        static std::wstring const CommandLineOptionService;
        static std::wstring const CommandLineOptionServiceShort;
        static std::wstring const CommandLineOptionConsole;
        static std::wstring const CommandLineOptionConsoleShort;
        static std::wstring const CommandLineOptionHelp;
        static std::wstring const CommandLineOptionHelpShort;
        static std::wstring const CommandLineOptionAutoclean;
        static std::wstring const CommandLineOptionAutocleanShort;

        static std::wstring const UpgradeTempDirectoryName;
        static std::wstring const ExtraDirsDirectoryName;
        static std::wstring const RestartValidationFileName;
        static std::wstring const FindAllSearchPattern;
        static std::wstring const OldDirectoryRenameSuffix;

        static Common::StringLiteral MsiInstallCommandLine;
        static Common::StringLiteral MsiUninstallCommandLine;

        static Common::StringLiteral FileCopySentinelFileName;

        static Common::StringLiteral TraceFileFormat;

        //process exit code
        static const unsigned int ExitCodeUnhandledException = 3;
        static const unsigned int FabricUpgradeManagerOpenTimeout = 2;
        static const unsigned int FabricUpgradeManagerCloseTimeout = 5;
        static const unsigned int ServiceStopTimeoutInMinutes = 5;
        static const unsigned int ServiceStartTimeoutInMinutes = 2;
        static const unsigned int FabricHostServiceStopRetryCount = 5;
        static const unsigned int UpgradeTimeoutInMinutes = 10;
        static const unsigned int FabricNodeOpenTimeoutInSeconds = 60;
        static const unsigned int FabricNodeUpgradeCompleteTimeoutInSeconds = 300;
        static const unsigned int FabricNodeUpgradeCompletePollIntervalInSeconds = 2;

        static const unsigned int ChildrenFolderCount = 2;
        static std::wstring ChildrenFolders[];

        static constexpr size_t EstimatedValidationFileSize = MAX_PATH * 400 * sizeof(wchar_t); // [MAX_PATH] * [~400 files] * [sizeof(wchar_t)]
    };
}

