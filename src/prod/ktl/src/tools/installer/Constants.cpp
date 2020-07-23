//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "Constants.h"
#include "Path.h"

using namespace std;

const int Constants::NumberDrivers = 1;
const struct InfAndDevNode Constants::InstallerList[Constants::NumberDrivers] = {
    { L"KtlLogger.inf", L"root\\KtlLogger" },
};

const wstring Constants::FilesToBeSigned = L"KtlLogger.sys";

const wstring Constants::CurrentModulePath = PathHelper::GetDirectoryName(PathHelper::GetModuleLocation());

static const wstring UtilityDirectory;
DWORD Constants::ExitCodeOK = 0;
DWORD Constants::ExitCodeReboot = 1;
DWORD Constants::ExitCodeFail = 2;
DWORD Constants::ExitCodeUsage = 3;
