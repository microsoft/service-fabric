// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LocalClusterManager
{
    internal enum ClusterOperationType
    {
        None = 0,
        Manage = 1,
        Reset = 2,
        Start = 3,
        Stop = 4,
        SetupOneNodeCluster = 5,
        SetupFiveNodeCluster = 6,
        SwitchToOneNodeCluster = 7,
        SwitchToFiveNodeCluster = 8,
        Remove = 9,
        SetupOneNodeMeshCluster = 10,
        SetupFiveNodeMeshCluster = 11,
        SwitchToMeshOneNodeCluster = 12,
        SwitchToMeshFiveNodeCluster = 13,
    }
}