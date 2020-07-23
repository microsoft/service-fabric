// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    [Flags]
    [Serializable]
    public enum WinFabricRepairTaskResult
    {
        Invalid = 0,
        Succeeded = 1,
        Cancelled = 2,
        Interrupted = 4,
        Failed = 8,
        Pending = 16
    }
}

#pragma warning restore 1591