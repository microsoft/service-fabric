// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ValidateAllServicesAction : ValidateAction
    {
        public ValidateAllServicesAction(TimeSpan maximumStabilizationTimeout)
        {
            this.MaximumStabilizationTimeout = maximumStabilizationTimeout;
            this.CheckFlag = ValidationCheckFlag.All;
        }

        internal override Type ActionHandlerType
        {
            get { return typeof(ValidateAllServicesActionHandler); }
        }

        private class ValidateAllServicesActionHandler : FabricTestActionHandler<ValidateAllServicesAction>
        {
            // Throws exception if validation was unsuccessful.
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, ValidateAllServicesAction action, CancellationToken token)
            {
                var timer = new TimeoutHelper(action.MaximumStabilizationTimeout);

                //// Validate system services first.
                var validateSystemServices = new ValidateSystemServicesAction(timer.GetRemainingTime())
                {
                    ActionTimeout = action.ActionTimeout,
                    RequestTimeout = action.RequestTimeout,
                    CheckFlag = action.CheckFlag
                };

                await testContext.ActionExecutor.RunAsync(validateSystemServices, token).ConfigureAwait(false);

                var appListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.QueryManager.GetApplicationListAsync(
                        null, 
                        string.Empty, 
                        action.RequestTimeout, 
                        token),
                    timer.GetRemainingTime(),
                    token).ConfigureAwait(false);

                List<Task> tasks = new List<Task>();
                foreach (Application appResult in appListResult)
                {
                    var validateAppServices = new ValidateApplicationServicesAction(appResult.ApplicationName, timer.GetRemainingTime())
                    {
                        ActionTimeout = action.ActionTimeout,
                        RequestTimeout = action.RequestTimeout,
                        CheckFlag = action.CheckFlag
                    };

                    var task = testContext.ActionExecutor.RunAsync(validateAppServices, token);
                    tasks.Add(task);
                    this.ActionTraceSource.WriteNoise(TraceType, "ValidateAllServicesActionHandler: Validation task added for application: {0}", appResult.ApplicationName.OriginalString);
                }

                await Task.WhenAll(tasks).ConfigureAwait(false);

                this.ResultTraceString = "ValidateAllServicesActionHandler completed for all services";
            }
        }
    }
}