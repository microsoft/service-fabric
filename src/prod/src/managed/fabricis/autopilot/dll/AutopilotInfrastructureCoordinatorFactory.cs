// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Common;
    using Microsoft.Search.Autopilot;

    internal sealed class AutopilotInfrastructureCoordinatorFactory : IInfrastructureCoordinatorFactory
    {
        private readonly Uri serviceName;
        private readonly IConfigSection configSection;
        private readonly Guid partitionId;
        private readonly long replicaId;

        private readonly CoordinatorEnvironment env;

        public AutopilotInfrastructureCoordinatorFactory(
            Uri serviceName,
            IConfigStore configStore,
            string configSectionName,
            Guid partitionId,
            long replicaId)
        {
            this.serviceName = serviceName.Validate("serviceName");
            configStore.Validate("configStore");
            configSectionName.Validate("configSectionName");

            this.configSection = new ConfigSection(new TraceType(Constants.TraceTypeName + "Config"), configStore, configSectionName);

            this.partitionId = partitionId;
            this.replicaId = replicaId;

            this.env = new CoordinatorEnvironment(this.serviceName.AbsoluteUri, this.configSection, string.Empty);

            TraceType factoryTraceType = this.env.CreateTraceType("Factory");

            factoryTraceType.WriteInfo(
                "Autopilot coordinator factory created; AppRoot = '{0}'",
                Environment.GetEnvironmentVariable("AppRoot"));

            try
            {
                if (APRuntime.IsInitialized)
                {
                    factoryTraceType.WriteInfo("APRuntime is already initialized");
                }
                else
                {
                    factoryTraceType.WriteInfo("Initializing APRuntime");
                    APRuntime.Initialize();
                    factoryTraceType.WriteInfo("Initialized APRuntime successfully");
                }
            }
            catch (Exception e)
            {
                factoryTraceType.WriteError("Failed to initialize APRuntime: {0}", e);
                throw;
            }
        }

        public IInfrastructureCoordinator Create()
        {
            var coordinator =
                new AutopilotInfrastructureCoordinator(
                    env,
                    new AutopilotDMClient(),
                    new ServiceFabricRepairManager(env),
                    new ServiceFabricHealthClient(), 
                    partitionId, 
                    replicaId);

            return coordinator;
        }
    }
}