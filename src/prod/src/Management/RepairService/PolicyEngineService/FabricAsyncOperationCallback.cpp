// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace RepairPolicyEngine;

FabricAsyncOperationCallback::FabricAsyncOperationCallback()
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationCallback::FabricAsyncOperationCallback()");
}

FabricAsyncOperationCallback::~FabricAsyncOperationCallback()
{
    Trace.WriteNoise(ComponentName, "FabricAsyncOperationCallback::~FabricAsyncOperationCallback()");
}
