// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Fabric.Common;
    using System.Globalization;
    using Microsoft.Win32;

    /// <summary>
    /// A class that creates the <see cref="WindowsAzureInfrastructureCoordinator"/>.
    /// </summary>
    internal class WindowsAzureInfrastructureCoordinatorFactory : IInfrastructureCoordinatorFactory
    {
        /// <summary>
        /// The setting to be provided in the config (e.g. cluster manifest) to load the corresponding coordinator.
        /// </summary>
        private const string CoordinatorTypeKeyName = "CoordinatorType";        
        private const string CoordinatorTypeValueServerRestart = "ServerRestart";

        private static readonly TraceType TraceType = new TraceType("InfrastructureCoordinatorFactory");

        private readonly IConfigStore configStore;
        private readonly string configSectionName;
        private readonly IInfrastructureAgentWrapper agent;
        private readonly Guid partitionId;
        private readonly long replicaId;
        private readonly IAzureModeDetector modeDetector;

        public WindowsAzureInfrastructureCoordinatorFactory(
            IConfigStore configStore, 
            string configSectionName,
            Guid partitionId,
            long replicaId,
            IInfrastructureAgentWrapper agent,
            IAzureModeDetector modeDetector)
        {
            configStore.ThrowIfNull("configStore");
            configSectionName.ThrowIfNullOrWhiteSpace("configSectionName");            
            agent.ThrowIfNull("agent");

            this.configStore = configStore;
            this.configSectionName = configSectionName;
            this.agent = agent;
            this.partitionId = partitionId;
            this.replicaId = replicaId;
            this.modeDetector = modeDetector;
        }

        public IInfrastructureCoordinator Create()
        {
            IManagementClient managementClient;

            try
            {
                managementClient = new ManagementClientFactory().Create();
            }
            catch (Exception ex)
            {
                Trace.WriteError(TraceType, "Error creating management client. Cannot continue further. Exception: {0}", ex);
                throw;
            }

            var retryPolicyFactory = new LinearRetryPolicyFactory(
                TraceType,
                Constants.BackoffPeriodInMilliseconds,
                Constants.MaxRetryAttempts,
                AzureHelper.IsRetriableException);

            var versionedPropertyStore = VersionedPropertyStore.CreateAsync(
                Guid.NewGuid(),
                TraceType,
                new Uri(Constants.StoreName), 
                new PropertyManagerWrapper(), retryPolicyFactory).GetAwaiter().GetResult();

            var configSection = new ConfigSection(TraceType, configStore, configSectionName);
            var jobBlockingPolicyManager = JobBlockingPolicyManager.CreateAsync(TraceType, versionedPropertyStore).GetAwaiter().GetResult();
            var jobImpactManager = new JobImpactManager(configSection, new ServiceFabricQueryClient(TraceType));

            var coordinator = new WindowsAzureInfrastructureCoordinator(
                AzureHelper.GetTenantId(configSection),
                managementClient,
                agent,
                configStore,
                configSectionName,
                partitionId,
                replicaId,
                new ServiceFabricHealthClient(),
                jobBlockingPolicyManager,
                jobImpactManager,
                this.modeDetector);
            
            return coordinator;
        }
    }
}