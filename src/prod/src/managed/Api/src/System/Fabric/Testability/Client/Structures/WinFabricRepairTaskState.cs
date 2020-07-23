// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    [Serializable]
    public enum WinFabricRepairTaskState
    {
        Invalid = 0,
        Created = 1,
        Claimed = 2,
        Preparing = 4,
        Approved = 8,
        Executing = 16,
        Restoring = 32,
        Completed = 64,
    }

    [Flags]
    [Serializable]
    public enum WinFabricRepairTaskStateFilter
    {
        Default = 0,
        Created = WinFabricRepairTaskState.Created,
        Claimed = WinFabricRepairTaskState.Claimed,
        Preparing = WinFabricRepairTaskState.Preparing,
        Approved = WinFabricRepairTaskState.Approved,
        Executing = WinFabricRepairTaskState.Executing,
        Restoring = WinFabricRepairTaskState.Restoring,
        Completed = WinFabricRepairTaskState.Completed,
        ReadyToExecute = Approved | Executing,
        Active = Created | Claimed | Preparing | Approved | Executing | Restoring,
        All = Active | Completed,
    }
}

#pragma warning restore 1591