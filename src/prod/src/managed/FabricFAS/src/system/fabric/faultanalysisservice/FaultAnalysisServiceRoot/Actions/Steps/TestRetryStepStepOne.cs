// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Threading;
    using System.Threading.Tasks;
    using Fabric.Common;

    internal class TestRetryStepStepOne : StepBase
    {
        public TestRetryStepStepOne(FabricClient fabricClient, ActionStateBase state, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(fabricClient, state, requestTimeout, operationTimeout)
        {
        }

        public override StepStateNames StepName
        {
            get { return StepStateNames.LookingUpState; }
        }

        public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Inside TestRetryStepStepOne", this.State.OperationId);

            // Simulate work
            await Task.Delay(TimeSpan.FromMilliseconds(500)).ConfigureAwait(false);
            this.State.StateProgress.Push(StepStateNames.PerformingActions);
            return this.State;
        }
    }
}