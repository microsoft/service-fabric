// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    internal enum UnlockSource : short
    {
        EnqueueLockContext = 1,
        DequeueLockContext = 2,
        ApplyEnqueue = 3
    }
}