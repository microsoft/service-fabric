// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Numerics;
    using System.Threading;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;

    internal class StartNodeFromFASAction : FabricTestAction
    {
        public StartNodeFromFASAction(IReliableStateManager stateManager, IStatefulServicePartition partition, NodeCommandState state, IReliableDictionary<string, bool> stoppedNodeTable, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(stateManager, partition, state, requestTimeout, operationTimeout)
        {
            this.StoppedNodeTable = stoppedNodeTable;            
        }

        public IReliableDictionary<string, bool> StoppedNodeTable
        {
            get;
            private set;
        }

        public string NodeName { get; set; }

        public BigInteger NodeInstanceId { get; set; }

        public override StepBase GetStep(
            FabricClient fabricClient,
            ActionStateBase actionState,
            StepStateNames stateName,
            CancellationToken cancellationToken)
        {
            return StartNodeStepsFactory.GetStep(stateName, fabricClient, actionState, this, this.RequestTimeout, this.OperationTimeout, cancellationToken);
        }
    }
}