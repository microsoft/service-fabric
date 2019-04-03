// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    internal enum FabricFaultType
    {
        /// <summary>Create random fault on a random single replica</summary>
        Random = 0,

        /// <summary>Create random fault on secondary replica</summary>
        SecondaryRandom = 1,

        /// <summary>Create random fault on primary replica</summary>
        PrimaryRandom = 2,

        /// <summary>Invalid</summary>
        Invalid = 3,

        //
        // faults on multiple replicas
        //

        /// <summary>Cause quorum loss</summary>
        QuorumLoss = 4,

        /// <summary>Cause a partital data loss</summary>
        PartialDataLoss = 5,

        /// <summary>Cause a full data loss</summary>
        FullDataLoss = 6,

        /// <summary>Create faults by restarting partition</summary>
        RestartPartition = 7,

        //
        // faults(permanent/transient) on a random replica
        //

        /// <summary>Create faults by removing a single replica</summary>
        RemoveReplica = 8,

        /// <summary>Create faults by moving the secondary replica</summary>
        MoveSecondary = 9,

        /// <summary>Create faults by restarting a random replica</summary>
        RestartReplica = 10,

        /// <summary>Create faults by restarting node</summary>
        RestartNode = 11,

        /// <summary>Create faults by killing service host</summary>
        KillServiceHost = 12,

        //
        // faults(permanent/transient) on a primary replica
        //

        /// <summary>Create faults by removing primary</summary>
        RemovePrimary = 13,

        /// <summary>Create faults by moving the primary</summary>
        MovePrimary = 14,

        /// <summary>Create faults by restarting primary</summary>
        RestartPrimary = 15,
    }
}