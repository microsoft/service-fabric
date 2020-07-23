// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMdsAgentSvc
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    internal class Service : IStatelessServiceInstance
    {
        private StatelessServiceInitializationParameters initializationParameters;
        private MdsAgentManager mdsAgentManager;
        private MonitoringAgentServiceEvent trace;

        public void Initialize(StatelessServiceInitializationParameters initializationParameters)
        {
            this.initializationParameters = initializationParameters;
            this.trace = new MonitoringAgentServiceEvent(
                this.initializationParameters.CodePackageActivationContext.WorkDirectory,
                this.initializationParameters.PartitionId,
                this.initializationParameters.InstanceId);

            ConfigReader.Initialize(this.initializationParameters, this.trace);
        }

        public Task<string> OpenAsync(IStatelessServicePartition partition, CancellationToken cancellationToken)
        {
            this.trace.Message("Service.OpenAsync: method invoked.");

            this.mdsAgentManager = new MdsAgentManager(this.initializationParameters, this.trace);
            this.mdsAgentManager.Start();
            return Utility.CreateCompletedTask(string.Empty);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            this.trace.Message("Service.CloseAsync: method invoked.");
            return this.mdsAgentManager.StopAsync();
        }

        public void Abort()
        {
            this.trace.Message("Service.Abort: method invoked.");

            // Just exit the process abruptly, which will cause the MA also
            // to exit abruptly.
            Environment.Exit(0);
        }
    }
}