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

    internal class TestRetryStepStepTwo : StepBase
    {
        public TestRetryStepStepTwo(FabricClient fabricClient, ActionStateBase state, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(fabricClient, state, requestTimeout, operationTimeout)
        {
        }

        public override StepStateNames StepName
        {
            get { return StepStateNames.PerformingActions; }
        }

        public static TestRetryStepState Convert(ActionStateBase actionState)
        {
            TestRetryStepState testRetryStepState = actionState as TestRetryStepState;
            if (testRetryStepState == null)
            {
                throw new InvalidCastException("State object could not be converted");
            }

            return testRetryStepState;
        }

        public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Inside TestRetryStepStepTwo - this should retry w/o rollback when exception is thrown", this.State.OperationId);
            TestRetryStepState castedState = Convert(this.State);

            // Simulate work
            await Task.Delay(TimeSpan.FromMilliseconds(500)).ConfigureAwait(false);

            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Inside TestRetryStepStepTwo - before PerformInternalServiceFaultIfRequested", this.State.OperationId);
            ActionTest.PerformInternalServiceFaultIfRequested(this.State.OperationId, serviceInternalFaultInfo, this.State, cancellationToken, true);
            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - Inside TestRetryStepStepTwo - after PerformInternalServiceFaultIfRequested", this.State.OperationId);

            this.State.StateProgress.Push(StepStateNames.CompletedSuccessfully);

            return this.State;
        }
    }
}