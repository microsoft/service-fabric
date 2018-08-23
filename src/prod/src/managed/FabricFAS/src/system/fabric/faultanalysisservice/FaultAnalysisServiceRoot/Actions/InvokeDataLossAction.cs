// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Threading;
    using Microsoft.ServiceFabric.Data;

    internal class InvokeDataLossAction : FabricTestAction
    {
        public InvokeDataLossAction(
            IReliableStateManager stateManager,
            IStatefulServicePartition partition,
            InvokeDataLossState state,
            PartitionSelector partitionSelector,
            DataLossMode dataLossMode,
            int dataLossCheckWaitDurationInSeconds,
            int dataLossCheckPollIntervalInSeconds,
            int replicaDropWaitDurationInSeconds,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout)
            : base(stateManager, partition, state, requestTimeout, operationTimeout)
        {
            ThrowIf.Null(partitionSelector, "partitionSelector");

            this.PartitionSelector = partitionSelector;
            this.DataLossMode = dataLossMode;
            this.DataLossCheckWaitDurationInSeconds = dataLossCheckWaitDurationInSeconds;
            this.DataLossCheckPollIntervalInSeconds = dataLossCheckPollIntervalInSeconds;
            this.ReplicaDropWaitDurationInSeconds = replicaDropWaitDurationInSeconds;
        }

        public PartitionSelector PartitionSelector { get; set; }

        public DataLossMode DataLossMode { get; set; }

        public int DataLossCheckWaitDurationInSeconds { get; private set; }

        public int DataLossCheckPollIntervalInSeconds { get; private set; }

        public int ReplicaDropWaitDurationInSeconds { get; private set; }

        public override StepBase GetStep(
            FabricClient fabricClient,
            ActionStateBase actionState,
            StepStateNames stateName,
            CancellationToken cancellationToken)
        {
            return DataLossStepsFactory.GetStep(stateName, fabricClient, actionState, this, this.RequestTimeout, this.OperationTimeout, cancellationToken);
        }
    }
}