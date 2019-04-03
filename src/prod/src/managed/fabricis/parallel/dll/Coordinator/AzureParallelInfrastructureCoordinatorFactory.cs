// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;
    using Common;

    internal sealed class AzureParallelInfrastructureCoordinatorFactory : IInfrastructureCoordinatorFactory, IAzureModeDetectorFactory
    {
        /// <summary>
        /// The fallback if we cannot get the tenant Id.
        /// </summary>
        private const string PartitionIdPrefix = "Partition-";

        private static readonly TraceType TraceType = new TraceType(Constants.TraceTypeName);

        private readonly Uri serviceName;
        private readonly IConfigSection configSection;
        private readonly IInfrastructureAgentWrapper agent;
        private readonly Guid partitionId;
        private readonly long replicaId;

        private readonly string tenantId;
        private readonly CoordinatorEnvironment env;
        private readonly IActivityLogger activityLogger;
        private readonly PolicyAgentServiceWrapper policyAgentServiceWrapper;

        public AzureParallelInfrastructureCoordinatorFactory(
            Uri serviceName,
            IConfigStore configStore,
            string configSectionName,
            Guid partitionId,
            long replicaId,
            IInfrastructureAgentWrapper agent)
        {
            this.serviceName = serviceName.Validate("serviceName");
            configStore.Validate("configStore");
            configSectionName.Validate("configSectionName");
            this.agent = agent.Validate("agent");

            this.configSection = new ConfigSection(TraceType, configStore, configSectionName);

            this.partitionId = partitionId;
            this.replicaId = replicaId;

            try
            {
                this.tenantId = AzureHelper.GetTenantId(configSection);
            }
            catch (Exception ex)
            {
                // this happens on the Linux environment (since there is no registry)
                this.tenantId = PartitionIdPrefix + partitionId;
                TraceType.WriteWarning("Unable to get tenant Id from configuration. Using partition Id '{0}' text instead. Exception: {1}", this.tenantId, ex);                
            }            

            // TODO use tenant ID or config section name suffix as base trace ID?
            this.env = new CoordinatorEnvironment(this.serviceName.AbsoluteUri, this.configSection, tenantId, this.agent);
            this.activityLogger = new ActivityLoggerFactory().Create(env.CreateTraceType("Event"));

            this.policyAgentServiceWrapper = new PolicyAgentServiceWrapper(env, activityLogger);
        }

        public IInfrastructureCoordinator Create()
        {
            var repairManager = new ServiceFabricRepairManagerFactory(env, activityLogger).Create();
            if (repairManager == null)
            {
                const string message = "Unable to create Repair Manager client; cannot continue further.";
                TraceType.WriteWarning(message);
                throw new ManagementException(message);
            }

            var policyAgentClient = new PolicyAgentClient(env, policyAgentServiceWrapper, activityLogger);

            var retryPolicyFactory = new LinearRetryPolicyFactory(
                env.DefaultTraceType,
                InfrastructureService.Constants.BackoffPeriodInMilliseconds,
                InfrastructureService.Constants.MaxRetryAttempts,
                AzureHelper.IsRetriableException);

            string tenantSpecificStoreName = "{0}/{1}".ToString(InfrastructureService.Constants.StoreName, configSection.Name);

            var tenantSpecificVersionedPropertyStore = VersionedPropertyStore.CreateAsync(
                Guid.NewGuid(),
                env.CreateTraceType("PropertyStore"),
                new Uri(tenantSpecificStoreName),
                new PropertyManagerWrapper(),
                retryPolicyFactory).GetAwaiter().GetResult();

            // if this exists, job blocking policy manager will migrate job blocking policy properties 
            // from this one to the tenant-specific store.
            var versionedPropertyStore = VersionedPropertyStore.CreateAsync(
                Guid.NewGuid(),
                env.CreateTraceType("PropertyStoreOld"),
                new Uri(InfrastructureService.Constants.StoreName), 
                new PropertyManagerWrapper(), 
                retryPolicyFactory).GetAwaiter().GetResult();

            var jobBlockingPolicyManager = JobBlockingPolicyManager.CreateAsync(
                env.CreateTraceType("JobBlockingPolicyManager"),
                tenantSpecificVersionedPropertyStore,
                versionedPropertyStore).GetAwaiter().GetResult();

            var allowActionMap = new AllowActionMap();
            var coordinatorCommandProcessor = new CoordinatorCommandProcessor(
                env,
                policyAgentClient,
                jobBlockingPolicyManager,
                allowActionMap);
            var mappedPolicyFactory = new DefaultActionPolicyFactory(env, jobBlockingPolicyManager, allowActionMap);

            var coordinator = 
                new AzureParallelInfrastructureCoordinator(
                    env,
                    tenantId,
                    policyAgentClient,
                    repairManager,
                    new ServiceFabricHealthClient(), 
                    coordinatorCommandProcessor,
                    jobBlockingPolicyManager,
                    mappedPolicyFactory, 
                    activityLogger,
                    partitionId, 
                    replicaId);

            return coordinator;
        }

        public IAzureModeDetector CreateAzureModeDetector()
        {
            return this.policyAgentServiceWrapper;
        }
    }
}