// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    internal enum OperationType : byte
    {
        Invalid = 0, 

        Enqueue = 1, 

        Dequeue = 2, 
    }
}