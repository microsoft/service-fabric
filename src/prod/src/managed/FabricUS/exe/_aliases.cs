// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    // FabricUS accesses a lot of internals in System.Fabric.dll (S). In order to use
    // KeyValueStoreReplica_V2, FabricUS must also access internals of Microsoft.ServiceFabric.Internal.dll (M).
    //
    // (M) copies over many internal classes from (S), which causes ambiguity (compile errors)
    // for internal classes used by FabricUS that exist in both DLLs. "extern alias" is used
    // to disambiguate these classes.
    //
    extern alias ServiceFabricInternal;
    using IConfigStore = ServiceFabricInternal::System.Fabric.Common.IConfigStore;
}