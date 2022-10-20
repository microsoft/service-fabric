// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class StartPartitionQuorumLossRequest : FabricRequest
    {
        public StartPartitionQuorumLossRequest(IFabricClient fabricClient, Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumLossMode, TimeSpan quorumLossDuration, TimeSpan timeout)
        : this(fabricClient, operationId, partitionSelector, quorumLossMode, quorumLossDuration, timeout, false)
        { }

        public StartPartitionQuorumLossRequest(IFabricClient fabricClient, Guid operationId, PartitionSelector partitionSelector, QuorumLossMode quorumLossMode, TimeSpan quorumLossDuration, TimeSpan timeout, bool expectError)
            : base(fabricClient, timeout)
        {
            this.OperationId = operationId;
            this.PartitionSelector = partitionSelector;
            this.QuorumLossMode = quorumLossMode;
            this.QuorumLossDuration = quorumLossDuration;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NOT_READY);
            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RECONFIGURATION_PENDING);

            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS);

            if (expectError)
            {
                this.SucceedErrorCodes.Clear();
                this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TEST_COMMAND_OPERATION_ID_ALREADY_EXISTS);
            }
        }

        public Guid OperationId
        {
            get;
            private set;
        }

        public PartitionSelector PartitionSelector
        {
            get;
            private set;
        }

        public QuorumLossMode QuorumLossMode
        {
            get;
            private set;
        }

        public TimeSpan QuorumLossDuration
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "StartPartitionQuorumLossRequest with OperationId={0}, timeout={1}", this.OperationId, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.StartPartitionQuorumLossAsync(this.OperationId, this.PartitionSelector, this.QuorumLossMode, this.QuorumLossDuration, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591