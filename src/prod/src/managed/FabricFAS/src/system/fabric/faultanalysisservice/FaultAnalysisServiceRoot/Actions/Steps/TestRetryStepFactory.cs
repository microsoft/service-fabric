// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Threading;
    using Fabric.Common;
    using Globalization;

    internal static class TestRetryStepFactory
    {
        public static StepBase GetStep(
            StepStateNames stateName,
            FabricClient fabricClient,
            ActionStateBase actionState,
            TestRetryStepAction action,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            StepBase step = null;

            if (stateName == StepStateNames.LookingUpState)
            {
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - constructing step (1)", actionState.OperationId);
                step = new TestRetryStepStepOne(fabricClient, actionState, requestTimeout, operationTimeout);
            }
            else if (stateName == StepStateNames.PerformingActions)
            {
                TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - constructing step (2)", actionState.OperationId);
                step = new TestRetryStepStepTwo(fabricClient, actionState, requestTimeout, operationTimeout);
            }
            else if (stateName == StepStateNames.CompletedSuccessfully)
            {
                // done - but then this method should not have been called               
                ReleaseAssert.Failfast("GetStep() should not have been called when the state name is CompletedSuccessfully");
            }
            else
            {
                ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "Unexpected state name={0}", stateName));
            }

            return step;
        }
    }
}