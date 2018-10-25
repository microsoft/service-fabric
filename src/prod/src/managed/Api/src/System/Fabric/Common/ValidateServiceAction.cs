// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ValidateServiceAction : ValidateAction
    {
        public ValidateServiceAction(Uri serviceName, TimeSpan maximumStabilizationTimeout)
        {
            this.ServiceName = serviceName;
            this.MaximumStabilizationTimeout = maximumStabilizationTimeout;
            this.CheckFlag = ValidationCheckFlag.All;
        }

        public Uri ServiceName { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(ValidateServiceActionHandler); }
        }

        private class ValidateServiceActionHandler : FabricTestActionHandler<ValidateServiceAction>
        {
            private TimeoutHelper timer;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, ValidateServiceAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.ServiceName, "ServiceName");

                this.timer = new TimeoutHelper(action.MaximumStabilizationTimeout);

                // TODO: make these actions which store state locally as well. 
                ServiceQueryClient serviceQueryClient = new ServiceQueryClient(
                    action.ServiceName,
                    testContext,
                    action.CheckFlag,
                    action.ActionTimeout,
                    action.RequestTimeout);

                var ensureStabilityTask = serviceQueryClient.EnsureStabilityAsync(
                    this.timer.GetRemainingTime(),
                    cancellationToken);

                var validateHealthTask = serviceQueryClient.ValidateHealthAsync(
                    this.timer.GetRemainingTime(),
                    cancellationToken);

                var tasks = new List<Task>
                                {
                                    ensureStabilityTask,
                                    validateHealthTask
                                };

                await Task.WhenAll(tasks).ConfigureAwait(false);

                this.ResultTraceString = StringHelper.Format(
                    "ValidateServiceAction succeeded for {0}",
                    action.ServiceName);
            }
        }
    }
}