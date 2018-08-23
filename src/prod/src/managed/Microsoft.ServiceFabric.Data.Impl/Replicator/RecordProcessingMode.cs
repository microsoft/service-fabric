// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    internal enum RecordProcessingMode
    {
        Normal = 0,

        ApplyImmediately = 1,

        ProcessImmediately = 2,
    }
}