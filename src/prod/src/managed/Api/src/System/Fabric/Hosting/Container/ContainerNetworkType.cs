//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal enum ContainerNetworkType
    {
        Other = NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_OTHER,
        
        Open = NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_OPEN,
        
        Isolated = NativeTypes.FABRIC_CONTAINER_NETWORK_TYPE.FABRIC_CONTAINER_NETWORK_TYPE_ISOLATED
    }
}
