// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Concurrent;
    using System.Fabric.WRP.Model;
    using System.Threading;

    internal interface IResourceOperations
    {
        bool OperationExists(IOperationDescription description);
        bool TryAdd(IOperationDescription description);
        bool TryRemove(IOperationStatus status);
        bool IsEmpty();
    }

    internal class ResourceOperations : IResourceOperations
    {
        public ResourceOperations()
        {
            this.Operations = new ConcurrentDictionary<string, long>();
        }

        public ConcurrentDictionary<string, long> Operations { get; }
        public bool OperationExists(IOperationDescription description)
        {
            return this.Operations.ContainsKey(description.ResourceId) &&
                   this.Operations[description.ResourceId] == description.OperationSequenceNumber;
        }

        public bool TryAdd(IOperationDescription description)
        {
            return this.Operations.TryAdd(
                        description.ResourceId,
                        description.OperationSequenceNumber);
        }

        public bool TryRemove(IOperationStatus status)
        {
            var operationId = Int64.MinValue;
            return this.Operations.TryRemove(
                        status.ResourceId,
                        out operationId);
        }

        public bool IsEmpty()
        {
            return this.Operations.IsEmpty;
        }
    }
}
