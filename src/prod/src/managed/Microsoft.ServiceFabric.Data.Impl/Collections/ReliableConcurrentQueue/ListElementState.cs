// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    internal enum ListElementState : short
    {
        Invalid = 0,

        EnqueueInFlight = 1,

        EnqueueApplied = 2,

        EnqueueUndone = 3,

        DequeueInFlight = 4,

        DequeueApplied = 5,
    }
}