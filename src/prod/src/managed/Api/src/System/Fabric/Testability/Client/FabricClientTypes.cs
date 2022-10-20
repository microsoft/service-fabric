// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client
{
    using System;

    [Flags]
    public enum FabricClientTypes
    {
        None = 0x00,
        System = 0x01,
        Rest = 0x02,
        PowerShell = 0x04,
        All = System | Rest | PowerShell
    }
}


#pragma warning restore 1591