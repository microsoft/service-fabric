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

    internal class StuckStep : StepBase
    {
        public StuckStep(FabricClient fabricClient, ActionStateBase state, TimeSpan requestTimeout, TimeSpan operationTimeout)
            : base(fabricClient, state, requestTimeout, operationTimeout)
        {
        }

        public override StepStateNames StepName
        {
            get { return StepStateNames.LookingUpState; }
        }

        public override async Task<ActionStateBase> RunAsync(CancellationToken cancellationToken, ServiceInternalFaultInfo serviceInternalFaultInfo)
        {
            // Intentionally get stuck
            await Task.Delay(Timeout.Infinite, cancellationToken).ConfigureAwait(false);

            return null;
        }
    }    
}