// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricClient.h"
#include "FabricClient_.h"

int TestFabricClient ()
{
    IID iid;
    FabricCreateClient(0, nullptr, iid, nullptr);
    FabricCreateLocalClient(iid, nullptr);
    FabricCreateClient2(0, nullptr, nullptr, iid, nullptr);
    FabricCreateLocalClient2(nullptr, iid, nullptr);
    FabricCreateClient3(0, nullptr, nullptr, nullptr, iid, nullptr);
    FabricCreateLocalClient3(nullptr, nullptr, iid, nullptr);
    FabricCreateLocalClient4(nullptr, nullptr, (FABRIC_CLIENT_ROLE) 0, iid, nullptr);
    GetFabricClientDefaultSettings(nullptr);
    FabricGetDefaultRollingUpgradeMonitoringPolicy(nullptr);
}
