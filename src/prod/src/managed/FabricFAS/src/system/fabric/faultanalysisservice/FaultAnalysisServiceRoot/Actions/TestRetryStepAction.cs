// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Threading;
    using Microsoft.ServiceFabric.Data;

    internal class TestRetryStepAction : FabricTestAction
    {
        public TestRetryStepAction(IReliableStateManager stateManager, IStatefulServicePartition partition, TestRetryStepState state, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(stateManager, partition, state, requestTimeout, operationTimeout)
        {
        }

        public override StepBase GetStep(
            FabricClient fabricClient,
            ActionStateBase actionState,
            StepStateNames stateName,
            CancellationToken cancellationToken)
        {
            return TestRetryStepFactory.GetStep(stateName, fabricClient, actionState, this, this.RequestTimeout, this.OperationTimeout, cancellationToken);
        }
    }
}