// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal enum ContainerIsolationMode
    {
        Unknown = NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE.FABRIC_CONTAINER_ISOLATION_MODE_UNKNOWN,
        
        Process = NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE.FABRIC_CONTAINER_ISOLATION_MODE_PROCESS,
        
        HyperV = NativeTypes.FABRIC_CONTAINER_ISOLATION_MODE.FABRIC_CONTAINER_ISOLATION_MODE_HYPER_V
    }
}