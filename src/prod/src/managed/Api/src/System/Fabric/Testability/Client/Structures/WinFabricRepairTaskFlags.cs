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
    public enum WinFabricRepairTaskFlags
    {
        None = 0,
        CancelRequested = 1,
        AbortRequested = 2,
        ForcedApproval = 4,
        ValidMask = 7,
    }
}

#pragma warning restore 1591