// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Fabric.Testability.Actions;
    using System.Fabric.Testability.Client;
    using System.Threading;
    using System.Threading.Tasks;

    public static class TestabilityExtensionsFuture
    {
        private const double RequestTimeoutFactor = 0.2;
        private const double MinRequestTimeoutInSeconds = 15.0;
        private static readonly TimeSpan MinRequestTimeout = TimeSpan.FromSeconds(MinRequestTimeoutInSeconds);

        private const double DefaultOperationTimeoutInSeconds = 300.0;
        internal static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromSeconds(DefaultOperationTimeoutInSeconds);

        // IFabricTestabilityClient extensions for test convenience
        public static void ConnectToEventStore(this IFabricTestabilityClient client, EventStoreConnectionInformation connInfo)
        {
            var systemFabricCient = GetSystemFabricClient(client);
            var testContext = systemFabricCient.FabricClient.TestManager.TestContext;
            var storeConnection = new EventStoreConnection(connInfo);
            testContext.ExtensionProperties.Add(Constants.EventStoreConnection, storeConnection);
        }

        public static async Task<IList<AbnormalProcessTerminationInformation>> GetAbnormalProcessTerminationInformationAsync(
            this IFabricTestabilityClient client, 
            DateTime start,
            DateTime end,
            string hostName,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            var systemFabricCient = GetSystemFabricClient(client);
            var testContext = systemFabricCient.FabricClient.TestManager.TestContext;
            AbnormalProcessTerminationAction abnornalTerminationAction = new AbnormalProcessTerminationAction(start, end, hostName);
            abnornalTerminationAction.ActionTimeout = operationTimeout;
            abnornalTerminationAction.RequestTimeout = TestabilityExtensionsFuture.GetRequestTimeout(operationTimeout);
            await testContext.ActionExecutor.RunAsync(abnornalTerminationAction, token);
            return abnornalTerminationAction.Result;
        }

        internal static async Task<ApiFaultInformation> GetApiFaultInformationAsync(
            this IFabricTestabilityClient client, 
            DateTime start,
            DateTime end,
            TimeSpan operationTimeout,
            CancellationToken token)
        {
            var systemFabricCient = GetSystemFabricClient(client);
            var testContext = systemFabricCient.FabricClient.TestManager.TestContext;
            var apiFaultInformationAction = new ApiFaultInformationAction(start, end)
            {
                ActionTimeout = operationTimeout,
                RequestTimeout = TestabilityExtensionsFuture.GetRequestTimeout(operationTimeout)
            };
            await testContext.ActionExecutor.RunAsync(apiFaultInformationAction, token);
            return apiFaultInformationAction.Result;
        }

        public static async Task<Replica> GetReplicaAsync(this IFabricTestabilityClient client, ReplicaSelector replicaSelector)
        {
            var systemFabricCient = GetSystemFabricClient(client);
            GetSelectedReplicaStateAction replicaAction = new GetSelectedReplicaStateAction(replicaSelector);
            await systemFabricCient.FabricClient.TestManager.TestContext.ActionExecutor.RunAsync(replicaAction);
            return replicaAction.Result.Item2;
        }

        public static async Task<SelectedPartition> GetPartitionAsync(this IFabricTestabilityClient client, PartitionSelector partitionSelector)
        {
            var systemFabricCient = GetSystemFabricClient(client);
            GetSelectedPartitionStateAction partitionAction = new GetSelectedPartitionStateAction(partitionSelector);
            await systemFabricCient.FabricClient.TestManager.TestContext.ActionExecutor.RunAsync(partitionAction);
            return partitionAction.Result;
        }

        public static async Task ValidateServiceAsync(
            this IFabricTestabilityClient client,
            Uri serviceName,
            bool allowQuorumLoss,
            TimeSpan stabilizationTimeout,
            CancellationToken token)
        {
            var systemFabricCient = GetSystemFabricClient(client);

            TestabilityTrace.TraceSource.WriteInfo("TestabilityExtensionsFuture", "ValidateServiceAsync(): stabilizationTimeout='{0}'", stabilizationTimeout);

            ValidateServiceAction validateServiceAction = new ValidateServiceAction(serviceName, stabilizationTimeout);
            validateServiceAction.CheckFlag = allowQuorumLoss ? ValidationCheckFlag.AllButQuorumLoss : ValidationCheckFlag.All;
            await systemFabricCient.FabricClient.TestManager.TestContext.ActionExecutor.RunAsync(validateServiceAction, token);
        }

        private static SystemFabricClient GetSystemFabricClient(IFabricTestabilityClient client)
        {
            var fabricClientHelper = client as FabricClientHelper;
            ReleaseAssert.AssertIf(fabricClientHelper == null, "Unexpected type for IFabricTestabilityClient: {0}", client.GetType());
            var systemFabricCient = fabricClientHelper.GetFabricClientObject(FabricClientTypes.System) as SystemFabricClient;
            ReleaseAssert.AssertIf(systemFabricCient == null, "Unexpected type for FabricClientObject: {0}", systemFabricCient.GetType());
            return systemFabricCient;
        }

        private static TimeSpan GetRequestTimeout(TimeSpan operationTimeout)
        {
            if (operationTimeout.TotalSeconds <= MinRequestTimeoutInSeconds)
            {
                return operationTimeout;
            }

            TimeSpan requestTimeout = TimeSpan.FromSeconds(RequestTimeoutFactor * operationTimeout.TotalSeconds);
            return requestTimeout < MinRequestTimeout ? MinRequestTimeout : requestTimeout;
        }
    }
}

#pragma warning restore 1591