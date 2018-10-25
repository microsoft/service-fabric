// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Query;

    internal class ValidateSystemServicesAction : ValidateAction
    {
        internal static FabricEvents.ExtensionsEvents Trace = AppTrace.TraceSource;

        private static readonly Uri SystemApplicationName = new Uri(Constants.SystemApplicationName);

        public ValidateSystemServicesAction(TimeSpan maximumStabilizationTimeout)
        {
            this.MaximumStabilizationTimeout = maximumStabilizationTimeout;
            this.CheckFlag = ValidationCheckFlag.All;
        }

        internal override Type ActionHandlerType
        {
            get { return typeof(ValidateSystemServicesActionHandler); }
        }

        private class ValidateSystemServicesActionHandler : FabricTestActionHandler<ValidateSystemServicesAction>
        {
            private TimeoutHelper timer;

            // Throws exception if validation was unsuccessful.
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, ValidateSystemServicesAction action, CancellationToken token)
            {
                this.timer = new TimeoutHelper(action.MaximumStabilizationTimeout);

                // Get all system services
                var svcListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.QueryManager.GetServiceListAsync(
                        SystemApplicationName, 
                        null, 
                        action.RequestTimeout, 
                        token), 
                    this.timer.GetRemainingTime(),
                    token).ConfigureAwait(false);

                List<Task> tasks = new List<Task>();
                foreach (Service svcResult in svcListResult)
                {
                    var validateService = new ValidateServiceAction(svcResult.ServiceName, this.timer.GetRemainingTime())
                    {
                        ActionTimeout = action.ActionTimeout,
                        RequestTimeout = action.RequestTimeout,
                        CheckFlag = action.CheckFlag
                    };

                    var task = testContext.ActionExecutor.RunAsync(validateService, token);
                    tasks.Add(task);
                    Trace.WriteNoise("ValidateSystemServicesActionHandler: Validation task added for service: {0}", svcResult.ServiceName.OriginalString);
                }

                await Task.WhenAll(tasks).ConfigureAwait(false);

                ResultTraceString = "ValidateSystemServicesActionHandler completed successfully";
            }
        }
    }
}