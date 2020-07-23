// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.Common;

namespace System.Fabric.InfrastructureService.Test
{
    using Collections.Generic;

    internal class MockInfrastructureCoordinatorFactory : IInfrastructureCoordinatorFactory
    {
        private readonly IManagementClient managementClient;
        private readonly IDictionary<string, string> configSettings;
        private readonly IQueryClient queryClient;
        private readonly IInfrastructureAgentWrapper agent;

        private static readonly TraceType traceType = new TraceType(typeof(MockInfrastructureCoordinatorFactory).Name);

        /// <remarks>
        /// If <see cref="configSettings"/> is null, a default value is used.
        /// </remarks>
        public MockInfrastructureCoordinatorFactory(IManagementClient managementClient, IDictionary<string, string> configSettings = null, IQueryClient queryClient = null, IInfrastructureAgentWrapper agent = null)
        {
            managementClient.ThrowIfNull("managementClient");
           
            this.managementClient = managementClient;
            this.configSettings = configSettings != null
                ? new Dictionary<string, string>(configSettings, StringComparer.OrdinalIgnoreCase)
                : new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            this.queryClient = queryClient ?? new MockQueryClient();
            this.agent = agent ?? new MockInfrastructureAgentWrapper();
        }

        public IInfrastructureCoordinator Create()
        {
            string configSectionName = typeof(WindowsAzureInfrastructureCoordinatorTest).Name + "ConfigSection";

            const string ConfigJobPollingIntervalInSecondsKeyName = "WindowsAzure.JobPollingIntervalInSeconds";
            
            var configStore = new MockConfigStore();

            // loop faster in the ProcessManagementNotification loop of WindowsAzureInfrastructureCoordinator
            configStore.AddKeyValue(configSectionName, ConfigJobPollingIntervalInSecondsKeyName, "1");

            foreach (var pair in configSettings)
            {
                configStore.AddKeyValue(configSectionName, pair.Key, pair.Value);                
            }

            var partitionId = Guid.NewGuid();
            var jobBlockingPolicyManager = new MockJobBlockingPolicyManager();

            var jobImpactManager = new JobImpactManager(new ConfigSection(traceType, configStore, configSectionName), queryClient);

            var coordinator = new WindowsAzureInfrastructureCoordinator(
                "mytenant",
                managementClient,
                this.agent,
                configStore,
                configSectionName,
                partitionId,
                0,
                new MockHealthClient(),
                jobBlockingPolicyManager,
                jobImpactManager,
                null);

            return coordinator;
        }
    }
}