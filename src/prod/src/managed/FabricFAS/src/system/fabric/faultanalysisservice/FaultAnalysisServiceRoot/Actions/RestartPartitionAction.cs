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

    internal class RestartPartitionAction : FabricTestAction
    {
        public RestartPartitionAction(IReliableStateManager stateManager, IStatefulServicePartition partition, RestartPartitionState state, PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(stateManager, partition, state, requestTimeout, operationTimeout)
        {
            ThrowIf.Null(partitionSelector, "partitionSelector");

            this.PartitionSelector = partitionSelector;
            this.RestartPartitionMode = restartPartitionMode;
        }

        public PartitionSelector PartitionSelector { get; set; }

        public RestartPartitionMode RestartPartitionMode { get; set; }

        public override StepBase GetStep(
            FabricClient fabricClient,
            ActionStateBase actionState,
            StepStateNames stateName,
            CancellationToken cancellationToken)
        {
            return RestartPartitionStepsFactory.GetStep(stateName, fabricClient, actionState, this, this.RequestTimeout, this.OperationTimeout, cancellationToken);
        }
    }
}