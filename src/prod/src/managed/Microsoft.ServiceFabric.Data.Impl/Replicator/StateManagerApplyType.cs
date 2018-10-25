// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// State manager operation apply type.
    /// </summary>
    internal enum StateManagerApplyType : byte
    {
        Invalid = 0,

        Insert = 1,

        Delete = 2,
    }
}