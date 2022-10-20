//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include "PnpDriverInstaller.h"
#include <string>

class Constants
{
public:
    static const int NumberDrivers;
    static const InfAndDevNode InstallerList[];
    static const std::wstring CurrentModulePath;
    static const std::wstring FilesToBeSigned;
    static DWORD ExitCodeOK;
    static DWORD ExitCodeReboot;
    static DWORD ExitCodeFail;
    static DWORD ExitCodeUsage;
};
