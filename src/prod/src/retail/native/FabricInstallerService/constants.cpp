// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace FabricInstallerService;

wstring const Constants::ServiceName = L"FabricInstallerSvc";
wstring const Constants::ServiceDisplayName = L"Fabric Installer Service";

wstring const Constants::FabricHostServiceName = L"FabricHostSvc";

wstring const Constants::CommandLineOptionService = L"--service";
wstring const Constants::CommandLineOptionServiceShort = L"-s";
wstring const Constants::CommandLineOptionConsole = L"--console";
wstring const Constants::CommandLineOptionConsoleShort = L"-c";
wstring const Constants::CommandLineOptionHelp = L"--help";
wstring const Constants::CommandLineOptionHelpShort = L"-h";
wstring const Constants::CommandLineOptionAutoclean = L"--autoclean";
wstring const Constants::CommandLineOptionAutocleanShort = L"--a";

wstring const Constants::UpgradeTempDirectoryName = L"UT";
wstring const Constants::ExtraDirsDirectoryName = L"ED";
wstring const Constants::RestartValidationFileName = L"UpgradeValidation.bin";
wstring const Constants::FindAllSearchPattern = L"*";
wstring const Constants::OldDirectoryRenameSuffix = L".old";

StringLiteral Constants::MsiInstallCommandLine = "msiexec.exe /i \"{0}\" /l*vx \"{1}\" IACCEPTEULA=yes /qn";
StringLiteral Constants::MsiUninstallCommandLine = "msiexec.exe /x \"{0}\" /l*vx \"{1}\" /qn UNINSTALLBEFOREUPGRADE=yes";

StringLiteral Constants::FileCopySentinelFileName = "FileCopySentinel_{0}.file";

StringLiteral Constants::TraceFileFormat = "FabricInstallerService_{0}_{1}.trace";

wstring Constants::ChildrenFolders[Constants::ChildrenFolderCount] = { L"bin", L"EULA" };
