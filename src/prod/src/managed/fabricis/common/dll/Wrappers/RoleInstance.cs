// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// The different states a role instance can take.
    /// </summary>
    /// <remarks>
    /// Both the old and new MR libraries have the same mapping
    /// TODO, try simplify! it is currently pretty confusing since we have 3 enums mapping to the same thing
    /// </remarks>
    internal enum RoleInstanceState
    {
        Unknown = 0,
        CreatingVM = 1,
        StartingRole = 2,
        ReadyRole = 3,
        BusyRole = 4,
        UnresponsiveRole = 5,
        StoppingRole = 6,
        StoppedVM = 7,
        RestartingRole = 8,
        CyclingRole = 9,
        FailedStartingRole = 10,
        FailedStartingVM = 11,
        RetiredVM = 12,
        Unhealthy = 13,
    }

    /// <summary>
    /// A wrapper for the ManagementRoleInstance class and RoleInstanceHealthInfo classes present 
    /// in the old and new MR SDKs respectively.
    /// </summary>
    internal sealed class RoleInstance
    {
        public RoleInstance(string id, RoleInstanceState status, DateTime lastUpdateTime)
        {
            this.Id = id;
            this.Status = status;
            this.LastUpdateTime = lastUpdateTime;
        }

        public string Id { get; private set; }

        public RoleInstanceState Status { get; private set; }

        public DateTime LastUpdateTime { get; private set; }
    }
}