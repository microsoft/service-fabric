// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    [Flags]
    internal enum ClusterEntityFlags
    {
        None = 0x0000,
        // An entity is Faulted when the entity itself is undergoing a fault. 
        // For example a NodeEntity is faulted when it is being restarted or
        // a ReplicaEntity is Faulted when it is being removed
        Faulted = 0x0001,
        // An entity is Unavailable when any of its child/related entities are faulted
        // thus making it unselectable because selecting the parent would end up in a double fault. 
        // For example a NodeEntity is unavailable when a 
        // corresponding CodePackageEntity which runs on the node is faulted (restarted)
        Unavailable = 0x0002,
        // An entity is unsafe when a fault on this entity could make a partition in the 
        // cluster go below its fault tolerance thus making it potentially in an 
        // unavailable (quorum loss) or data loss state. For example a NodeEntity is unsafe to 
        // fault when any partition hosted on this node is already close to fault tolerance level
        Unsafe = 0x0004,
        // An entity is excluded from faulting if EnableChaosTargetFilter is set to true and
        // not included through one of the inclusion lists in the ChaosTargetFilter
        Excluded = 0x0008
    }
}