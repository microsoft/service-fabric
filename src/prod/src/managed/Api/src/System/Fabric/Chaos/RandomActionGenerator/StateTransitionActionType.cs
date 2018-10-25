// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    /// <summary>
    /// This enum will represent a type of action RandomSessionAction on WindowsFabric cluster which gets generated during RandomSession.
    /// </summary>
    enum StateTransitionActionType
    {
        /// <summary>
        /// Forces the Failover Manager service to rebuild GFUM
        /// </summary>
        FmRebuild,
        NodeUp,
        NodeDown,
        NodeRestart,
        CodePackageRestart,
        StartWorkload,
        StopWorkload,
        RemoveReplica,
        RestartReplica,
        MovePrimary,
        MoveSecondary,
        MAX
    }
}