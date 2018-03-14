// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricCommon.h"
#include "FabricCommon_.h"

int TestFabricCommon ()
{
    IID iid;
    FabricGetConfigStore(iid, nullptr, nullptr);
    FabricGetConfigStoreEnvironmentVariable(nullptr, nullptr);
    FabricSetLastErrorMessage(nullptr, nullptr);
    FabricGetLastErrorMessage(nullptr);
    FabricClearLastErrorMessage(0);
    VerifyFileSignature(nullptr, nullptr);
}
