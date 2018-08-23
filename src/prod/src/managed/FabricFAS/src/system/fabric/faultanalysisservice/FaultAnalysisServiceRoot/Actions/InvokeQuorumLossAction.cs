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

    internal class InvokeQuorumLossAction : FabricTestAction
    {
        public InvokeQuorumLossAction(IReliableStateManager stateManager, IStatefulServicePartition partition, InvokeQuorumLossState state, PartitionSelector partitionSelector, QuorumLossMode quorumLossMode, TimeSpan quorumLossDuration, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(stateManager, partition, state, requestTimeout, operationTimeout)
        {
            ThrowIf.Null(partitionSelector, "partitionSelector");

            this.PartitionSelector = partitionSelector;
            this.QuorumLossMode = quorumLossMode;
            this.QuorumLossDuration = quorumLossDuration;
        }

        public PartitionSelector PartitionSelector { get; set; }

        public QuorumLossMode QuorumLossMode { get; set; }

        public TimeSpan QuorumLossDuration { get; set; }

        public override StepBase GetStep(
            FabricClient fabricClient,
            ActionStateBase actionState,
            StepStateNames stateName,
            CancellationToken cancellationToken)
        {
            return QuorumLossStepsFactory.GetStep(stateName, fabricClient, actionState, this, this.RequestTimeout, this.OperationTimeout, cancellationToken);
        }
    }
}