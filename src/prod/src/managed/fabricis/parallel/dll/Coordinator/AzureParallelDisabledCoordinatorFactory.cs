// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Common;
    using Threading.Tasks;

    internal sealed class AzureParallelDisabledCoordinatorFactory : IInfrastructureCoordinatorFactory
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

        public AzureParallelDisabledCoordinatorFactory(
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
        }

        public IInfrastructureCoordinator Create()
        {
            var repairManager = new ServiceFabricRepairManager(env, activityLogger);
            var healthClient = new ServiceFabricHealthClient();

            var coordinator = new AzureParallelDisabledCoordinator(
                env,
                tenantId,
                this.CreatePolicyAgentClientAsync,
                repairManager,
                healthClient,
                activityLogger,
                partitionId,
                replicaId);

            return coordinator;
        }

        private Task<IPolicyAgentClient> CreatePolicyAgentClientAsync()
        {
            TraceType.WriteInfo("Trying to create policy agent client");
            var policyAgentServiceWrapper = new PolicyAgentServiceWrapper(env, activityLogger, silentErrors: true);
            return Task.FromResult<IPolicyAgentClient>(new PolicyAgentClient(env, policyAgentServiceWrapper, activityLogger));
        }
    }
}