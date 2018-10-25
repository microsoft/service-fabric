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

    internal class ValidateApplicationServicesAction : ValidateAction
    {
        public ValidateApplicationServicesAction(Uri applicationName, TimeSpan maximumStabilizationTimeout)
        {
            this.ApplicationName = applicationName;
            this.MaximumStabilizationTimeout = maximumStabilizationTimeout;
            this.CheckFlag = ValidationCheckFlag.All;
        }

        public Uri ApplicationName { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(ValidateApplicationServicesActionHandler); }
        }

        private class ValidateApplicationServicesActionHandler : FabricTestActionHandler<ValidateApplicationServicesAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, ValidateApplicationServicesAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.ApplicationName, "ApplicationName");
                TimeoutHelper helper = new TimeoutHelper(action.MaximumStabilizationTimeout);

                // TODO: make these actions which store state locally as well. 
                var serviceListResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.QueryManager.GetServiceListAsync(
                        action.ApplicationName, 
                        null, 
                        action.RequestTimeout, 
                        cancellationToken),
                    helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                List<Task> serviceValidationTasks = new List<Task>();
                foreach (Service serviceResult in serviceListResult)
                {
                    var validateService = new ValidateServiceAction(serviceResult.ServiceName, helper.GetRemainingTime())
                    {
                        ActionTimeout = action.ActionTimeout,
                        RequestTimeout = action.RequestTimeout,
                        CheckFlag = action.CheckFlag
                    };

                    serviceValidationTasks.Add(testContext.ActionExecutor.RunAsync(validateService, cancellationToken));
                }

                await Task.WhenAll(serviceValidationTasks.ToArray()).ConfigureAwait(false);

                ResultTraceString = StringHelper.Format("ValidateApplicationServicesAction succeeded for {0}", action.ApplicationName);
            }
        }
    }
}