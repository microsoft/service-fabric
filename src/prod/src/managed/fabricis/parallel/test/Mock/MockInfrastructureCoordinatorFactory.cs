// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using InfrastructureService.Test;

    internal sealed class MockInfrastructureCoordinatorFactory : IInfrastructureCoordinatorFactory
    {        
        private readonly IDictionary<string, string> configSettings;

        private readonly IPolicyAgentClient policyAgentClient; // TODO, pass in factory or object?

        private readonly IRepairManager repairManager;

        /// <remarks>
        /// If <see cref="configSettings"/> is null, a default value is used.
        /// </remarks>
        public MockInfrastructureCoordinatorFactory(
            IDictionary<string, string> configSettings = null,
            IPolicyAgentClient policyAgentClient = null,
            IRepairManager repairManager = null)
        {
            this.configSettings = configSettings != null
                ? new Dictionary<string, string>(configSettings, StringComparer.OrdinalIgnoreCase)
                : new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            var activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);

            var env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                new ConfigSection(Constants.TraceType, new MockConfigStore(), "ConfigSection"),
                string.Empty,
                new MockInfrastructureAgentWrapper());

            this.policyAgentClient = policyAgentClient ?? new PolicyAgentClient(env, new PolicyAgentServiceWrapper(env, activityLogger), activityLogger);
            this.repairManager = repairManager ?? new MockRepairManager();
        }

        public IInfrastructureCoordinator Create()
        {
            string configSectionName = typeof(MockInfrastructureCoordinatorFactory).Name + "ConfigSection";

            const string ConfigJobPollingIntervalInSecondsKeyName = Parallel.Constants.ConfigKeys.JobPollingIntervalInSeconds;

            var configStore = new MockConfigStore();

            // loop faster in the ProcessManagementNotification loop of WindowsAzureInfrastructureCoordinator
            configStore.AddKeyValue(configSectionName, ConfigJobPollingIntervalInSecondsKeyName, "1");

            foreach (var pair in configSettings)
            {
                configStore.AddKeyValue(configSectionName, pair.Key, pair.Value);
            }

            var partitionId = Guid.NewGuid();
            var jobBlockingPolicyManager = new MockJobBlockingPolicyManager();
            var activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);
            var configSection = new ConfigSection(Constants.TraceType, configStore, configSectionName);            
            var allowActionMap = new AllowActionMap();
            var env = new CoordinatorEnvironment("fabric:/System/InfrastructureService/Mock", configSection, "mytenant", new MockInfrastructureAgentWrapper());
            var coordinatorCommandProcessor = new CoordinatorCommandProcessor(env, policyAgentClient, jobBlockingPolicyManager, allowActionMap);

            var coordinator = new AzureParallelInfrastructureCoordinator(
                env,
                "mytenant",
                policyAgentClient,
                repairManager, 
                new MockHealthClient(), 
                coordinatorCommandProcessor,
                jobBlockingPolicyManager,
                new DefaultActionPolicyFactory(env, jobBlockingPolicyManager, allowActionMap), 
                activityLogger,
                partitionId, 
                0);

            return coordinator;
        }
    }
}