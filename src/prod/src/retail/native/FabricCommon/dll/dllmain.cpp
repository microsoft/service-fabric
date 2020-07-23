// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "../stdafx.h"

BOOL FabricCommonDllMainImpl(
    HMODULE module,
    DWORD reason,
    LPVOID reserved);
    
BOOL APIENTRY DllMain(
    HMODULE module,
    DWORD reason,
    LPVOID reserved)
{
    return FabricCommonDllMainImpl(module, reason, reserved);
}