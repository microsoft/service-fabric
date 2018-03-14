// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FabricRuntime.h"
#include "FabricRuntime_.h"

int TestFabricRuntime ()
{
    IID iid;
    FABRIC_PARTITION_ID pid;
    FABRIC_URI serviceName;
	  FabricBeginCreateRuntime(iid, nullptr, 0, nullptr, nullptr);
	  FabricEndCreateRuntime(nullptr, nullptr);
    FabricCreateRuntime(iid, nullptr);
    FabricBeginGetActivationContext(iid, 0, nullptr, nullptr);
    FabricEndGetActivationContext(nullptr, nullptr);
    FabricGetActivationContext(iid, nullptr);
    FabricRegisterCodePackageHost(nullptr);
    FabricCreateKeyValueStoreReplica(iid, nullptr, pid, 0, nullptr, (FABRIC_LOCAL_STORE_KIND) 0, nullptr, nullptr, nullptr);
    FabricCreateKeyValueStoreReplica2(iid, nullptr, pid, 0, nullptr, (FABRIC_LOCAL_STORE_KIND) 0, nullptr, nullptr, nullptr, (FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE) 0, nullptr);
    FabricCreateKeyValueStoreReplica3(iid, nullptr, pid, 0, nullptr, (FABRIC_LOCAL_STORE_KIND) 0, nullptr, nullptr, nullptr, (FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE) 0, nullptr);
    FabricCreateKeyValueStoreReplica4(iid, nullptr, pid, 0, serviceName, nullptr, (FABRIC_LOCAL_STORE_KIND) 0, nullptr, nullptr, nullptr, (FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE) 0, nullptr);
    FabricCreateKeyValueStoreReplica5(iid, nullptr, pid, 0, serviceName, nullptr, nullptr, (FABRIC_LOCAL_STORE_KIND) 0, nullptr, nullptr, nullptr, nullptr);
    FabricLoadReplicatorSettings(nullptr, nullptr, nullptr, nullptr);
}
