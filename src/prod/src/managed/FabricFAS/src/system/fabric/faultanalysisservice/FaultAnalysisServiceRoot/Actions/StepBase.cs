// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{
    using System.Fabric.Common;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Threading;
    using System.Threading.Tasks;

    internal abstract class StepBase
    {
        public const string TraceType = "Steps";

        public StepBase(FabricClient fabricClient, ActionStateBase stepState, TimeSpan requestTimeout, TimeSpan operationTimeout)
        {
            ReleaseAssert.AssertIf(fabricClient == null, "fabricClient should not be null");
            ReleaseAssert.AssertIf(stepState == null, "'stepState' should not be null");

            this.FabricClient = fabricClient;
            this.State = stepState;
            this.RequestTimeout = requestTimeout;
            this.OperationTimeout = operationTimeout;
        }

        public abstract StepStateNames StepName
        {
            get;
        }

        public FabricClient FabricClient
        {
            get;
            private set;
        }

        public ActionStateBase State
        {
            get;
            private set;
        }

        public TimeSpan RequestTimeout
        {
            get;
            private set;
        }

        public TimeSpan OperationTimeout
        {
            get;
            private set;
        }

        public abstract Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo);

        public virtual Task CleanupAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult<bool>(true);
        }
    }
}