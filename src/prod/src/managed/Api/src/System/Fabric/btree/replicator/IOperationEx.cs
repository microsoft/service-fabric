// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    public interface IOperationEx
    {
        OperationTypeEx OperationType { get; }

        long SequenceNumber { get; }

        long AtomicGroupId { get; }

        IOperationData ServiceMetadata { get; }

        IOperationData Data { get; }

        void Acknowledge();
    }
}