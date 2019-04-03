// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Actions
{
    using System.Fabric.Strings;
    using System.Collections.Generic;
    using System.Fabric.Testability.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common;

    internal class AbnormalProcessTerminationAction : FabricTestAction<IList<AbnormalProcessTerminationInformation>>
    {
        private static readonly string connectionPrameterName = Constants.EventStoreConnection;

        public AbnormalProcessTerminationAction(DateTime startTime, DateTime endTime, string hostName = null)
            : base()
        {
            this.StartTime = startTime;
            this.EndTime = endTime;
            this.HostName = hostName;
        }

        public DateTime StartTime { get; set; }

        public DateTime EndTime { get; set; }

        public string HostName { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(AbnormalProcessTerminationActionHandler); }
        }

        private class AbnormalProcessTerminationActionHandler : FabricTestActionHandler<AbnormalProcessTerminationAction>
        {
            protected async override Task ExecuteActionAsync(FabricTestContext testContext, AbnormalProcessTerminationAction action, CancellationToken cancellationToken)
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
                    Task<IList<AbnormalProcessTerminationInformation>>.Factory.StartNew(
                        () => EventStoreHelper.GetAbnormalProcessTerminationInformation(
                                    storeConnection,
                                    action.StartTime,
                                    action.EndTime,
                                    action.HostName),
                        cancellationToken);

                await task;

                ResultTraceString = StringHelper.Format("AbnormalProcessTerminationAction succeeded.");
                action.Result = task.Result;
            }
        }
    }
}