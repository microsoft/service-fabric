// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Actions
{
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Fabric.Testability.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal class ApiFaultInformationAction : FabricTestAction<ApiFaultInformation>
    {
        private static readonly string connectionPrameterName = Constants.EventStoreConnection;

        public ApiFaultInformationAction(DateTime startTime, DateTime endTime)
            : base()
        {
            this.StartTime = startTime;
            this.EndTime = endTime;
        }

        public DateTime StartTime { get; set; }

        public DateTime EndTime { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(ApiFaultInformationActionHandler); }
        }

        private class ApiFaultInformationActionHandler : FabricTestActionHandler<ApiFaultInformationAction>
        {
            protected async override Task ExecuteActionAsync(FabricTestContext testContext, ApiFaultInformationAction action, CancellationToken cancellationToken)
            {
                object value;
                var storeConnection = testContext.ExtensionProperties.TryGetValue(connectionPrameterName, out value)
                                          ? value as
                                            EventStoreConnection
                                          : null;

                if (storeConnection == null)
                {
                    throw new InvalidOperationException(StringResources.EventStoreError_ConnectionRequired);
                }

                var task =
                    Task<ApiFaultInformation>.Factory.StartNew(
                        () => EventStoreHelper.GetApiFaultInformation(
                                    storeConnection,
                                    action.StartTime,
                                    action.EndTime),
                        cancellationToken);

                await task;

                ResultTraceString = StringHelper.Format("ApiFaultInformationAction succeeded.");
                action.Result = task.Result;
            }
        }
    }
}